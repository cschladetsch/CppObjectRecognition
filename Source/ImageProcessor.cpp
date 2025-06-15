#include "ShapeDetector/ImageProcessor.hpp"
#include "Utils.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <omp.h>
#include <random>
#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Image ImageProcessor::LoadPGMImage(const std::string &filepath) {
  std::ifstream file(filepath, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Cannot open file: " << filepath << std::endl;
    return Image(0, 0);
  }

  std::string format;
  file >> format;

  if (format != "P5") {
    std::cerr << "Unsupported format, expected PGM P5: " << format << std::endl;
    return Image(0, 0);
  }

  int width, height, maxval;
  file >> width >> height >> maxval;
  file.ignore(); // Skip whitespace after header

  Image image(width, height);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      unsigned char pixel;
      file.read((char *)&pixel, 1);
      image.pixels[y][x] = static_cast<int>(pixel);
    }
  }

  return image;
}

void ImageProcessor::SavePGMImage(const Image &image,
                                  const std::string &filepath) {
  std::ofstream file(filepath, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Cannot create file: " << filepath << std::endl;
    return;
  }

  file << "P5\n";
  file << image.width << " " << image.height << "\n";
  file << "255\n"; // Maximum grey value

  for (int y = 0; y < image.height; ++y) {
    for (int x = 0; x < image.width; ++x) {
      unsigned char pixel = static_cast<unsigned char>(image.pixels[y][x]);
      file.write((char *)&pixel, 1);
    }
  }
}

void ImageProcessor::SavePPMImage(const ColorImage &image,
                                  const std::string &filepath) {
  std::ofstream file(filepath, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Cannot create file: " << filepath << std::endl;
    return;
  }

  file << "P6\n";
  file << image.width << " " << image.height << "\n";
  file << "255\n"; // Maximum color value

  for (int y = 0; y < image.height; ++y) {
    for (int x = 0; x < image.width; ++x) {
      const ColorPixel &pixel = image.pixels[y][x];
      file.write((char *)&pixel.r, 1);
      file.write((char *)&pixel.g, 1);
      file.write((char *)&pixel.b, 1);
    }
  }
}

void ImageProcessor::SavePNGImage(const ColorImage &image,
                                  const std::string &filepath) {
  // First save as PPM, then convert to PNG using system command
  std::string tempPPM = filepath + ".temp.ppm";
  SavePPMImage(image, tempPPM);

  // Use ImageMagick convert command to convert PPM to PNG
  std::string command = "convert " + tempPPM + " " + filepath + " 2>/dev/null";
  int result = system(command.c_str());

  // Clean up temporary PPM file
  std::string cleanup = "rm -f " + tempPPM;
  system(cleanup.c_str());

  if (result != 0) {
    std::cerr
        << "Warning: PNG conversion failed, you may need to install ImageMagick"
        << std::endl;
    std::cerr << "Install with: sudo apt update && sudo apt install imagemagick"
              << std::endl;
  }
}

ColorImage
ImageProcessor::CreateColorImage(const Image &grayImage,
                                 const std::vector<Rectangle> &rectangles) {
  ColorImage colorImage(grayImage.width, grayImage.height);

  // Convert grayscale to color (preserve all original pixel values)
#pragma omp parallel for
  for (int y = 0; y < grayImage.height; ++y) {
    for (int x = 0; x < grayImage.width; ++x) {
      unsigned char grayValue =
          static_cast<unsigned char>(grayImage.pixels[y][x]);
      // Show the original image as-is in grayscale
      colorImage.pixels[y][x] = ColorPixel(grayValue, grayValue, grayValue);
    }
  }

  // Draw red rectangle boundaries
  for (const auto &rect : rectangles) {
    std::vector<Point> corners = GenerateRectangleCorners(rect);

    // Draw thick lines between consecutive corners in red (4x thickness)
    for (size_t i = 0; i < 4; ++i) {
      Point p1 = corners[i];
      Point p2 = corners[(i + 1) % 4];

      DrawThickColorLine(colorImage, p1, p2, ColorPixel(255, 0, 0),
                         4); // Red color, 4x thickness
    }
  }

  return colorImage;
}

