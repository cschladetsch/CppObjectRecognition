#include "ShapeDetector/SphereDetector.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
#include <omp.h>
#include <queue>
#include <stack>
#include <unordered_set>

constexpr double MIN_DISTANCE_SQUARED = 4.0;
constexpr double EPSILON_TOLERANCE = 1e-9;
constexpr double PI = std::numbers::pi;

ObloidDetector::ObloidDetector()
    : minRadius_(10), maxRadius_(100), circularityThreshold_(0.8), confidenceThreshold_(0.7) {
  // Pre-allocate caches for better performance
  distanceCache_.reserve(1000);
  radiusCache_.reserve(100);
}

ObloidDetector::~ObloidDetector() {}

void ObloidDetector::SetMinRadius(int minRadius) { minRadius_ = minRadius; }

void ObloidDetector::SetMaxRadius(int maxRadius) { maxRadius_ = maxRadius; }

void ObloidDetector::SetCircularityThreshold(double threshold) {
  circularityThreshold_ = threshold;
}

void ObloidDetector::SetConfidenceThreshold(double threshold) {
  confidenceThreshold_ = threshold;
}

std::vector<Obloid> ObloidDetector::DetectObloids(const Image &image) {
  std::vector<Obloid> obloids;
  obloids.reserve(20);

  // Preprocess image for obloid detection
  Image processed = PreprocessImage(image);
  std::vector<std::vector<Point>> contours = FindContours(processed);

  // Process contours to find obloids
  if (contours.size() > 10) {
    // Parallel processing for large number of contours
    std::vector<Obloid> tempObloids(contours.size());
    std::vector<bool> validObloids(contours.size(), false);

#pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < contours.size(); ++i) {
      Obloid obloid;
      if (IsObloid(contours[i], obloid)) {
        if (obloid.radius >= minRadius_ && obloid.radius <= maxRadius_ &&
            obloid.confidence >= confidenceThreshold_) {
          tempObloids[i] = obloid;
          validObloids[i] = true;
        }
      }
    }

    // Collect valid obloids
    for (size_t i = 0; i < contours.size(); ++i) {
      if (validObloids[i]) {
        obloids.push_back(tempObloids[i]);
      }
    }
  } else {
    // Sequential processing for small number of contours
    for (const auto &contour : contours) {
      Obloid obloid;
      if (IsObloid(contour, obloid)) {
        if (obloid.radius >= minRadius_ && obloid.radius <= maxRadius_ &&
            obloid.confidence >= confidenceThreshold_) {
          obloids.push_back(obloid);
        }
      }
    }
  }

  // Remove duplicate obloids
  RemoveDuplicateObloids(obloids);

  return obloids;
}

Image ObloidDetector::PreprocessImage(const Image &image) const {
  Image result = image;

  // Apply Gaussian blur for noise reduction
  Image blurred = ApplyGaussianBlur(result, 1.0);

  // Apply thresholding optimized for circular shapes
#pragma omp parallel for
  for (int y = 0; y < blurred.height; ++y) {
    for (int x = 0; x < blurred.width; ++x) {
      result.pixels[y][x] = (blurred.pixels[y][x] > 127) ? 255 : 0;
    }
  }

  return result;
}

std::vector<std::vector<Point>> ObloidDetector::FindContours(const Image &image) const {
  std::vector<std::vector<Point>> contours;
  contours.reserve(50);
  std::vector<std::vector<bool>> visited(image.height,
                                         std::vector<bool>(image.width, false));

  // Find all connected white regions
  for (int y = 0; y < image.height; ++y) {
    for (int x = 0; x < image.width; ++x) {
      if (!visited[y][x] && image.pixels[y][x] == 255) {
        std::vector<Point> region;
        region.reserve(500);
        ScanlineFillContour(image, x, y, region, visited);

        if (region.size() >= 20) { // Minimum size for a circle
          std::vector<Point> boundary = ExtractBoundary(region, image);
          if (boundary.size() >= 8) {
            contours.push_back(std::move(boundary));
          }
        }
      }
    }
  }

  return contours;
}

