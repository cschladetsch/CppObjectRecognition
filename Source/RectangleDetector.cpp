#include "ShapeDetector/RectangleDetector.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <queue>
#include <omp.h>
#include <numbers>
constexpr double MIN_DISTANCE_SQUARED = 1.0;
constexpr double MIN_DISTANCE_SQUARED_LARGE = 64.0;

RectangleDetector::RectangleDetector() 
    : minArea_(500.0), maxArea_(10000.0), approxEpsilon_(0.02) {}

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
    
    Image processed = PreprocessImage(image);
    std::vector<std::vector<Point>> contours = FindContours(processed);
    
    
    for (size_t i = 0; i < contours.size(); ++i) {
        const auto& contour = contours[i];
        if (IsRectangle(contour)) {
            Rectangle rect = CreateRectangle(contour);
            if (rect.width > 0 && rect.height > 0) {
                rectangles.push_back(rect);
            }
        }
    }
    
    return rectangles;
}

Image RectangleDetector::PreprocessImage(const Image& image) const {
    Image result = image;
    
    #pragma omp parallel for
    for (int y = 0; y < result.height; ++y) {
        for (int x = 0; x < result.width; ++x) {
            result.pixels[y][x] = (result.pixels[y][x] > 127) ? 255 : 0;
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
        
        // Process scanline
        for (int x = seg.x1; x <= seg.x2; x++) {
            if (!visited[seg.y][x]) {
                visited[seg.y][x] = true;
                contour.push_back(Point(x, seg.y));
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
    
    
    // First check: must have exactly 4 vertices for a rectangle
    if (approx.size() != 4) {
        // Allow 5-6 vertices for slightly imperfect rectangles
        if (approx.size() < 4 || approx.size() > 6) return false;
        
        // If we have 5-6 vertices, try to find the best 4 corners
        if (approx.size() > 4) {
            auto corners = SelectBestCorners(approx);
            approx = std::vector<Point>(corners.begin(), corners.end());
            if (approx.size() != 4) return false;
        }
    }
    
    // Check area constraints
    double area = CalculateArea(approx);
    if (area < minArea_ || area > maxArea_) return false;
    
    // Check if it's a valid quadrilateral (parallel sides)
    if (!IsValidQuadrilateral(approx)) return false;
    
    // Additional check: verify corner angles are close to π/2 radians (90 degrees)
    for (int i = 0; i < 4; ++i) {
        int prev = (i + 3) % 4;
        int next = (i + 1) % 4;
        double angle = CalculateCornerAngleFast(approx[prev], approx[i], approx[next]);
        
        
        // Rectangle corners should be close to π/2 radians (allow ~0.6 radians tolerance, about 34 degrees)
        if (std::abs(angle - std::numbers::pi / 2.0) > 0.6) {
            return false;
        }
    }
    
    // Check rectangularity: compare area with bounding box area
    int minX = approx[0].x, maxX = approx[0].x;
    int minY = approx[0].y, maxY = approx[0].y;
    for (const auto& p : approx) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }
    double boundingBoxArea = (maxX - minX) * (maxY - minY);
    double rectangularity = area / boundingBoxArea;
    
    
    // For a perfect rectangle, this ratio should be close to 1
    // Allow more tolerance for rotated rectangles (45° rotation gives ~0.5)
    if (rectangularity < 0.5) {
        return false;
    }
    
    return true;
}

bool RectangleDetector::IsValidQuadrilateral(const std::vector<Point>& quad) const {
    if (quad.size() != 4) return false;
    
    // Calculate vectors for each side
    std::vector<std::pair<double, double>> sides;
    for (int i = 0; i < 4; ++i) {
        int next = (i + 1) % 4;
        double dx = quad[next].x - quad[i].x;
        double dy = quad[next].y - quad[i].y;
        double length = std::sqrt(dx * dx + dy * dy);
        if (length > 0) {
            sides.push_back({dx / length, dy / length}); // Normalized vector
        }
    }
    
    if (sides.size() != 4) return false;
    
    // Check if opposite sides are roughly parallel (dot product close to ±1)
    double dot1 = sides[0].first * sides[2].first + sides[0].second * sides[2].second;
    double dot2 = sides[1].first * sides[3].first + sides[1].second * sides[3].second;
    
    // Allow some tolerance for rotation and imperfect detection
    return (std::abs(std::abs(dot1) - 1.0) < 0.3) && (std::abs(std::abs(dot2) - 1.0) < 0.3);
}

std::vector<Point> RectangleDetector::ApproximateContour(const std::vector<Point>& contour, double epsilon) const {
    if (contour.size() < 4) return contour;
    
    // Use direct Douglas-Peucker on the contour for better results with rotated rectangles
    std::vector<Point> approx;
    approx.reserve(8);  // Pre-allocate for typical polygon
    const double perimeter = CalculatePerimeter(contour);
    double epsilonValue = std::max(epsilon * perimeter, 5.0);
    
    std::vector<bool> keep(contour.size(), false);
    keep[0] = keep[contour.size() - 1] = true;
    
    DouglasPeuckerRecursive(contour, 0, contour.size() - 1, epsilonValue, keep);
    
    for (size_t i = 0; i < contour.size(); ++i) {
        if (keep[i]) {
            approx.push_back(contour[i]);
        }
    }
    
    if (approx.size() > 6) {
        approx.clear();
        epsilonValue *= 2.0;
        std::fill(keep.begin(), keep.end(), false);
        keep[0] = keep[contour.size() - 1] = true;
        
        DouglasPeuckerRecursive(contour, 0, contour.size() - 1, epsilonValue, keep);
        
        for (size_t i = 0; i < contour.size(); ++i) {
            if (keep[i]) {
                approx.push_back(contour[i]);
            }
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
    double perimeter = 0.0;
    for (size_t i = 0; i < contour.size(); ++i) {
        const size_t j = (i + 1) % contour.size();
        const double dx = contour[j].x - contour[i].x;
        const double dy = contour[j].y - contour[i].y;
        perimeter += std::sqrt(dx * dx + dy * dy);
    }
    return perimeter;
}

double RectangleDetector::CalculateArea(const std::vector<Point>& contour) const {
    if (contour.size() < 3) return 0.0;
    
    double area = 0.0;
    const size_t n = contour.size();
    for (size_t i = 0; i < n; ++i) {
        const size_t j = (i + 1) % n;
        area += contour[i].x * contour[j].y - contour[j].x * contour[i].y;
    }
    return std::abs(area) * 0.5;
}


double RectangleDetector::PointToLineDistanceSquared(const Point& point, const Point& lineStart, const Point& lineEnd) const {
    double A = lineEnd.y - lineStart.y;
    double B = lineStart.x - lineEnd.x;
    double C = lineEnd.x * lineStart.y - lineStart.x * lineEnd.y;
    
    double denominatorSquared = A * A + B * B;
    if (denominatorSquared == 0) return 0;
    
    double distance = A * point.x + B * point.y + C;
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