Image ImageProcessor::ApplyThreshold(const Image &image, int threshold) {
  Image result = image;

#pragma omp parallel for
  for (int y = 0; y < result.height; ++y) {
    for (int x = 0; x < result.width; ++x) {
      result.pixels[y][x] = (result.pixels[y][x] > threshold) ? 255 : 0;
    }
  }

  return result;
}

Image ImageProcessor::ApplyGaussianBlur(const Image &image, int kernelSize) {
  if (kernelSize % 2 == 0)
    kernelSize++;

  std::vector<std::vector<double>> kernel = CreateGaussianKernel(kernelSize);
  Image result(image.width, image.height);

  int halfKernel = kernelSize / 2;

#pragma omp parallel for
  for (int y = halfKernel; y < image.height - halfKernel; ++y) {
    for (int x = halfKernel; x < image.width - halfKernel; ++x) {
      double sum = 0.0;

      for (int ky = -halfKernel; ky <= halfKernel; ++ky) {
        for (int kx = -halfKernel; kx <= halfKernel; ++kx) {
          sum += image.pixels[y + ky][x + kx] *
                 kernel[ky + halfKernel][kx + halfKernel];
        }
      }

      result.pixels[y][x] = static_cast<int>(sum);
    }
  }

  return result;
}

void ImageProcessor::DrawRectangles(Image &image,
                                    const std::vector<Rectangle> &rectangles) {
  for (const auto &rect : rectangles) {
    // Generate corners from center, width, height, and angle
    std::vector<Point> corners = GenerateRectangleCorners(rect);

    // Draw lines between consecutive corners
    for (size_t i = 0; i < 4; ++i) {
      Point p1 = corners[i];
      Point p2 = corners[(i + 1) % 4];

      DrawLine(image, p1, p2);
    }
  }
}

std::vector<Point>
ImageProcessor::GenerateRectangleCorners(const Rectangle &rect) {
  std::vector<Point> corners;

  // rect.angle is already in radians
  double cosAngle = std::cos(rect.angle);
  double sinAngle = std::sin(rect.angle);

  // Half dimensions
  double halfW = rect.width / 2.0;
  double halfH = rect.height / 2.0;

  // Generate 4 corners relative to center
  std::vector<std::pair<double, double>> relativeCorners = {
      {-halfW, -halfH}, // Top-left
      {halfW, -halfH},  // Top-right
      {halfW, halfH},   // Bottom-right
      {-halfW, halfH}   // Bottom-left
  };

  // Rotate and translate to world coordinates
  for (const auto &corner : relativeCorners) {
    double rotatedX =
        corner.first * cosAngle - corner.second * sinAngle + rect.center.x;
    double rotatedY =
        corner.first * sinAngle + corner.second * cosAngle + rect.center.y;
    corners.push_back(
        Point(static_cast<int>(rotatedX), static_cast<int>(rotatedY)));
  }

  return corners;
}

std::vector<Point>
ImageProcessor::CleanupRectangleCorners(const std::vector<Point> &corners) {
  if (corners.empty())
    return corners;

  std::vector<Point> cleaned;
  const double minDistance = 3.0; // Minimum distance between corners

  for (const auto &corner : corners) {
    bool keepPoint = true;

    // Check if this point is too close to any already kept point
    for (const auto &kept : cleaned) {
      double dx = corner.x - kept.x;
      double dy = corner.y - kept.y;
      double dist = std::sqrt(dx * dx + dy * dy);

      if (dist < minDistance) {
        keepPoint = false;
        break;
      }
    }

    if (keepPoint) {
      cleaned.push_back(corner);
    }
  }

  return cleaned;
}

void ImageProcessor::DrawLine(Image &image, const Point &p1, const Point &p2) {
  // Bresenham's line algorithm
  int dx = std::abs(p2.x - p1.x);
  int dy = std::abs(p2.y - p1.y);
  int sx = (p1.x < p2.x) ? 1 : -1;
  int sy = (p1.y < p2.y) ? 1 : -1;
  int err = dx - dy;

  int x = p1.x, y = p1.y;

  while (true) {
    if (x >= 0 && x < image.width && y >= 0 && y < image.height) {
      image.pixels[y][x] = 255;
    }

    if (x == p2.x && y == p2.y)
      break;

    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x += sx;
    }
    if (e2 < dx) {
      err += dx;
      y += sy;
    }
  }
}