void ObloidDetector::ScanlineFillContour(
    const Image &image, int startX, int startY, std::vector<Point> &contour,
    std::vector<std::vector<bool>> &visited) const {
  
  // Efficient scanline flood fill algorithm (reusing from RectangleDetector)
  std::stack<ScanlineSegment> stack;

  // Find initial horizontal segment
  int x1 = startX, x2 = startX;
  while (x1 > 0 && image.pixels[startY][x1 - 1] == 255 &&
         !visited[startY][x1 - 1])
    x1--;
  while (x2 < image.width - 1 && image.pixels[startY][x2 + 1] == 255 &&
         !visited[startY][x2 + 1])
    x2++;

  stack.push(ScanlineSegment(startY, x1, x2, -1));

  while (!stack.empty()) {
    ScanlineSegment seg = stack.top();
    stack.pop();

    // Process scanline
    for (int x = seg.x1; x <= seg.x2; x++) {
      if (!visited[seg.y][x]) {
        visited[seg.y][x] = true;
        contour.emplace_back(x, seg.y);
      }
    }

    // Check lines above and below
    for (int dir = -1; dir <= 1; dir += 2) {
      int newY = seg.y + dir;
      if (newY < 0 || newY >= image.height)
        continue;

      int x = seg.x1;
      while (x <= seg.x2) {
        while (x <= seg.x2 &&
               (image.pixels[newY][x] != 255 || visited[newY][x]))
          x++;
        if (x > seg.x2)
          break;

        int newX1 = x;
        while (x <= seg.x2 && image.pixels[newY][x] == 255 && !visited[newY][x])
          x++;
        int newX2 = x - 1;

        while (newX1 > 0 && image.pixels[newY][newX1 - 1] == 255 &&
               !visited[newY][newX1 - 1])
          newX1--;
        while (newX2 < image.width - 1 &&
               image.pixels[newY][newX2 + 1] == 255 &&
               !visited[newY][newX2 + 1])
          newX2++;

        stack.push(ScanlineSegment(newY, newX1, newX2, seg.y));
      }
    }
  }
}

bool ObloidDetector::IsObloid(const std::vector<Point> &contour, Obloid &obloid) const {
  if (contour.size() < 8)
    return false;

  // Calculate basic shape properties
  double circularity = CalculateCircularity(contour);
  if (circularity < circularityThreshold_)
    return false;

  // Fit a circle to the contour
  obloid = FitCircleToContour(contour);
  
  // Validate circle geometry
  if (!ValidateCircleGeometry(contour, obloid.center, obloid.radius))
    return false;

  // Calculate confidence based on how well the contour fits a circle
  double fitError = CalculateCircleFitError(contour, obloid.center, obloid.radius);
  obloid.confidence = std::max(0.0, 1.0 - fitError / obloid.radius);

  return obloid.confidence >= confidenceThreshold_;
}

Obloid ObloidDetector::CreateObloid(const std::vector<Point> &contour) const {
  return FitCircleToContour(contour);
}

double ObloidDetector::CalculateCircularity(const std::vector<Point> &contour) const {
  if (contour.size() < 3)
    return 0.0;

  double area = CalculateArea(contour);
  double perimeter = CalculatePerimeter(contour);

  if (perimeter < EPSILON_TOLERANCE)
    return 0.0;

  // Circularity = 4π * Area / Perimeter²
  // Perfect circle has circularity = 1.0
  return (4.0 * PI * area) / (perimeter * perimeter);
}

double ObloidDetector::CalculatePerimeter(const std::vector<Point> &contour) const {
  if (contour.size() < 2)
    return 0.0;

  // For filled regions, estimate perimeter from area assuming circular shape
  // Perimeter of circle = 2 * π * r, where r = sqrt(area / π)
  double area = static_cast<double>(contour.size());
  double radius = std::sqrt(area / PI);
  return 2.0 * PI * radius;
}

