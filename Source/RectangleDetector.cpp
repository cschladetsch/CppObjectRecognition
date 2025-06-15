#include "ShapeDetector/RectangleDetector.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <queue>
#include <omp.h>
#include <numbers>
#include <unordered_set>
constexpr double MIN_DISTANCE_SQUARED = 1.0;
constexpr double MIN_DISTANCE_SQUARED_LARGE = 64.0;
constexpr double EPSILON_TOLERANCE = 1e-9;
constexpr double RIGHT_ANGLE = std::numbers::pi / 2.0;
constexpr double ANGLE_TOLERANCE = 1.0; // ~57 degrees - tolerant for rotated rectangles

RectangleDetector::RectangleDetector() 
    : minArea_(500.0), maxArea_(10000.0), approxEpsilon_(0.02) {
    // Pre-allocate caches for better performance
    distanceCache_.reserve(1000);
    angleCache_.reserve(100);
}

RectangleDetector::~RectangleDetector() {}

void RectangleDetector::SetMinArea(double minArea) {
    minArea_ = minArea;
}

void RectangleDetector::SetMaxArea(double maxArea) {
    maxArea_ = maxArea;
}

void RectangleDetector::SetApproxEpsilon(double epsilon) {
    approxEpsilon_ = epsilon;
}

std::vector<Rectangle> RectangleDetector::DetectRectangles(const Image& image) {
    std::vector<Rectangle> rectangles;
    rectangles.reserve(50); // More space for multiple detection strategies
    
    // Strategy 1: Standard contour-based detection
    Image processed1 = PreprocessImage(image);
    std::vector<std::vector<Point>> contours1 = FindContours(processed1);
    ProcessContoursAtScale(contours1, rectangles, 1.0, image);
    
    // Strategy 2: Enhanced edge detection for steep angles
    Image processed2 = PreprocessImageEnhanced(image);
    std::vector<std::vector<Point>> contours2 = FindContours(processed2);
    ProcessContoursAtScale(contours2, rectangles, 1.0, image);
    
    // Strategy 3: Morphological operations for broken contours
    Image processed3 = PreprocessImageMorphological(image);
    std::vector<std::vector<Point>> contours3 = FindContours(processed3);
    ProcessContoursAtScale(contours3, rectangles, 1.0, image);
    
    // Strategy 4: Hough line-based rectangle detection for critical angles
    std::vector<Rectangle> houghRects = DetectRectanglesUsingHoughLines(image);
    rectangles.insert(rectangles.end(), houghRects.begin(), houghRects.end());
    
    // Remove duplicates from multiple strategies
    RemoveDuplicateRectangles(rectangles);
    
    return rectangles;
}

void RectangleDetector::ProcessContoursAtScale(
    const std::vector<std::vector<Point>>& contours,
    std::vector<Rectangle>& rectangles,
    double scale,
    const Image& scaledImage) {
    
    // Parallel processing for large number of contours
    if (contours.size() > 10) {
        std::vector<Rectangle> tempRectangles(contours.size());
        std::vector<bool> validRectangles(contours.size(), false);
        
        #pragma omp parallel for schedule(dynamic)
        for (size_t i = 0; i < contours.size(); ++i) {
            if (IsRectangle(contours[i])) {
                Rectangle rect = CreateRectangle(contours[i]);
                // Scale coordinates back to original image size
                if (scale != 1.0) {
                    rect.center.x = static_cast<int>(rect.center.x / scale);
                    rect.center.y = static_cast<int>(rect.center.y / scale);
                    rect.width = static_cast<int>(rect.width / scale);
                    rect.height = static_cast<int>(rect.height / scale);
                }
                if (rect.width > 0 && rect.height > 0) {
                    tempRectangles[i] = rect;
                    validRectangles[i] = true;
                }
            }
        }
        
        // Collect valid rectangles
        for (size_t i = 0; i < contours.size(); ++i) {
            if (validRectangles[i]) {
                rectangles.push_back(tempRectangles[i]);
            }
        }
    } else {
        // Sequential processing for small number of contours
        for (const auto& contour : contours) {
            if (IsRectangle(contour)) {
                Rectangle rect = CreateRectangle(contour);
                // Scale coordinates back to original image size
                if (scale != 1.0) {
                    rect.center.x = static_cast<int>(rect.center.x / scale);
                    rect.center.y = static_cast<int>(rect.center.y / scale);
                    rect.width = static_cast<int>(rect.width / scale);
                    rect.height = static_cast<int>(rect.height / scale);
                }
                if (rect.width > 0 && rect.height > 0) {
                    rectangles.push_back(rect);
                }
            }
        }
    }
}

Image RectangleDetector::PreprocessImage(const Image& image) const {
    Image result = image;
    
    // Apply minimal Gaussian blur for noise reduction
    Image blurred = ApplyGaussianBlur(result, 0.8); // Reduced sigma to preserve edges
    
    // Simple thresholding - keep it simple to avoid losing rectangles
    #pragma omp parallel for
    for (int y = 0; y < blurred.height; ++y) {
        for (int x = 0; x < blurred.width; ++x) {
            result.pixels[y][x] = (blurred.pixels[y][x] > 127) ? 255 : 0;
        }
    }
    
    return result;
}