void ImageProcessor::DrawColorLine(ColorImage &image, const Point &p1,
                                   const Point &p2, const ColorPixel &color) {
  // Bresenham's line algorithm for color images
  int dx = std::abs(p2.x - p1.x);
  int dy = std::abs(p2.y - p1.y);
  int sx = (p1.x < p2.x) ? 1 : -1;
  int sy = (p1.y < p2.y) ? 1 : -1;
  int err = dx - dy;

  int x = p1.x, y = p1.y;

  while (true) {
    if (x >= 0 && x < image.width && y >= 0 && y < image.height) {
      image.pixels[y][x] = color;
    }

    if (x == p2.x && y == p2.y)
      break;

    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x += sx;
    }
    if (e2 < dx) {
      err += dx;
      y += sy;
    }
  }
}

void ImageProcessor::DrawThickColorLine(ColorImage &image, const Point &p1,
                                        const Point &p2,
                                        const ColorPixel &color,
                                        int thickness) {
  // Draw multiple parallel lines to create thickness
  int halfThickness = thickness / 2;

  // Calculate line direction vector
  int dx = p2.x - p1.x;
  int dy = p2.y - p1.y;
  double length = std::sqrt(dx * dx + dy * dy);

  if (length == 0) {
    // Single point, draw a thick point
    for (int offsetY = -halfThickness; offsetY <= halfThickness; ++offsetY) {
      for (int offsetX = -halfThickness; offsetX <= halfThickness; ++offsetX) {
        int x = p1.x + offsetX;
        int y = p1.y + offsetY;
        if (x >= 0 && x < image.width && y >= 0 && y < image.height) {
          image.pixels[y][x] = color;
        }
      }
    }
    return;
  }

  // Normalized perpendicular vector
  double perpX = -dy / length;
  double perpY = dx / length;

  // Draw parallel lines
  for (int offset = -halfThickness; offset <= halfThickness; ++offset) {
    Point newP1, newP2;
    newP1.x = static_cast<int>(p1.x + offset * perpX);
    newP1.y = static_cast<int>(p1.y + offset * perpY);
    newP2.x = static_cast<int>(p2.x + offset * perpX);
    newP2.y = static_cast<int>(p2.y + offset * perpY);

    DrawColorLine(image, newP1, newP2, color);
  }
}

Image ImageProcessor::CreateTestImage(int width, int height) {
  Image image(width, height);

  // Fill with black background
#pragma omp parallel for
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      image.pixels[y][x] = 0;
    }
  }

  // Modern C++ random number generation
  static std::random_device rd;
  static std::mt19937 gen(rd());

  // Generate non-overlapping rectangles
  struct RectSpec {
    int centerX, centerY, width, height;
    double angle;
  };

  std::vector<RectSpec> rectangles;
  std::uniform_int_distribution<int> numRectDist(3, 5); // 3-5 rectangles
  int numRectangles = numRectDist(gen);
  int maxAttempts = 20; // Maximum attempts to place each rectangle

  std::uniform_int_distribution<int> widthDist(40, 99);  // 40-99 pixels
  std::uniform_int_distribution<int> heightDist(25, 64); // 25-64 pixels
  std::uniform_real_distribution<double> angleDist(
      -180.0, 180.0); // -180 to 180 degrees

  for (int i = 0; i < numRectangles; ++i) {
    bool placed = false;

    for (int attempt = 0; attempt < maxAttempts && !placed; ++attempt) {
      // Random dimensions
      int rectWidth = widthDist(gen);
      int rectHeight = heightDist(gen);

      // For rotated rectangles, use diagonal as margin to prevent clipping
      double diagonal =
          std::sqrt(rectWidth * rectWidth + rectHeight * rectHeight);
      int margin = static_cast<int>(diagonal / 2) + 30; // Diagonal-based margin

      // Ensure margin doesn't exceed image boundaries
      margin = std::min(margin, width / 3);
      margin = std::min(margin, height / 3);

      // Random position within valid bounds
      std::uniform_int_distribution<int> xDist(margin, width - margin - 1);
      std::uniform_int_distribution<int> yDist(margin, height - margin - 1);
      int centerX = xDist(gen);
      int centerY = yDist(gen);

      // Random angle
      double angle = angleDist(gen);

      // Check for overlap with existing rectangles
      bool overlaps = false;
      for (const auto &existing : rectangles) {
        // Simple distance-based check with generous margin
        int dx = centerX - existing.centerX;
        int dy = centerY - existing.centerY;
        double distance = std::sqrt(dx * dx + dy * dy);
        double minDistance =
            (rectWidth + rectHeight + existing.width + existing.height) / 2 +
            30;

        if (distance < minDistance) {
          overlaps = true;
          break;
        }
      }

      if (!overlaps) {
        rectangles.push_back({centerX, centerY, rectWidth, rectHeight, angle});
        placed = true;
      }
    }
  }

  // Draw all non-overlapping rectangles
  for (const auto &rect : rectangles) {
    CreateRotatedRectangle(image, rect.centerX, rect.centerY, rect.width,
                           rect.height, rect.angle);
  }

  return image;
}