double ObloidDetector::CalculateArea(const std::vector<Point> &contour) const {
  if (contour.size() < 3)
    return 0.0;

  // Since our contour contains all pixels in the filled region (from flood fill),
  // not just the boundary, the area is simply the number of pixels
  return static_cast<double>(contour.size());
}

Point ObloidDetector::CalculateCentroid(const std::vector<Point> &contour) const {
  if (contour.empty())
    return Point(0, 0);

  double sumX = 0, sumY = 0;
  for (const auto &point : contour) {
    sumX += point.x;
    sumY += point.y;
  }

  return Point(static_cast<int>(std::round(sumX / contour.size())),
               static_cast<int>(std::round(sumY / contour.size())));
}

int ObloidDetector::EstimateRadius(const std::vector<Point> &contour, const Point &center) const {
  if (contour.empty())
    return 0;

  double sumRadius = 0.0;
  for (const auto &point : contour) {
    double dx = point.x - center.x;
    double dy = point.y - center.y;
    sumRadius += std::sqrt(dx * dx + dy * dy);
  }

  return static_cast<int>(std::round(sumRadius / contour.size()));
}

double ObloidDetector::CalculateRadialVariance(const std::vector<Point> &contour, 
                                               const Point &center, int radius) const {
  if (contour.empty())
    return 0.0;

  double variance = 0.0;
  for (const auto &point : contour) {
    double dx = point.x - center.x;
    double dy = point.y - center.y;
    double distance = std::sqrt(dx * dx + dy * dy);
    double diff = distance - radius;
    variance += diff * diff;
  }

  return variance / contour.size();
}

bool ObloidDetector::IsCircularContour(const std::vector<Point> &contour) const {
  if (contour.size() < 8)
    return false;

  Point center = CalculateCentroid(contour);
  int radius = EstimateRadius(contour, center);

  if (radius < minRadius_ || radius > maxRadius_)
    return false;

  double variance = CalculateRadialVariance(contour, center, radius);
  double normalizedVariance = variance / (radius * radius);

  return normalizedVariance < 0.1; // Threshold for circular variance
}

std::vector<Point> ObloidDetector::ExtractBoundary(const std::vector<Point> &region,
                                                   const Image &image) const {
  std::vector<Point> boundary;
  boundary.reserve(region.size() / 4);

  for (const Point &p : region) {
    bool isBoundary = false;

    // Check 8-connected neighbors
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0)
          continue;

        int nx = p.x + dx;
        int ny = p.y + dy;

        if (nx < 0 || nx >= image.width || ny < 0 || ny >= image.height ||
            image.pixels[ny][nx] == 0) {
          isBoundary = true;
          break;
        }
      }
      if (isBoundary)
        break;
    }

    if (isBoundary) {
      boundary.push_back(p);
    }
  }

  return boundary;
}

void ObloidDetector::RemoveDuplicateObloids(std::vector<Obloid> &obloids) const {
  if (obloids.size() <= 1)
    return;

  // Sort by radius for consistent processing (keep larger obloids)
  std::sort(obloids.begin(), obloids.end(),
            [](const Obloid &a, const Obloid &b) {
              return a.radius > b.radius;
            });

  std::vector<bool> toRemove(obloids.size(), false);

  for (size_t i = 0; i < obloids.size(); ++i) {
    if (toRemove[i])
      continue;

    for (size_t j = i + 1; j < obloids.size(); ++j) {
      if (toRemove[j])
        continue;

      // Check if obloids overlap significantly
      double centerDist = std::sqrt(
          std::pow(obloids[i].center.x - obloids[j].center.x, 2) +
          std::pow(obloids[i].center.y - obloids[j].center.y, 2));

      double radiusSum = obloids[i].radius + obloids[j].radius;

      // If distance between centers is less than 70% of radius sum, consider them duplicates
      if (centerDist < radiusSum * 0.7) {
        toRemove[j] = true; // Remove smaller/later one
      }
    }
  }

  // Remove marked obloids
  obloids.erase(
      std::remove_if(obloids.begin(), obloids.end(),
                     [&toRemove, &obloids](const Obloid &obloid) {
                       return toRemove[&obloid - &obloids[0]];
                     }),
      obloids.end());
}