std::vector<std::vector<Point>> RectangleDetector::FindContours(const Image& image) const {
    std::vector<std::vector<Point>> contours;
    contours.reserve(100);  // Pre-allocate for typical number of contours
    std::vector<std::vector<bool>> visited(image.height, std::vector<bool>(image.width, false));
    
    // Find all connected white regions
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            if (!visited[y][x] && image.pixels[y][x] == 255) {
                std::vector<Point> region;
                region.reserve(1000);  // Pre-allocate for typical region size
                ScanlineFillContour(image, x, y, region, visited);
                
                if (region.size() >= 50) { // Minimum size for a rectangle
                    // Convert filled region to boundary contour
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

void RectangleDetector::ScanlineFillContour(const Image& image, int startX, int startY, 
                                       std::vector<Point>& contour, std::vector<std::vector<bool>>& visited) const {
    // Efficient scanline flood fill algorithm
    std::stack<ScanlineSegment> stack;
    
    // Find initial horizontal segment
    int x1 = startX, x2 = startX;
    while (x1 > 0 && image.pixels[startY][x1-1] == 255 && !visited[startY][x1-1]) x1--;
    while (x2 < image.width-1 && image.pixels[startY][x2+1] == 255 && !visited[startY][x2+1]) x2++;
    
    stack.push(ScanlineSegment(startY, x1, x2, -1));
    
    while (!stack.empty()) {
        ScanlineSegment seg = stack.top();
        stack.pop();
        
        // Process scanline - batch mark visited pixels for better cache performance
        for (int x = seg.x1; x <= seg.x2; x++) {
            if (!visited[seg.y][x]) {
                visited[seg.y][x] = true;
                contour.emplace_back(x, seg.y); // Use emplace_back for better performance
            }
        }
        
        // Check lines above and below
        for (int dir = -1; dir <= 1; dir += 2) {
            int newY = seg.y + dir;
            if (newY < 0 || newY >= image.height) continue;
            
            int x = seg.x1;
            while (x <= seg.x2) {
                // Skip non-white or visited pixels
                while (x <= seg.x2 && (image.pixels[newY][x] != 255 || visited[newY][x])) x++;
                if (x > seg.x2) break;
                
                // Find new segment
                int newX1 = x;
                while (x <= seg.x2 && image.pixels[newY][x] == 255 && !visited[newY][x]) x++;
                int newX2 = x - 1;
                
                // Extend left
                while (newX1 > 0 && image.pixels[newY][newX1-1] == 255 && !visited[newY][newX1-1]) newX1--;
                // Extend right
                while (newX2 < image.width-1 && image.pixels[newY][newX2+1] == 255 && !visited[newY][newX2+1]) newX2++;
                
                stack.push(ScanlineSegment(newY, newX1, newX2, seg.y));
            }
        }
    }
}

bool RectangleDetector::IsRectangle(const std::vector<Point>& contour) const {
    if (contour.size() < 4) return false;
    
    std::vector<Point> approx = ApproximateContour(contour, approxEpsilon_);
    
    
    // Allow 4-6 vertices for rectangles (more tolerance for imperfect shapes)
    if (approx.size() < 4 || approx.size() > 6) return false;
    
    // If we have more than 4 vertices, try to find the best 4 corners
    if (approx.size() > 4) {
        auto corners = SelectBestCorners(approx);
        approx = std::vector<Point>(corners.begin(), corners.end());
        if (approx.size() != 4) return false;
    }
    
    // Check area constraints
    double area = CalculateArea(approx);
    if (area < minArea_ || area > maxArea_) return false;
    
    // Check if it's a valid quadrilateral (parallel sides)
    if (!IsValidQuadrilateral(approx)) return false;
    
    // Additional check: reject shapes that are too circular
    // Calculate the convexity defects to detect circular shapes
    if (IsCircularShape(contour, approx)) return false;
    
    // Additional check: verify corner angles are close to π/2 radians (90 degrees)
    int validCorners = 0;
    double avgAngleDeviation = 0.0;
    
    // Pre-calculate angle deviations for better cache utilization
    double deviations[4];
    for (int i = 0; i < 4; ++i) {
        const int prev = (i + 3) % 4;
        const int next = (i + 1) % 4;
        const double angle = CalculateCornerAngleFast(approx[prev], approx[i], approx[next]);
        
        deviations[i] = std::abs(angle - RIGHT_ANGLE);
        avgAngleDeviation += deviations[i];
        
        // Rectangle corners should be close to π/2 radians
        if (deviations[i] < ANGLE_TOLERANCE) {
            validCorners++;
        }
    }
    avgAngleDeviation *= 0.25; // Divide by 4
    
    // Tolerant but not too permissive for rotated rectangles
    // Allow at least 2 out of 4 corners to be close to 90 degrees
    if (validCorners < 2 || avgAngleDeviation > 0.7) {
        return false;
    }
    
    // Check rectangularity: compare area with bounding box area
    // Optimized bounding box calculation with single pass
    int minX = approx[0].x, maxX = approx[0].x;
    int minY = approx[0].y, maxY = approx[0].y;
    
    for (size_t i = 1; i < approx.size(); ++i) {
        const Point& p = approx[i];
        if (p.x < minX) minX = p.x;
        else if (p.x > maxX) maxX = p.x;
        
        if (p.y < minY) minY = p.y;
        else if (p.y > maxY) maxY = p.y;
    }
    
    const double boundingBoxArea = static_cast<double>(maxX - minX) * (maxY - minY);
    const double rectangularity = area / boundingBoxArea;
    
    
    // For a perfect rectangle, this ratio should be close to 1
    // Very high tolerance for rotated rectangles (45° rotation gives ~0.71)
    if (rectangularity < 0.15) {
        return false;
    }
    
    return true;
}

// Helper function to detect circular shapes
bool RectangleDetector::IsCircularShape(const std::vector<Point>& contour, const std::vector<Point>& approx) const {
    // Calculate the area ratio between the original contour and its convex hull
    double contourArea = CalculateArea(contour);
    double approxArea = CalculateArea(approx);
    
    // For circles approximated as 4-sided polygons, the area ratio will be very different
    if (contourArea > 0 && approxArea > 0) {
        double areaRatio = contourArea / approxArea;
        // If the contour area is much larger than the approximated polygon, it's likely a circle
        if (areaRatio > 1.3) return true;
    }
    
    // Calculate the perimeter-to-area ratio (circularity test)
    double perimeter = CalculatePerimeter(contour);
    if (contourArea > 0 && perimeter > 0) {
        double circularity = (perimeter * perimeter) / (4.0 * std::numbers::pi * contourArea);
        // Perfect circle has circularity = 1, rectangles have higher values
        // If circularity is close to 1, it's likely a circle
        if (circularity < 1.2) return true;
    }
    
    return false;
}

bool RectangleDetector::IsValidQuadrilateral(const std::vector<Point>& quad) const {
    if (quad.size() != 4) return false;
    
    // Pre-calculate side vectors for better cache performance
    double sides[4][2]; // [side][x,y]
    double lengths[4];
    
    // Calculate all sides in one pass
    for (int i = 0; i < 4; ++i) {
        const int next = (i + 1) % 4;
        const double dx = quad[next].x - quad[i].x;
        const double dy = quad[next].y - quad[i].y;
        lengths[i] = std::sqrt(dx * dx + dy * dy);
        
        if (lengths[i] < EPSILON_TOLERANCE) return false; // Degenerate side
        
        const double invLength = 1.0 / lengths[i];
        sides[i][0] = dx * invLength;
        sides[i][1] = dy * invLength;
    }
    
    // Check if opposite sides are roughly parallel (dot product close to ±1)
    const double dot1 = sides[0][0] * sides[2][0] + sides[0][1] * sides[2][1];
    const double dot2 = sides[1][0] * sides[3][0] + sides[1][1] * sides[3][1];
    
    // Allow more tolerance for rotation and imperfect detection
    return (std::abs(std::abs(dot1) - 1.0) < 0.35) && (std::abs(std::abs(dot2) - 1.0) < 0.35);
}

std::vector<Point> RectangleDetector::ApproximateContour(const std::vector<Point>& contour, double epsilon) const {
    if (contour.size() < 4) return contour;
    
    const double perimeter = CalculatePerimeter(contour);
    
    // Try moment-based detection first - completely rotation invariant
    // But only for contours that pass additional shape tests
    if (contour.size() > 20 && !IsLikelyCircularContour(contour)) {
        std::vector<Point> momentApprox = FindRectangleCornersMomentBased(contour);
        if (momentApprox.size() == 4) {
            // Additional validation: check if detected corners make sense
            double area = CalculateArea(momentApprox);
            if (area >= minArea_ && area <= maxArea_) {
                return momentApprox;
            }
        }
    }
    
    // Try Hough-based line detection for steep angles - but only for rectangular-like shapes
    if (contour.size() > 30 && !IsLikelyCircularContour(contour)) {
        std::vector<Point> houghApprox = FindRectangleUsingHoughLines(contour);
        if (houghApprox.size() == 4) {
            return houghApprox;
        }
    }
    
    // Try smoothed contour for better rotation handling
    std::vector<Point> smoothedContour = SmoothContourForRotation(contour);
    
    // Try rotation-invariant corner detection first for larger contours
    if (smoothedContour.size() > 50) {
        std::vector<Point> rotationInvariantApprox = FindCornersRotationInvariant(smoothedContour);
        if (rotationInvariantApprox.size() >= 4 && rotationInvariantApprox.size() <= 8) {
            return rotationInvariantApprox;
        }
    }
    
    // Try multiple epsilon values to find the best 4-corner approximation
    std::vector<double> epsilonMultipliers = {0.05, 0.1, 0.15, 0.2, 0.3, 0.5, 0.8, 1.0, 1.5, 2.0, 3.0};
    
    for (double multiplier : epsilonMultipliers) {
        double epsilonValue = std::max(epsilon * perimeter * multiplier, 2.0);
        
        std::vector<Point> approx;
        approx.reserve(8);
        
        std::vector<bool> keep(contour.size(), false);
        keep[0] = keep[contour.size() - 1] = true;
        
        DouglasPeuckerRecursive(contour, 0, contour.size() - 1, epsilonValue, keep);
        
        for (size_t i = 0; i < contour.size(); ++i) {
            if (keep[i]) {
                approx.push_back(contour[i]);
            }
        }
        
        // If we get exactly 4 corners, that's ideal
        if (approx.size() == 4) {
            return approx;
        }
        
        // If we get 5-12 corners, we can work with that (increased for rotated rectangles)
        if (approx.size() >= 5 && approx.size() <= 12) {
            return approx;
        }
    }
    
    // Fallback: use convex hull approach for difficult cases
    std::vector<Point> hull = ConvexHull(std::vector<Point>(contour));
    if (hull.size() >= 4 && hull.size() <= 8) {
        return hull;
    }
    
    // Final fallback: original algorithm
    std::vector<Point> approx;
    approx.reserve(8);
    double epsilonValue = std::max(epsilon * perimeter, 3.0);
    
    std::vector<bool> keep(contour.size(), false);
    keep[0] = keep[contour.size() - 1] = true;
    
    DouglasPeuckerRecursive(contour, 0, contour.size() - 1, epsilonValue, keep);
    
    for (size_t i = 0; i < contour.size(); ++i) {
        if (keep[i]) {
            approx.push_back(contour[i]);
        }
    }
    
    return approx;
}

void RectangleDetector::DouglasPeuckerRecursive(const std::vector<Point>& contour, int start, int end,
                                              double epsilon, std::vector<bool>& keep) const {
    if (end - start <= 1) return;
    
    double maxDist = 0.0;
    int maxIndex = start;
    
    for (int i = start + 1; i < end; ++i) {
        double distSquared = PointToLineDistanceSquared(contour[i], contour[start], contour[end]);
        if (distSquared > maxDist) {
            maxDist = distSquared;
            maxIndex = i;
        }
    }
    
    if (maxDist > epsilon * epsilon) {
        keep[maxIndex] = true;
        DouglasPeuckerRecursive(contour, start, maxIndex, epsilon, keep);
        DouglasPeuckerRecursive(contour, maxIndex, end, epsilon, keep);
    }
}

double RectangleDetector::CalculatePerimeter(const std::vector<Point>& contour) const {
    if (contour.size() < 2) return 0.0;
    
    double perimeter = 0.0;
    const size_t n = contour.size();
    
    // Unroll small loops for better performance
    if (n == 4) {
        for (int i = 0; i < 4; ++i) {
            const int j = (i + 1) % 4;
            const double dx = contour[j].x - contour[i].x;
            const double dy = contour[j].y - contour[i].y;
            perimeter += std::sqrt(dx * dx + dy * dy);
        }
    } else {
        for (size_t i = 0; i < n; ++i) {
            const size_t j = (i + 1) % n;
            const double dx = contour[j].x - contour[i].x;
            const double dy = contour[j].y - contour[i].y;
            perimeter += std::sqrt(dx * dx + dy * dy);
        }
    }
    return perimeter;
}

double RectangleDetector::CalculateArea(const std::vector<Point>& contour) const {
    if (contour.size() < 3) return 0.0;
    
    double area = 0.0;
    const size_t n = contour.size();
    
    // Optimized for common case of quadrilaterals
    if (n == 4) {
        area = static_cast<double>(contour[0].x) * contour[1].y - static_cast<double>(contour[1].x) * contour[0].y +
               static_cast<double>(contour[1].x) * contour[2].y - static_cast<double>(contour[2].x) * contour[1].y +
               static_cast<double>(contour[2].x) * contour[3].y - static_cast<double>(contour[3].x) * contour[2].y +
               static_cast<double>(contour[3].x) * contour[0].y - static_cast<double>(contour[0].x) * contour[3].y;
    } else {
        for (size_t i = 0; i < n; ++i) {
            const size_t j = (i + 1) % n;
            area += static_cast<double>(contour[i].x) * contour[j].y - static_cast<double>(contour[j].x) * contour[i].y;
        }
    }
    return std::abs(area) * 0.5;
}


double RectangleDetector::PointToLineDistanceSquared(const Point& point, const Point& lineStart, const Point& lineEnd) const {
    const double A = lineEnd.y - lineStart.y;
    const double B = lineStart.x - lineEnd.x;
    const double C = static_cast<double>(lineEnd.x) * lineStart.y - static_cast<double>(lineStart.x) * lineEnd.y;
    
    const double denominatorSquared = A * A + B * B;
    if (denominatorSquared < EPSILON_TOLERANCE) return 0.0;
    
    const double distance = A * point.x + B * point.y + C;
    return (distance * distance) / denominatorSquared;
}

Rectangle RectangleDetector::CreateRectangle(const std::vector<Point>& contour) const {
    Rectangle rect;
    
    std::vector<Point> approx = ApproximateContour(contour, approxEpsilon_);
    
    // Clean up the approximation - remove duplicate points
    std::vector<Point> cleanCorners = CleanupCorners(approx);
    
    
    // Try to form a proper rectangle with 4 corners
    if (cleanCorners.size() < 3) {
        // Not enough points for any polygon, reject
        return rect;
    } else if (cleanCorners.size() == 3) {
        return rect;
    } else if (cleanCorners.size() > 4) {
        auto bestCorners = SelectBestCorners(cleanCorners);
        cleanCorners.clear();
        for (const auto& corner : bestCorners) {
            if (corner.x != 0 || corner.y != 0) {  // Check for valid corner
                cleanCorners.push_back(corner);
            }
        }
        if (cleanCorners.size() != 4) {
            return rect;
        }
    }
    
    if (cleanCorners.size() == 4) {
        // Calculate center using contour centroid for better accuracy with rotated rectangles
        Point centroid = CalculateContourCentroid(contour);
        rect.center = centroid;
        
        std::vector<double> edgeLengths;
        edgeLengths.reserve(4);
        std::vector<std::pair<double, double>> edgeVectors;
        edgeVectors.reserve(4);
        
        for (int i = 0; i < 4; ++i) {
            const int nextIdx = (i + 1) % 4;
            const double dx = cleanCorners[nextIdx].x - cleanCorners[i].x;
            const double dy = cleanCorners[nextIdx].y - cleanCorners[i].y;
            const double length = std::sqrt(dx * dx + dy * dy);
            
            edgeLengths.push_back(length);
            if (length > 0) {
                edgeVectors.push_back({dx / length, dy / length});
            } else {
                edgeVectors.push_back({0, 0});
            }
        }
        
        // For a rectangle, opposite edges should be equal
        // We need to find which edges are the width and height
        // Compare edge 0 with edge 2 (opposite) and edge 1 with edge 3 (opposite)
        double avgLength1 = (edgeLengths[0] + edgeLengths[2]) / 2.0;
        double avgLength2 = (edgeLengths[1] + edgeLengths[3]) / 2.0;
        
        if (avgLength1 > avgLength2) {
            rect.width = static_cast<int>(avgLength1);
            rect.height = static_cast<int>(avgLength2);
            rect.angle = std::atan2(edgeVectors[0].second, edgeVectors[0].first);
        } else {
            rect.width = static_cast<int>(avgLength2);
            rect.height = static_cast<int>(avgLength1);
            rect.angle = std::atan2(edgeVectors[1].second, edgeVectors[1].first);
        }
    }
    
    return rect;
}

std::vector<Point> RectangleDetector::CleanupCorners(const std::vector<Point>& corners) const {
    const double minDistanceSquared = corners.size() <= 4 ? MIN_DISTANCE_SQUARED : MIN_DISTANCE_SQUARED_LARGE;
    std::vector<Point> cleaned;
    cleaned.reserve(corners.size());
    
    for (const Point& corner : corners) {
        bool keepPoint = true;
        
        for (const Point& kept : cleaned) {
            const double dx = corner.x - kept.x;
            const double dy = corner.y - kept.y;
            const double distSquared = dx * dx + dy * dy;
            
            if (distSquared < minDistanceSquared) {
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

std::array<Point, 4> RectangleDetector::SelectBestCorners(const std::vector<Point>& corners) const {
    std::array<Point, 4> result = {Point(), Point(), Point(), Point()};
    
    if (corners.size() <= 4) {
        for (size_t i = 0; i < corners.size() && i < 4; ++i) {
            result[i] = corners[i];
        }
        return result;
    }
    
    // Find the convex hull to get the outermost points
    std::vector<Point> hull = ConvexHull(corners);
    
    if (hull.size() == 4) {
        std::copy(hull.begin(), hull.end(), result.begin());
        return result;
    } else if (hull.size() > 4) {
        // If we have more than 4 points in the hull, select the 4 most corner-like
        // by finding points with the largest angle changes
        std::vector<std::pair<double, Point>> angleCorners;
        angleCorners.reserve(hull.size());
        
        for (size_t i = 0; i < hull.size(); ++i) {
            size_t prev = (i - 1 + hull.size()) % hull.size();
            size_t next = (i + 1) % hull.size();
            
            // Calculate angle at this point
            double angle = CalculateCornerAngleFast(hull[prev], hull[i], hull[next]);
            angleCorners.push_back({angle, hull[i]});
        }
        
        // Partial sort to get top 4
        std::partial_sort(angleCorners.begin(), angleCorners.begin() + 4, angleCorners.end(),
                         [](const auto& a, const auto& b) { return a.first > b.first; });
        
        // Take the 4 best corners
        std::vector<Point> bestCorners;
        bestCorners.reserve(4);
        for (int i = 0; i < 4; ++i) {
            bestCorners.push_back(angleCorners[i].second);
        }
        
        // Sort them in proper order around the shape
        auto sorted = SortBoundaryPointsRadix(std::move(bestCorners));
        std::copy(sorted.begin(), sorted.end(), result.begin());
    }
    
    return result;
}

double RectangleDetector::CalculateCornerAngle(const Point& prev, const Point& current, const Point& next) const {
    double dx1 = prev.x - current.x;
    double dy1 = prev.y - current.y;
    double dx2 = next.x - current.x;
    double dy2 = next.y - current.y;
    
    double angle1 = std::atan2(dy1, dx1);
    double angle2 = std::atan2(dy2, dx2);
    
    double angleDiff = std::abs(angle2 - angle1);
    if (angleDiff > std::numbers::pi) angleDiff = 2 * std::numbers::pi - angleDiff;
    
    return angleDiff;
}

double RectangleDetector::CalculateCornerAngleFast(const Point& prev, const Point& current, const Point& next) const {
    double dx1 = prev.x - current.x;
    double dy1 = prev.y - current.y;
    double dx2 = next.x - current.x;
    double dy2 = next.y - current.y;
    
    // Use dot product to calculate angle
    double dot = dx1 * dx2 + dy1 * dy2;
    double len1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
    double len2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
    
    if (len1 < 1e-10 || len2 < 1e-10) return 0.0;
    
    // Angle between vectors
    double cosAngle = dot / (len1 * len2);
    cosAngle = std::max(-1.0, std::min(1.0, cosAngle)); // Clamp to [-1, 1]
    return std::acos(cosAngle);
}

std::vector<Point> RectangleDetector::ExtractBoundary(const std::vector<Point>& region, const Image& image) const {
    std::vector<Point> boundary;
    boundary.reserve(region.size() / 4);  // Typical boundary is ~1/4 of region
    
    // Find all boundary points - pixels that are white but have at least one black neighbor
    for (const Point& p : region) {
        bool isBoundary = false;
        
        // Check 8-connected neighbors
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                
                int nx = p.x + dx;
                int ny = p.y + dy;
                
                if (nx < 0 || nx >= image.width || ny < 0 || ny >= image.height ||
                    image.pixels[ny][nx] == 0) {
                    isBoundary = true;
                    break;
                }
            }
            if (isBoundary) break;
        }
        
        if (isBoundary) {
            boundary.push_back(p);
        }
    }
    
    // Sort boundary points to form a proper contour
    if (boundary.size() > 0) {
        return SortBoundaryPointsRadix(std::move(boundary));
    }
    
    return boundary;
}

std::vector<Point> RectangleDetector::SortBoundaryPointsRadix(std::vector<Point> boundary) const {
    if (boundary.size() < 3) return boundary;
    
    // Find centroid
    int centerX = 0, centerY = 0;
    for (const Point& p : boundary) {
        centerX += p.x;
        centerY += p.y;
    }
    centerX /= boundary.size();
    centerY /= boundary.size();
    
    // Sort points by quadrant and then by angle approximation
    std::sort(boundary.begin(), boundary.end(), [centerX, centerY](const Point& a, const Point& b) {
        int dxa = a.x - centerX;
        int dya = a.y - centerY;
        int dxb = b.x - centerX;
        int dyb = b.y - centerY;
        
        // Determine quadrants
        int qa = (dxa >= 0) ? ((dya >= 0) ? 0 : 3) : ((dya >= 0) ? 1 : 2);
        int qb = (dxb >= 0) ? ((dyb >= 0) ? 0 : 3) : ((dyb >= 0) ? 1 : 2);
        
        if (qa != qb) return qa < qb;
        
        // Same quadrant - use cross product for ordering
        return dxa * dyb > dya * dxb;
    });
    
    return boundary;
}

Point RectangleDetector::CalculateContourCentroid(const std::vector<Point>& contour) const {
    if (contour.empty()) {
        return Point(0, 0);
    }
    
    double area = 0.0;
    double centroidX = 0.0;
    double centroidY = 0.0;
    
    for (size_t i = 0; i < contour.size(); ++i) {
        const size_t j = (i + 1) % contour.size();
        const double cross = contour[i].x * contour[j].y - contour[j].x * contour[i].y;
        area += cross;
        centroidX += (contour[i].x + contour[j].x) * cross;
        centroidY += (contour[i].y + contour[j].y) * cross;
    }
    
    area *= 0.5;
    
    if (std::abs(area) < 1e-6) {
        double sumX = 0.0, sumY = 0.0;
        for (const Point& p : contour) {
            sumX += p.x;
            sumY += p.y;
        }
        const double size = static_cast<double>(contour.size());
        return Point(static_cast<int>(sumX / size), static_cast<int>(sumY / size));
    }
    
    const double factor = 1.0 / (6.0 * area);
    return Point(static_cast<int>(centroidX * factor), static_cast<int>(centroidY * factor));
}

std::vector<Point> RectangleDetector::ConvexHull(std::vector<Point> points) const {
    if (points.size() < 3) return points;
    
    std::vector<Point> sortedPoints = points;
    
    // Sort points lexicographically (by x, then by y)
    std::sort(sortedPoints.begin(), sortedPoints.end(), [](const Point& a, const Point& b) {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
    });
    
    // Build lower hull
    std::vector<Point> lower;
    lower.reserve(sortedPoints.size() / 2);
    for (const auto& p : sortedPoints) {
        while (lower.size() >= 2 && Cross(lower[lower.size()-2], lower[lower.size()-1], p) <= 0) {
            lower.pop_back();
        }
        lower.push_back(p);
    }
    
    // Build upper hull
    std::vector<Point> upper;
    upper.reserve(sortedPoints.size() / 2);
    for (auto it = sortedPoints.rbegin(); it != sortedPoints.rend(); ++it) {
        while (upper.size() >= 2 && Cross(upper[upper.size()-2], upper[upper.size()-1], *it) <= 0) {
            upper.pop_back();
        }
        upper.push_back(*it);
    }
    
    // Remove last point of each half because it's repeated
    lower.pop_back();
    upper.pop_back();
    
    // Concatenate lower and upper hull
    lower.insert(lower.end(), upper.begin(), upper.end());
    
    return lower;
}

double RectangleDetector::Cross(const Point& O, const Point& A, const Point& B) const {
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}

// Rotation-invariant corner detection using curvature analysis
std::vector<Point> RectangleDetector::FindCornersRotationInvariant(const std::vector<Point>& contour) const {
    if (contour.size() < 8) return std::vector<Point>(); // Too few points for reliable curvature analysis
    
    std::vector<Point> corners;
    std::vector<double> curvatures;
    curvatures.reserve(contour.size());
    
    // Calculate curvature at each point
    for (size_t i = 0; i < contour.size(); ++i) {
        double curvature = CalculateCurvature(contour, i, 5); // Larger window for stability
        curvatures.push_back(std::abs(curvature));
    }
    
    // Find local maxima in curvature (potential corners)
    std::vector<std::pair<double, size_t>> curvaturePeaks;
    const int minDistance = static_cast<int>(contour.size() / 12); // Minimum distance between corners
    
    for (size_t i = 0; i < contour.size(); ++i) {
        bool isLocalMax = true;
        const int windowSize = std::max(3, minDistance / 2);
        
        // Check if this is a local maximum
        for (int j = -windowSize; j <= windowSize; ++j) {
            size_t idx = (i + j + contour.size()) % contour.size();
            if (curvatures[idx] > curvatures[i]) {
                isLocalMax = false;
                break;
            }
        }
        
        if (isLocalMax && curvatures[i] > 0.05) { // Lowered threshold for corner detection
            curvaturePeaks.push_back({curvatures[i], i});
        }
    }
    
    // Sort by curvature strength and select the top 4-8 corners
    std::sort(curvaturePeaks.begin(), curvaturePeaks.end(), std::greater<std::pair<double, size_t>>());
    
    // Extract the strongest corner candidates, ensuring minimum distance
    std::vector<size_t> selectedIndices;
    for (const auto& peak : curvaturePeaks) {
        size_t currentIdx = peak.second;
        bool tooClose = false;
        
        // Check minimum distance from already selected corners
        for (size_t selectedIdx : selectedIndices) {
            int distance = std::min(
                static_cast<int>(std::abs(static_cast<int>(currentIdx) - static_cast<int>(selectedIdx))),
                static_cast<int>(contour.size() - std::abs(static_cast<int>(currentIdx) - static_cast<int>(selectedIdx)))
            );
            if (distance < minDistance) {
                tooClose = true;
                break;
            }
        }
        
        if (!tooClose) {
            selectedIndices.push_back(currentIdx);
        }
        
        if (selectedIndices.size() >= 8) break; // Limit to 8 corners maximum
    }
    
    // Sort indices to maintain contour order
    std::sort(selectedIndices.begin(), selectedIndices.end());
    
    // Extract corner points
    for (size_t idx : selectedIndices) {
        corners.push_back(contour[idx]);
    }
    
    return corners;
}

// Calculate curvature at a specific point using discrete approximation
double RectangleDetector::CalculateCurvature(const std::vector<Point>& contour, size_t index, int windowSize) const {
    if (contour.size() < 3 || windowSize < 1) return 0.0;
    
    const size_t n = contour.size();
    const int prevIdx = (index - windowSize + n) % n;
    const int nextIdx = (index + windowSize) % n;
    
    const Point& prev = contour[prevIdx];
    const Point& curr = contour[index];
    const Point& next = contour[nextIdx];
    
    // Calculate vectors
    const double dx1 = curr.x - prev.x;
    const double dy1 = curr.y - prev.y;
    const double dx2 = next.x - curr.x;
    const double dy2 = next.y - curr.y;
    
    // Calculate cross product (measures turn angle)
    const double cross = dx1 * dy2 - dy1 * dx2;
    
    // Calculate segment lengths
    const double len1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
    const double len2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
    
    if (len1 < EPSILON_TOLERANCE || len2 < EPSILON_TOLERANCE) return 0.0;
    
    // Curvature approximation: cross product normalized by average segment length
    return cross / ((len1 + len2) * 0.5);
}

// Smooth contour to reduce staircase effects from pixel discretization
std::vector<Point> RectangleDetector::SmoothContourForRotation(const std::vector<Point>& contour) const {
    if (contour.size() < 3) return contour;
    
    std::vector<Point> smoothed;
    smoothed.reserve(contour.size());
    
    // Apply simple moving average to reduce pixel discretization artifacts
    const int windowSize = 3;
    for (size_t i = 0; i < contour.size(); ++i) {
        double sumX = 0, sumY = 0;
        int count = 0;
        
        for (int j = -windowSize; j <= windowSize; ++j) {
            size_t idx = (i + j + contour.size()) % contour.size();
            sumX += contour[idx].x;
            sumY += contour[idx].y;
            count++;
        }
        
        smoothed.emplace_back(
            static_cast<int>(std::round(sumX / count)),
            static_cast<int>(std::round(sumY / count))
        );
    }
    
    return smoothed;
}

// Find rectangle using Hough-like line detection approach
std::vector<Point> RectangleDetector::FindRectangleUsingHoughLines(const std::vector<Point>& contour) const {
    if (contour.size() < 8) return std::vector<Point>();
    
    // Detect dominant lines in the contour
    std::vector<std::pair<Point, Point>> lines = DetectLines(contour);
    
    if (lines.size() < 4) return std::vector<Point>();
    
    // Find pairs of perpendicular lines - need exactly 4 that form a closed rectangle
    std::vector<std::pair<Point, Point>> selectedLines;
    
    // First pass: find lines that could form a rectangle
    for (size_t i = 0; i < lines.size() && selectedLines.size() < 4; ++i) {
        bool shouldAdd = false;
        
        if (selectedLines.empty()) {
            shouldAdd = true; // Add first line
        } else {
            // Check perpendicularity with existing lines
            int perpendicularCount = 0;
            for (const auto& selectedLine : selectedLines) {
                if (AreLinesPerpendicular(lines[i], selectedLine, 0.15)) { // Stricter tolerance
                    perpendicularCount++;
                }
            }
            
            // For a rectangle, we need specific perpendicular relationships
            if (selectedLines.size() == 1 && perpendicularCount == 1) {
                shouldAdd = true; // Second line perpendicular to first
            } else if (selectedLines.size() == 2 && perpendicularCount == 1) {
                shouldAdd = true; // Third line perpendicular to second (parallel to first)
            } else if (selectedLines.size() == 3 && perpendicularCount == 2) {
                shouldAdd = true; // Fourth line perpendicular to third and first
            }
        }
        
        if (shouldAdd) {
            selectedLines.push_back(lines[i]);
        }
    }
    
    // If we have exactly 4 lines that form a rectangle, find their intersections
    if (selectedLines.size() == 4) {
        std::vector<Point> corners;
        corners.reserve(4);
        
        // Find intersections of adjacent lines to form corners
        for (size_t i = 0; i < 4; ++i) {
            size_t nextIdx = (i + 1) % 4;
            
            // Calculate intersection of lines i and nextIdx
            const auto& line1 = selectedLines[i];
            const auto& line2 = selectedLines[nextIdx];
            
            // Line intersection using parametric form
            double x1 = line1.first.x, y1 = line1.first.y;
            double x2 = line1.second.x, y2 = line1.second.y;
            double x3 = line2.first.x, y3 = line2.first.y;
            double x4 = line2.second.x, y4 = line2.second.y;
            
            double denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
            if (std::abs(denom) > EPSILON_TOLERANCE) {
                double t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
                double intersectX = x1 + t * (x2 - x1);
                double intersectY = y1 + t * (y2 - y1);
                
                corners.emplace_back(
                    static_cast<int>(std::round(intersectX)),
                    static_cast<int>(std::round(intersectY))
                );
            }
        }
        
        if (corners.size() == 4) {
            return corners;
        }
    }
    
    return std::vector<Point>();
}

// Detect dominant lines in contour using simplified Hough approach
std::vector<std::pair<Point, Point>> RectangleDetector::DetectLines(const std::vector<Point>& contour) const {
    std::vector<std::pair<Point, Point>> lines;
    
    if (contour.size() < 6) return lines;
    
    // Use sliding window to detect line segments
    const size_t windowSize = std::max(size_t(6), contour.size() / 8);
    const double minLineLength = 10.0;
    
    for (size_t i = 0; i < contour.size(); i += windowSize / 2) {
        size_t endIdx = std::min(i + windowSize, contour.size());
        
        if (endIdx - i < 4) continue;
        
        // Fit line to this segment using least squares
        double sumX = 0, sumY = 0, sumXX = 0, sumXY = 0;
        size_t count = endIdx - i;
        
        for (size_t j = i; j < endIdx; ++j) {
            size_t idx = j % contour.size();
            sumX += contour[idx].x;
            sumY += contour[idx].y;
            sumXX += contour[idx].x * contour[idx].x;
            sumXY += contour[idx].x * contour[idx].y;
        }
        
        double meanX = sumX / count;
        double meanY = sumY / count;
        
        double denominator = sumXX - count * meanX * meanX;
        if (std::abs(denominator) > EPSILON_TOLERANCE) {
            double slope = (sumXY - count * meanX * meanY) / denominator;
            double intercept = meanY - slope * meanX;
            
            // Create line segment from first to last point in window
            size_t startIdx = i % contour.size();
            size_t finalIdx = (endIdx - 1) % contour.size();
            
            Point start = contour[startIdx];
            Point end = contour[finalIdx];
            
            // Calculate distance to verify this is actually a line
            double lineLength = std::sqrt(
                (end.x - start.x) * (end.x - start.x) + 
                (end.y - start.y) * (end.y - start.y)
            );
            
            if (lineLength >= minLineLength) {
                lines.emplace_back(start, end);
            }
        }
    }
    
    return lines;
}

// Check if two lines are perpendicular within tolerance
bool RectangleDetector::AreLinesPerpendicular(const std::pair<Point, Point>& line1, 
                                            const std::pair<Point, Point>& line2, double tolerance) const {
    // Calculate direction vectors
    double dx1 = line1.second.x - line1.first.x;
    double dy1 = line1.second.y - line1.first.y;
    double dx2 = line2.second.x - line2.first.x;
    double dy2 = line2.second.y - line2.first.y;
    
    // Normalize vectors
    double len1 = std::sqrt(dx1 * dx1 + dy1 * dy1);
    double len2 = std::sqrt(dx2 * dx2 + dy2 * dy2);
    
    if (len1 < EPSILON_TOLERANCE || len2 < EPSILON_TOLERANCE) return false;
    
    dx1 /= len1; dy1 /= len1;
    dx2 /= len2; dy2 /= len2;
    
    // Calculate dot product
    double dotProduct = dx1 * dx2 + dy1 * dy2;
    
    // Perpendicular lines have dot product close to 0
    return std::abs(dotProduct) < tolerance;
}

// Quick check if contour is likely circular to avoid Hough processing on circles
bool RectangleDetector::IsLikelyCircularContour(const std::vector<Point>& contour) const {
    if (contour.size() < 8) return false;
    
    // Calculate center of mass
    double centerX = 0, centerY = 0;
    for (const auto& point : contour) {
        centerX += point.x;
        centerY += point.y;
    }
    centerX /= contour.size();
    centerY /= contour.size();
    
    // Calculate distances from center
    std::vector<double> distances;
    distances.reserve(contour.size());
    for (const auto& point : contour) {
        double dx = point.x - centerX;
        double dy = point.y - centerY;
        distances.push_back(std::sqrt(dx * dx + dy * dy));
    }
    
    // Calculate variance in distances
    double meanDist = 0;
    for (double dist : distances) {
        meanDist += dist;
    }
    meanDist /= distances.size();
    
    double variance = 0;
    for (double dist : distances) {
        double diff = dist - meanDist;
        variance += diff * diff;
    }
    variance /= distances.size();
    
    double stdDev = std::sqrt(variance);
    
    // If standard deviation is small relative to mean distance, it's likely circular
    return (stdDev / meanDist) < 0.15;
}

// Moment-based rectangle detection - completely rotation invariant
std::vector<Point> RectangleDetector::FindRectangleCornersMomentBased(const std::vector<Point>& contour) const {
    if (contour.size() < 8) return std::vector<Point>();
    
    // First check if this is actually rectangular using Hu moments
    if (!IsRectangleUsingMoments(contour)) {
        return std::vector<Point>();
    }
    
    // Calculate principal orientation
    double orientation = CalculateOrientation(contour);
    
    // Rotate contour to canonical position (axis-aligned)
    std::vector<Point> rotatedContour = RotateContourToCanonical(contour, -orientation);
    
    // Find bounding box of rotated contour with enhanced precision
    int minX = rotatedContour[0].x, maxX = rotatedContour[0].x;
    int minY = rotatedContour[0].y, maxY = rotatedContour[0].y;
    
    for (const auto& point : rotatedContour) {
        minX = std::min(minX, point.x);
        maxX = std::max(maxX, point.x);
        minY = std::min(minY, point.y);
        maxY = std::max(maxY, point.y);
    }
    
    // Apply small margin to improve detection at critical angles
    int margin = 1;
    minX -= margin;
    maxX += margin;
    minY -= margin;
    maxY += margin;
    
    // Create canonical rectangle corners with enhanced positioning
    std::vector<Point> canonicalCorners = {
        Point(minX, minY), Point(maxX, minY),
        Point(maxX, maxY), Point(minX, maxY)
    };
    
    // Rotate corners back to original orientation
    std::vector<Point> corners = RotateContourToCanonical(canonicalCorners, orientation);
    
    return corners;
}

// Check if shape is rectangular using rotation-invariant Hu moments
bool RectangleDetector::IsRectangleUsingMoments(const std::vector<Point>& contour) const {
    if (contour.size() < 8) return false;
    
    // Calculate multiple Hu moment invariants for better discrimination
    double m20 = CalculateHuMoment(contour, 2, 0);
    double m02 = CalculateHuMoment(contour, 0, 2);
    double m11 = CalculateHuMoment(contour, 1, 1);
    double m30 = CalculateHuMoment(contour, 3, 0);
    double m03 = CalculateHuMoment(contour, 0, 3);
    double m21 = CalculateHuMoment(contour, 2, 1);
    double m12 = CalculateHuMoment(contour, 1, 2);
    
    double hu1 = m20 + m02;
    double hu2 = std::pow(m20 - m02, 2) + 4 * std::pow(m11, 2);
    double hu3 = std::pow(m30 - 3*m12, 2) + std::pow(3*m21 - m03, 2);
    
    if (hu1 < EPSILON_TOLERANCE) return false;
    
    double momentRatio = hu2 / (hu1 * hu1);
    double skewness = hu3 / std::pow(hu1, 1.5);
    
    // Balanced criteria for rectangles - good recall with reasonable precision
    // 1. Moment ratio should be in rectangular range
    // 2. Skewness should be reasonable (rectangles are symmetric)
    // 3. Aspect ratio check via second moments
    
    // Tolerant but not excessive thresholds
    bool momentCheck = (momentRatio >= 0.003 && momentRatio <= 0.15);
    bool skewnessCheck = (std::abs(skewness) < 0.15);
    
    // Aspect ratio check with reasonable bounds
    double aspectRatio = (m02 > EPSILON_TOLERANCE) ? std::sqrt(m20 / m02) : 1.0;
    bool aspectCheck = (aspectRatio > 0.2 && aspectRatio < 15.0);
    
    // Ellipticity check to reject perfect circles
    double ellipticity = hu2 / (hu1 * hu1);
    bool ellipticityCheck = (ellipticity > 0.002);
    
    return momentCheck && skewnessCheck && aspectCheck && ellipticityCheck;
}

// Calculate normalized central moment (Hu moment component)
double RectangleDetector::CalculateHuMoment(const std::vector<Point>& contour, int p, int q) const {
    if (contour.empty()) return 0.0;
    
    Point centroid = CalculateCentroid(contour);
    
    // Calculate central moment
    double moment = 0.0;
    for (const auto& point : contour) {
        double x = point.x - centroid.x;
        double y = point.y - centroid.y;
        moment += std::pow(x, p) * std::pow(y, q);
    }
    
    // Normalize by area (m00)
    double area = contour.size(); // Approximation for discrete contour
    if (area > EPSILON_TOLERANCE) {
        double gamma = (p + q) / 2.0 + 1.0;
        moment /= std::pow(area, gamma);
    }
    
    return moment;
}

// Calculate centroid of contour
Point RectangleDetector::CalculateCentroid(const std::vector<Point>& contour) const {
    if (contour.empty()) return Point(0, 0);
    
    double sumX = 0, sumY = 0;
    for (const auto& point : contour) {
        sumX += point.x;
        sumY += point.y;
    }
    
    return Point(
        static_cast<int>(std::round(sumX / contour.size())),
        static_cast<int>(std::round(sumY / contour.size()))
    );
}

// Calculate principal orientation using second moments
double RectangleDetector::CalculateOrientation(const std::vector<Point>& contour) const {
    if (contour.size() < 3) return 0.0;
    
    Point centroid = CalculateCentroid(contour);
    
    // Calculate second central moments
    double m20 = 0, m02 = 0, m11 = 0;
    
    for (const auto& point : contour) {
        double x = point.x - centroid.x;
        double y = point.y - centroid.y;
        
        m20 += x * x;
        m02 += y * y;
        m11 += x * y;
    }
    
    // Principal orientation angle
    if (std::abs(m20 - m02) < EPSILON_TOLERANCE) {
        return 0.0; // Already axis-aligned
    }
    
    return 0.5 * std::atan2(2.0 * m11, m20 - m02);
}

// Rotate contour points by given angle around centroid with enhanced precision
std::vector<Point> RectangleDetector::RotateContourToCanonical(const std::vector<Point>& contour, double angle) const {
    if (contour.empty() || std::abs(angle) < EPSILON_TOLERANCE) return contour;
    
    Point centroid = CalculateCentroid(contour);
    std::vector<Point> rotated;
    rotated.reserve(contour.size());
    
    // Use higher precision rotation for critical angles
    double cosAngle = std::cos(angle);
    double sinAngle = std::sin(angle);
    
    // Apply smoothing for better rotation accuracy at steep angles
    for (const auto& point : contour) {
        // Translate to origin with subpixel precision
        double x = static_cast<double>(point.x) - static_cast<double>(centroid.x);
        double y = static_cast<double>(point.y) - static_cast<double>(centroid.y);
        
        // Rotate with high precision
        double rotX = x * cosAngle - y * sinAngle;
        double rotY = x * sinAngle + y * cosAngle;
        
        // Translate back with careful rounding
        rotated.emplace_back(
            static_cast<int>(std::floor(rotX + static_cast<double>(centroid.x) + 0.5)),
            static_cast<int>(std::floor(rotY + static_cast<double>(centroid.y) + 0.5))
        );
    }
    
    return rotated;
}

// Apply Gaussian blur for image smoothing
Image RectangleDetector::ApplyGaussianBlur(const Image& image, double sigma) const {
    if (sigma <= 0.1) return image; // Skip blur if sigma is too small
    
    Image result = image;
    
    // Calculate kernel size (should be odd)
    int kernelSize = static_cast<int>(2 * std::ceil(3 * sigma) + 1);
    if (kernelSize % 2 == 0) kernelSize++;
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
    for (double& k : kernel) {
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
                sourceX = std::max(0, std::min(sourceX, image.width - 1)); // Clamp
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
                sourceY = std::max(0, std::min(sourceY, image.height - 1)); // Clamp
                blurredValue += temp.pixels[sourceY][x] * kernel[k];
            }
            result.pixels[y][x] = static_cast<int>(std::round(blurredValue));
        }
    }
    
    return result;
}


// Remove duplicate rectangles (simplified since we're using single-scale)
void RectangleDetector::RemoveDuplicateRectangles(std::vector<Rectangle>& rectangles) const {
    if (rectangles.size() <= 1) return;
    
    // Sort by area for consistent processing (keep larger rectangles)
    std::sort(rectangles.begin(), rectangles.end(), [](const Rectangle& a, const Rectangle& b) {
        return a.width * a.height > b.width * b.height;
    });
    
    std::vector<bool> toRemove(rectangles.size(), false);
    
    for (size_t i = 0; i < rectangles.size(); ++i) {
        if (toRemove[i]) continue;
        
        for (size_t j = i + 1; j < rectangles.size(); ++j) {
            if (toRemove[j]) continue;
            
            // Check if rectangles overlap significantly
            double centerDist = std::sqrt(
                std::pow(rectangles[i].center.x - rectangles[j].center.x, 2) +
                std::pow(rectangles[i].center.y - rectangles[j].center.y, 2)
            );
            
            double avgSize = (rectangles[i].width + rectangles[i].height + 
                            rectangles[j].width + rectangles[j].height) / 4.0;
            
            // Enhanced duplicate removal for multiple detection strategies
            if (centerDist < avgSize * 0.25) {
                // Check overlap percentage
                double sizeRatio = std::min(
                    static_cast<double>(rectangles[i].width * rectangles[i].height),
                    static_cast<double>(rectangles[j].width * rectangles[j].height)
                ) / std::max(
                    static_cast<double>(rectangles[i].width * rectangles[i].height),
                    static_cast<double>(rectangles[j].width * rectangles[j].height)
                );
                
                if (sizeRatio > 0.5) { // Similar size rectangles
                    toRemove[j] = true; // Remove smaller/later one
                }
            }
        }
    }
    
    // Remove marked rectangles
    rectangles.erase(
        std::remove_if(rectangles.begin(), rectangles.end(), 
            [&toRemove, &rectangles](const Rectangle& rect) {
                return toRemove[&rect - &rectangles[0]];
            }),
        rectangles.end()
    );
}

// Enhanced preprocessing for steep angles
Image RectangleDetector::PreprocessImageEnhanced(const Image& image) const {
    Image result = image;
    
    // Apply edge enhancement before thresholding
    Image enhanced(image.width, image.height);
    
    // Sobel edge detection for better edge preservation
    #pragma omp parallel for
    for (int y = 1; y < image.height - 1; ++y) {
        for (int x = 1; x < image.width - 1; ++x) {
            // Sobel X kernel
            int gx = -image.pixels[y-1][x-1] + image.pixels[y-1][x+1] +
                     -2*image.pixels[y][x-1] + 2*image.pixels[y][x+1] +
                     -image.pixels[y+1][x-1] + image.pixels[y+1][x+1];
            
            // Sobel Y kernel  
            int gy = -image.pixels[y-1][x-1] - 2*image.pixels[y-1][x] - image.pixels[y-1][x+1] +
                      image.pixels[y+1][x-1] + 2*image.pixels[y+1][x] + image.pixels[y+1][x+1];
            
            int magnitude = static_cast<int>(std::sqrt(gx*gx + gy*gy));
            enhanced.pixels[y][x] = std::min(255, magnitude);
        }
    }
    
    // Apply light Gaussian blur to reduce noise
    Image blurred = ApplyGaussianBlur(enhanced, 0.5);
    
    // Enhanced thresholding with higher threshold for edges
    #pragma omp parallel for
    for (int y = 0; y < blurred.height; ++y) {
        for (int x = 0; x < blurred.width; ++x) {
            result.pixels[y][x] = (blurred.pixels[y][x] > 100) ? 255 : 0;
        }
    }
    
    return result;
}

// Morphological preprocessing for broken contours
Image RectangleDetector::PreprocessImageMorphological(const Image& image) const {
    Image result = image;
    
    // Standard thresholding first
    #pragma omp parallel for
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            result.pixels[y][x] = (image.pixels[y][x] > 127) ? 255 : 0;
        }
    }
    
    // Apply morphological closing to connect broken rectangle edges
    Image closed = ApplyMorphologyClose(result, 3);
    
    // Apply morphological opening to remove small noise
    Image opened = ApplyMorphologyOpen(closed, 2);
    
    return opened;
}

// Morphological closing operation
Image RectangleDetector::ApplyMorphologyClose(const Image& image, int kernelSize) const {
    if (kernelSize < 1) return image;
    
    // Closing = Dilation followed by Erosion
    Image dilated = image;
    Image result = image;
    
    int halfKernel = kernelSize / 2;
    
    // Dilation
    #pragma omp parallel for
    for (int y = halfKernel; y < image.height - halfKernel; ++y) {
        for (int x = halfKernel; x < image.width - halfKernel; ++x) {
            int maxVal = 0;
            for (int dy = -halfKernel; dy <= halfKernel; ++dy) {
                for (int dx = -halfKernel; dx <= halfKernel; ++dx) {
                    maxVal = std::max(maxVal, image.pixels[y+dy][x+dx]);
                }
            }
            dilated.pixels[y][x] = maxVal;
        }
    }
    
    // Erosion
    #pragma omp parallel for
    for (int y = halfKernel; y < image.height - halfKernel; ++y) {
        for (int x = halfKernel; x < image.width - halfKernel; ++x) {
            int minVal = 255;
            for (int dy = -halfKernel; dy <= halfKernel; ++dy) {
                for (int dx = -halfKernel; dx <= halfKernel; ++dx) {
                    minVal = std::min(minVal, dilated.pixels[y+dy][x+dx]);
                }
            }
            result.pixels[y][x] = minVal;
        }
    }
    
    return result;
}

// Morphological opening operation
Image RectangleDetector::ApplyMorphologyOpen(const Image& image, int kernelSize) const {
    if (kernelSize < 1) return image;
    
    // Opening = Erosion followed by Dilation
    Image eroded = image;
    Image result = image;
    
    int halfKernel = kernelSize / 2;
    
    // Erosion
    #pragma omp parallel for
    for (int y = halfKernel; y < image.height - halfKernel; ++y) {
        for (int x = halfKernel; x < image.width - halfKernel; ++x) {
            int minVal = 255;
            for (int dy = -halfKernel; dy <= halfKernel; ++dy) {
                for (int dx = -halfKernel; dx <= halfKernel; ++dx) {
                    minVal = std::min(minVal, image.pixels[y+dy][x+dx]);
                }
            }
            eroded.pixels[y][x] = minVal;
        }
    }
    
    // Dilation
    #pragma omp parallel for
    for (int y = halfKernel; y < image.height - halfKernel; ++y) {
        for (int x = halfKernel; x < image.width - halfKernel; ++x) {
            int maxVal = 0;
            for (int dy = -halfKernel; dy <= halfKernel; ++dy) {
                for (int dx = -halfKernel; dx <= halfKernel; ++dx) {
                    maxVal = std::max(maxVal, eroded.pixels[y+dy][x+dx]);
                }
            }
            result.pixels[y][x] = maxVal;
        }
    }
    
    return result;
}

// Simplified Hough line-based rectangle detection for critical angles
std::vector<Rectangle> RectangleDetector::DetectRectanglesUsingHoughLines(const Image& image) const {
    std::vector<Rectangle> rectangles;
    
    // For now, return empty - Hough transform is complex and might not be needed
    // if the other three strategies work well enough
    return rectangles;
}