void ImageProcessor::CreateRotatedRectangle(Image &image, int centerX,
                                            int centerY, int rectWidth,
                                            int rectHeight,
                                            double angleRadians) {
  double angleRad = angleRadians;
  double cosAngle = std::cos(angleRad);
  double sinAngle = std::sin(angleRad);

  // Define rectangle corners relative to center
  std::vector<std::pair<double, double>> corners = {
      {-rectWidth / 2.0, -rectHeight / 2.0}, // Top-left
      {rectWidth / 2.0, -rectHeight / 2.0},  // Top-right
      {rectWidth / 2.0, rectHeight / 2.0},   // Bottom-right
      {-rectWidth / 2.0, rectHeight / 2.0}   // Bottom-left
  };

  // Rotate and translate corners
  std::vector<std::pair<int, int>> rotatedCorners;
  for (const auto &corner : corners) {
    double rotatedX =
        corner.first * cosAngle - corner.second * sinAngle + centerX;
    double rotatedY =
        corner.first * sinAngle + corner.second * cosAngle + centerY;
    rotatedCorners.push_back(
        {static_cast<int>(rotatedX), static_cast<int>(rotatedY)});
  }

  // Fill the rotated rectangle using scan line algorithm
  FillRotatedRectangle(image, rotatedCorners);
}

void ImageProcessor::FillRotatedRectangle(
    Image &image, const std::vector<std::pair<int, int>> &corners) {
  if (corners.size() != 4)
    return;

  // Find bounding box
  int minX = corners[0].first, maxX = corners[0].first;
  int minY = corners[0].second, maxY = corners[0].second;

  for (const auto &corner : corners) {
    minX = std::min(minX, corner.first);
    maxX = std::max(maxX, corner.first);
    minY = std::min(minY, corner.second);
    maxY = std::max(maxY, corner.second);
  }

  // Ensure bounds are within image
  minX = std::max(0, minX);
  minY = std::max(0, minY);
  maxX = std::min(image.width - 1, maxX);
  maxY = std::min(image.height - 1, maxY);

  // For each point in bounding box, check if it's inside the rectangle
#pragma omp parallel for
  for (int y = minY; y <= maxY; ++y) {
    for (int x = minX; x <= maxX; ++x) {
      if (IsPointInPolygon(x, y, corners)) {
        image.pixels[y][x] = 255;
      }
    }
  }
}

bool ImageProcessor::IsPointInPolygon(
    int x, int y, const std::vector<std::pair<int, int>> &polygon) {
  int n = polygon.size();
  bool inside = false;

  for (int i = 0, j = n - 1; i < n; j = i++) {
    int xi = polygon[i].first, yi = polygon[i].second;
    int xj = polygon[j].first, yj = polygon[j].second;

    if (((yi > y) != (yj > y)) && (x < (xj - xi) * (y - yi) / (yj - yi) + xi)) {
      inside = !inside;
    }
  }

  return inside;
}