Image ObloidDetector::ApplyGaussianBlur(const Image &image, double sigma) const {
  if (sigma <= 0.1)
    return image;

  Image result = image;

  // Calculate kernel size (should be odd)
  int kernelSize = static_cast<int>(2 * std::ceil(3 * sigma) + 1);
  if (kernelSize % 2 == 0)
    kernelSize++;
  int halfKernel = kernelSize / 2;

  // Create Gaussian kernel
  std::vector<double> kernel(kernelSize);
  double sum = 0.0;
  for (int i = 0; i < kernelSize; ++i) {
    int x = i - halfKernel;
    kernel[i] = std::exp(-(x * x) / (2 * sigma * sigma));
    sum += kernel[i];
  }

  // Normalize kernel
  for (double &k : kernel) {
    k /= sum;
  }

  // Apply horizontal blur
  Image temp = image;
#pragma omp parallel for
  for (int y = 0; y < image.height; ++y) {
    for (int x = 0; x < image.width; ++x) {
      double blurredValue = 0.0;
      for (int k = 0; k < kernelSize; ++k) {
        int sourceX = x + k - halfKernel;
        sourceX = std::max(0, std::min(sourceX, image.width - 1));
        blurredValue += image.pixels[y][sourceX] * kernel[k];
      }
      temp.pixels[y][x] = static_cast<int>(std::round(blurredValue));
    }
  }

  // Apply vertical blur
#pragma omp parallel for
  for (int y = 0; y < image.height; ++y) {
    for (int x = 0; x < image.width; ++x) {
      double blurredValue = 0.0;
      for (int k = 0; k < kernelSize; ++k) {
        int sourceY = y + k - halfKernel;
        sourceY = std::max(0, std::min(sourceY, image.height - 1));
        blurredValue += temp.pixels[sourceY][x] * kernel[k];
      }
      result.pixels[y][x] = static_cast<int>(std::round(blurredValue));
    }
  }

  return result;
}

bool ObloidDetector::ValidateCircleGeometry(const std::vector<Point> &contour, 
                                            const Point &center, int radius) const {
  if (contour.empty() || radius <= 0)
    return false;

  // Check if radius is within acceptable range
  if (radius < minRadius_ || radius > maxRadius_)
    return false;

  // Validate that contour points are reasonably distributed around the circle
  int inliers = 0;
  double tolerance = std::max(3.0, radius * 0.15); // 15% tolerance or minimum 3 pixels

  for (const auto &point : contour) {
    double dx = point.x - center.x;
    double dy = point.y - center.y;
    double distance = std::sqrt(dx * dx + dy * dy);
    
    if (std::abs(distance - radius) <= tolerance) {
      inliers++;
    }
  }

  // At least 70% of contour points should fit the circle
  return (static_cast<double>(inliers) / contour.size()) >= 0.7;
}

double ObloidDetector::CalculateCircleFitError(const std::vector<Point> &contour, 
                                               const Point &center, int radius) const {
  if (contour.empty())
    return std::numeric_limits<double>::max();

  double totalError = 0.0;
  for (const auto &point : contour) {
    double dx = point.x - center.x;
    double dy = point.y - center.y;
    double distance = std::sqrt(dx * dx + dy * dy);
    double error = std::abs(distance - radius);
    totalError += error;
  }

  return totalError / contour.size();
}