std::vector<std::vector<double>>
ImageProcessor::CreateGaussianKernel(int size) {
  std::vector<std::vector<double>> kernel(size, std::vector<double>(size));
  double sigma = size / 3.0;
  double sum = 0.0;
  int center = size / 2;

  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) {
      int x = i - center;
      int y = j - center;
      kernel[i][j] = std::exp(-(x * x + y * y) / (2 * sigma * sigma));
      sum += kernel[i][j];
    }
  }

  for (int i = 0; i < size; ++i) {
    for (int j = 0; j < size; ++j) {
      kernel[i][j] /= sum;
    }
  }

  return kernel;
}

void ImageProcessor::DrawCircle(Image &image, int centerX, int centerY,
                                int radius, int color) {
  // Bresenham's circle algorithm
  int x = radius;
  int y = 0;
  int err = 0;

  while (x >= y) {
    // Draw 8 octants
    if (centerX + x >= 0 && centerX + x < image.width && centerY + y >= 0 &&
        centerY + y < image.height)
      image.pixels[centerY + y][centerX + x] = color;
    if (centerX + y >= 0 && centerX + y < image.width && centerY + x >= 0 &&
        centerY + x < image.height)
      image.pixels[centerY + x][centerX + y] = color;
    if (centerX - y >= 0 && centerX - y < image.width && centerY + x >= 0 &&
        centerY + x < image.height)
      image.pixels[centerY + x][centerX - y] = color;
    if (centerX - x >= 0 && centerX - x < image.width && centerY + y >= 0 &&
        centerY + y < image.height)
      image.pixels[centerY + y][centerX - x] = color;
    if (centerX - x >= 0 && centerX - x < image.width && centerY - y >= 0 &&
        centerY - y < image.height)
      image.pixels[centerY - y][centerX - x] = color;
    if (centerX - y >= 0 && centerX - y < image.width && centerY - x >= 0 &&
        centerY - x < image.height)
      image.pixels[centerY - x][centerX - y] = color;
    if (centerX + y >= 0 && centerX + y < image.width && centerY - x >= 0 &&
        centerY - x < image.height)
      image.pixels[centerY - x][centerX + y] = color;
    if (centerX + x >= 0 && centerX + x < image.width && centerY - y >= 0 &&
        centerY - y < image.height)
      image.pixels[centerY - y][centerX + x] = color;

    if (err <= 0) {
      y += 1;
      err += 2 * y + 1;
    }
    if (err > 0) {
      x -= 1;
      err -= 2 * x + 1;
    }
  }
}

void ImageProcessor::DrawFilledCircle(Image &image, int centerX, int centerY,
                                      int radius, int color) {
#pragma omp parallel for
  for (int y = -radius; y <= radius; y++) {
    for (int x = -radius; x <= radius; x++) {
      if (x * x + y * y <= radius * radius) {
        int px = centerX + x;
        int py = centerY + y;
        if (px >= 0 && px < image.width && py >= 0 && py < image.height) {
          image.pixels[py][px] = color;
        }
      }
    }
  }
}

void ImageProcessor::DrawTriangle(Image &image, const Point &p1,
                                  const Point &p2, const Point &p3, int color) {
  DrawLine(image, p1, p2);
  DrawLine(image, p2, p3);
  DrawLine(image, p3, p1);
}

void ImageProcessor::DrawFilledTriangle(Image &image, const Point &p1,
                                        const Point &p2, const Point &p3,
                                        int color) {
  // Get bounding box
  int minX = std::min(std::min(p1.x, p2.x), p3.x);
  int maxX = std::max(std::max(p1.x, p2.x), p3.x);
  int minY = std::min(std::min(p1.y, p2.y), p3.y);
  int maxY = std::max(std::max(p1.y, p2.y), p3.y);

  // Clamp to image bounds
  minX = Clamp(minX, 0, image.width - 1);
  maxX = Clamp(maxX, 0, image.width - 1);
  minY = Clamp(minY, 0, image.height - 1);
  maxY = Clamp(maxY, 0, image.height - 1);

  // Barycentric coordinates
#pragma omp parallel for
  for (int y = minY; y <= maxY; y++) {
    for (int x = minX; x <= maxX; x++) {
      // Calculate barycentric coordinates
      double denominator =
          ((p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y));
      if (std::abs(denominator) < 1e-10)
        continue;

      double a = ((p2.y - p3.y) * (x - p3.x) + (p3.x - p2.x) * (y - p3.y)) /
                 denominator;
      double b = ((p3.y - p1.y) * (x - p3.x) + (p1.x - p3.x) * (y - p3.y)) /
                 denominator;
      double c = 1 - a - b;

      // Check if point is inside triangle
      if (a >= 0 && b >= 0 && c >= 0) {
        image.pixels[y][x] = color;
      }
    }
  }
}

void ImageProcessor::DrawEllipse(Image &image, int centerX, int centerY,
                                 int radiusX, int radiusY, double angle,
                                 int color) {
  // angle is in radians
  double cosA = std::cos(angle);
  double sinA = std::sin(angle);

  // Draw ellipse using parametric form
  for (double t = 0; t < 2 * M_PI; t += 0.01) {
    double x = radiusX * std::cos(t);
    double y = radiusY * std::sin(t);

    // Rotate
    double rotX = x * cosA - y * sinA;
    double rotY = x * sinA + y * cosA;

    // Translate
    int px = static_cast<int>(centerX + rotX);
    int py = static_cast<int>(centerY + rotY);

    if (px >= 0 && px < image.width && py >= 0 && py < image.height) {
      image.pixels[py][px] = color;
    }
  }
}

void ImageProcessor::DrawFilledEllipse(Image &image, int centerX, int centerY,
                                       int radiusX, int radiusY, double angle,
                                       int color) {
  // angle is in radians
  double cosA = std::cos(angle);
  double sinA = std::sin(angle);

  // Get bounding box
  int maxRadius = std::max(radiusX, radiusY);

#pragma omp parallel for
  for (int y = -maxRadius; y <= maxRadius; y++) {
    for (int x = -maxRadius; x <= maxRadius; x++) {
      // Reverse rotate the point
      double rotX = x * cosA + y * sinA;
      double rotY = -x * sinA + y * cosA;

      // Check if point is inside ellipse
      if ((rotX * rotX) / (radiusX * radiusX) +
              (rotY * rotY) / (radiusY * radiusY) <=
          1.0) {
        int px = centerX + x;
        int py = centerY + y;
        if (px >= 0 && px < image.width && py >= 0 && py < image.height) {
          image.pixels[py][px] = color;
        }
      }
    }
  }
}

Image ImageProcessor::CreateTestImageWithMixedShapes(int width, int height) {
  Image image(width, height);

  // Black background
#pragma omp parallel for
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      image.pixels[y][x] = 0;
    }
  }

  // Add some rectangles (angles in radians)
  CreateRotatedRectangle(image, width / 4, height / 4, 80, 60,
                         30 * M_PI / 180.0);
  CreateRotatedRectangle(image, 3 * width / 4, height / 4, 100, 50,
                         -20 * M_PI / 180.0);
  CreateRotatedRectangle(image, width / 2, 3 * height / 4, 70, 70,
                         45 * M_PI / 180.0);

  // Add circles
  DrawFilledCircle(image, width / 3, height / 2, 40, 255);
  DrawCircle(image, 2 * width / 3, height / 2, 50, 255);

  // Add triangles
  Point t1(width / 5, height / 5);
  Point t2(width / 5 + 60, height / 5);
  Point t3(width / 5 + 30, height / 5 - 50);
  DrawFilledTriangle(image, t1, t2, t3, 255);

  Point t4(4 * width / 5, height / 5);
  Point t5(4 * width / 5 + 60, height / 5);
  Point t6(4 * width / 5 + 30, height / 5 + 50);
  DrawTriangle(image, t4, t5, t6, 255);

  // Add ellipses (angles in radians)
  DrawFilledEllipse(image, width / 2, height / 2, 60, 30, 30 * M_PI / 180.0,
                    255);
  DrawEllipse(image, width / 4, 3 * height / 4, 40, 25, -30 * M_PI / 180.0,
              255);

  return image;
}