Obloid ObloidDetector::FitCircleToContour(const std::vector<Point> &contour) const {
  Obloid obloid;
  
  if (contour.size() < 3) {
    obloid.center = Point(0, 0);
    obloid.radius = 0;
    obloid.confidence = 0.0;
    return obloid;
  }

  // Use least squares circle fitting (simplified Kasa method)
  double sumX = 0, sumY = 0, sumX2 = 0, sumY2 = 0, sumXY = 0;
  double sumX3 = 0, sumY3 = 0, sumX2Y = 0, sumXY2 = 0;
  
  for (const auto &point : contour) {
    double x = point.x;
    double y = point.y;
    double x2 = x * x;
    double y2 = y * y;
    
    sumX += x;
    sumY += y;
    sumX2 += x2;
    sumY2 += y2;
    sumXY += x * y;
    sumX3 += x2 * x;
    sumY3 += y2 * y;
    sumX2Y += x2 * y;
    sumXY2 += x * y2;
  }
  
  int n = contour.size();
  
  // Set up matrix for least squares solution
  double A = 2 * (n * sumX2 - sumX * sumX);
  double B = 2 * (n * sumXY - sumX * sumY);
  double C = 2 * (n * sumY2 - sumY * sumY);
  double D = n * (sumX3 + sumXY2) - sumX * (sumX2 + sumY2);
  double E = n * (sumY3 + sumX2Y) - sumY * (sumX2 + sumY2);
  
  double det = A * C - B * B;
  
  if (std::abs(det) < EPSILON_TOLERANCE) {
    // Fallback to centroid-based estimation
    obloid.center = CalculateCentroid(contour);
    obloid.radius = EstimateRadius(contour, obloid.center);
  } else {
    // Solve for circle center
    double centerX = (D * C - E * B) / det;
    double centerY = (A * E - B * D) / det;
    
    obloid.center = Point(static_cast<int>(std::round(centerX)),
                          static_cast<int>(std::round(centerY)));
    obloid.radius = EstimateRadius(contour, obloid.center);
  }
  
  // Calculate confidence
  double fitError = CalculateCircleFitError(contour, obloid.center, obloid.radius);
  obloid.confidence = std::max(0.0, 1.0 - fitError / std::max(1.0, static_cast<double>(obloid.radius)));
  
  return obloid;
}

// SphereDetector implementation - adapting ObloidDetector methods for Sphere
SphereDetector::SphereDetector()
    : minRadius_(10), maxRadius_(100), circularityThreshold_(0.8), confidenceThreshold_(0.7) {
  distanceCache_.reserve(1000);
  radiusCache_.reserve(100);
}

SphereDetector::~SphereDetector() {}

void SphereDetector::SetMinRadius(int minRadius) { minRadius_ = minRadius; }
void SphereDetector::SetMaxRadius(int maxRadius) { maxRadius_ = maxRadius; }
void SphereDetector::SetCircularityThreshold(double threshold) { circularityThreshold_ = threshold; }
void SphereDetector::SetConfidenceThreshold(double threshold) { confidenceThreshold_ = threshold; }

std::vector<Sphere> SphereDetector::DetectSpheres(const Image &image) {
  // For simplicity, convert Obloid results to Sphere
  ObloidDetector obloidDetector;
  obloidDetector.SetMinRadius(minRadius_);
  obloidDetector.SetMaxRadius(maxRadius_);
  obloidDetector.SetCircularityThreshold(circularityThreshold_);
  obloidDetector.SetConfidenceThreshold(confidenceThreshold_);
  
  std::vector<Obloid> obloids = obloidDetector.DetectObloids(image);
  std::vector<Sphere> spheres;
  
  for (const auto& obloid : obloids) {
    Sphere sphere;
    sphere.center = obloid.center;
    sphere.radius = obloid.radius;
    sphere.confidence = obloid.confidence;
    spheres.push_back(sphere);
  }
  
  return spheres;
}