#include "shape_detector/rectangle_detector.h"
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

RectangleDetector::RectangleDetector() 
    : minArea_(500.0), maxArea_(10000.0), approxEpsilon_(0.05) {}

RectangleDetector::~RectangleDetector() {}

void RectangleDetector::setMinArea(double minArea) {
    minArea_ = minArea;
}

void RectangleDetector::setMaxArea(double maxArea) {
    maxArea_ = maxArea;
}

void RectangleDetector::setApproxEpsilon(double epsilon) {
    approxEpsilon_ = epsilon;
}

std::vector<Rectangle> RectangleDetector::detectRectangles(const Image& image) {
    std::vector<Rectangle> rectangles;
    
    Image processed = preprocessImage(image);
    std::vector<std::vector<Point>> contours = findContours(processed);
    
    
    for (const auto& contour : contours) {
        if (isRectangle(contour)) {
            Rectangle rect = createRectangle(contour);
            if (rect.width > 0 && rect.height > 0) {
                rectangles.push_back(rect);
            }
        }
    }
    
    return rectangles;
}

Image RectangleDetector::preprocessImage(const Image& image) {
    Image result = image;
    
    for (int y = 0; y < result.height; ++y) {
        for (int x = 0; x < result.width; ++x) {
            result.pixels[y][x] = (result.pixels[y][x] > 127) ? 255 : 0;
        }
    }
    
    return result;
}

std::vector<std::vector<Point>> RectangleDetector::findContours(const Image& image) {
    std::vector<std::vector<Point>> contours;
    std::vector<std::vector<bool>> visited(image.height, std::vector<bool>(image.width, false));
    
    // Find all connected white regions
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            if (!visited[y][x] && image.pixels[y][x] == 255) {
                std::vector<Point> region;
                floodFillContour(image, x, y, region, visited);
                
                if (region.size() >= 50) { // Minimum size for a rectangle
                    // Convert filled region to boundary contour
                    std::vector<Point> boundary = extractBoundary(region, image);
                    if (boundary.size() >= 8) {
                        contours.push_back(boundary);
                    }
                }
            }
        }
    }
    
    return contours;
}

void RectangleDetector::floodFillContour(const Image& image, int startX, int startY, 
                                       std::vector<Point>& contour, std::vector<std::vector<bool>>& visited) {
    // Simple flood fill to find all connected white pixels
    std::vector<Point> stack;
    stack.push_back(Point(startX, startY));
    
    while (!stack.empty()) {
        Point current = stack.back();
        stack.pop_back();
        
        if (current.x < 0 || current.x >= image.width || 
            current.y < 0 || current.y >= image.height ||
            visited[current.y][current.x] || 
            image.pixels[current.y][current.x] != 255) {
            continue;
        }
        
        visited[current.y][current.x] = true;
        contour.push_back(current);
        
        // Add 4-connected neighbors (horizontal and vertical only)
        // This prevents diagonal connections that could merge separate rectangles
        stack.push_back(Point(current.x + 1, current.y));
        stack.push_back(Point(current.x - 1, current.y));
        stack.push_back(Point(current.x, current.y + 1));
        stack.push_back(Point(current.x, current.y - 1));
    }
}

bool RectangleDetector::isRectangle(const std::vector<Point>& contour) {
    if (contour.size() < 4) return false;
    
    std::vector<Point> approx = approximateContour(contour, approxEpsilon_);
    
    // Allow 4 to 6 points (rotated rectangles might have extra vertices)
    if (approx.size() < 4 || approx.size() > 6) return false;
    
    double area = calculateArea(approx);
    return area >= minArea_ && area <= maxArea_;
}

bool RectangleDetector::isValidQuadrilateral(const std::vector<Point>& quad) {
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

std::vector<Point> RectangleDetector::approximateContour(const std::vector<Point>& contour, double epsilon) {
    if (contour.size() < 4) return contour;
    
    // Use direct Douglas-Peucker on the contour for better results with rotated rectangles
    std::vector<Point> approx;
    double perimeter = calculatePerimeter(contour);
    double epsilonValue = epsilon * perimeter;
    
    // Start with a more aggressive epsilon for rotated rectangles
    if (epsilonValue < 5.0) epsilonValue = 5.0;
    
    std::vector<bool> keep(contour.size(), false);
    keep[0] = keep[contour.size() - 1] = true;
    
    douglasPeuckerRecursive(contour, 0, contour.size() - 1, epsilonValue, keep);
    
    for (size_t i = 0; i < contour.size(); ++i) {
        if (keep[i]) {
            approx.push_back(contour[i]);
        }
    }
    
    // If we got too many points, try with a larger epsilon
    if (approx.size() > 6) {
        approx.clear();
        epsilonValue *= 2.0;
        std::fill(keep.begin(), keep.end(), false);
        keep[0] = keep[contour.size() - 1] = true;
        
        douglasPeuckerRecursive(contour, 0, contour.size() - 1, epsilonValue, keep);
        
        for (size_t i = 0; i < contour.size(); ++i) {
            if (keep[i]) {
                approx.push_back(contour[i]);
            }
        }
    }
    
    return approx;
}

void RectangleDetector::douglasPeuckerRecursive(const std::vector<Point>& contour, int start, int end,
                                              double epsilon, std::vector<bool>& keep) {
    if (end - start <= 1) return;
    
    double maxDist = 0.0;
    int maxIndex = start;
    
    for (int i = start + 1; i < end; ++i) {
        double dist = pointToLineDistance(contour[i], contour[start], contour[end]);
        if (dist > maxDist) {
            maxDist = dist;
            maxIndex = i;
        }
    }
    
    if (maxDist > epsilon) {
        keep[maxIndex] = true;
        douglasPeuckerRecursive(contour, start, maxIndex, epsilon, keep);
        douglasPeuckerRecursive(contour, maxIndex, end, epsilon, keep);
    }
}

double RectangleDetector::calculatePerimeter(const std::vector<Point>& contour) {
    double perimeter = 0.0;
    for (size_t i = 0; i < contour.size(); ++i) {
        size_t j = (i + 1) % contour.size();
        double dx = contour[j].x - contour[i].x;
        double dy = contour[j].y - contour[i].y;
        perimeter += std::sqrt(dx * dx + dy * dy);
    }
    return perimeter;
}

double RectangleDetector::calculateArea(const std::vector<Point>& contour) {
    if (contour.size() < 3) return 0.0;
    
    double area = 0.0;
    int n = contour.size();
    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        area += contour[i].x * contour[j].y;
        area -= contour[j].x * contour[i].y;
    }
    return std::abs(area) / 2.0;
}

void RectangleDetector::douglasPeucker(const std::vector<Point>& contour, int start, int end,
                                     double epsilon, std::vector<Point>& result) {
    double maxDist = 0.0;
    int maxIndex = start;
    
    for (int i = start + 1; i < end; ++i) {
        double dist = pointToLineDistance(contour[i], contour[start], contour[end]);
        if (dist > maxDist) {
            maxDist = dist;
            maxIndex = i;
        }
    }
    
    if (maxDist > epsilon) {
        douglasPeucker(contour, start, maxIndex, epsilon, result);
        douglasPeucker(contour, maxIndex, end, epsilon, result);
    } else {
        result.push_back(contour[start]);
        if (start != end) result.push_back(contour[end]);
    }
}

double RectangleDetector::pointToLineDistance(const Point& point, const Point& lineStart, const Point& lineEnd) {
    double A = lineEnd.y - lineStart.y;
    double B = lineStart.x - lineEnd.x;
    double C = lineEnd.x * lineStart.y - lineStart.x * lineEnd.y;
    
    double denominator = std::sqrt(A * A + B * B);
    if (denominator == 0) return 0;
    
    return std::abs(A * point.x + B * point.y + C) / denominator;
}

Rectangle RectangleDetector::createRectangle(const std::vector<Point>& contour) {
    Rectangle rect;
    
    std::vector<Point> approx = approximateContour(contour, approxEpsilon_);
    
    // Clean up the approximation - remove duplicate points
    std::vector<Point> cleanCorners = cleanupCorners(approx);
    
    // Try to form a proper rectangle with 4 corners
    if (cleanCorners.size() < 3) {
        // Not enough points for any polygon, reject
        return rect;
    } else if (cleanCorners.size() == 3) {
        // For 3 corners, reject - these create triangular artifacts
        return rect;
    } else if (cleanCorners.size() > 4) {
        // Too many corners, try to reduce to best 4
        cleanCorners = selectBestCorners(cleanCorners);
        if (cleanCorners.size() != 4) {
            return rect; // Failed to get exactly 4 corners
        }
    }
    // If we have exactly 4 corners, proceed
    if (cleanCorners.size() == 4) {
        // Calculate center using contour centroid for better accuracy with rotated rectangles
        Point centroid = calculateContourCentroid(contour);
        rect.center = centroid;
        
        // Find the two pairs of opposite edges to determine width, height, and angle
        // Calculate all edge lengths and vectors
        std::vector<double> edgeLengths;
        std::vector<std::pair<double, double>> edgeVectors;
        
        for (int i = 0; i < 4; ++i) {
            int nextIdx = (i + 1) % 4;
            double dx = cleanCorners[nextIdx].x - cleanCorners[i].x;
            double dy = cleanCorners[nextIdx].y - cleanCorners[i].y;
            double length = std::sqrt(dx * dx + dy * dy);
            
            edgeLengths.push_back(length);
            edgeVectors.push_back({dx / length, dy / length}); // normalized
        }
        
        // Find the two shortest opposing edges (should be width) and two longest (should be height)
        // For a rectangle, opposite edges should be equal
        double edge1 = edgeLengths[0];
        double edge2 = edgeLengths[1];
        
        if (edge1 > edge2) {
            rect.width = static_cast<int>(edge1);
            rect.height = static_cast<int>(edge2);
            rect.angle = std::atan2(edgeVectors[0].second, edgeVectors[0].first) * 180.0 / M_PI;
        } else {
            rect.width = static_cast<int>(edge2);
            rect.height = static_cast<int>(edge1);
            rect.angle = std::atan2(edgeVectors[1].second, edgeVectors[1].first) * 180.0 / M_PI;
        }
    }
    
    return rect;
}

std::vector<Point> RectangleDetector::cleanupCorners(const std::vector<Point>& corners) {
    if (corners.size() <= 4) {
        // If we have 4 or fewer corners, remove only exact duplicates
        std::vector<Point> cleaned;
        const double minDistance = 1.0; // Very small distance for exact duplicates
        
        for (size_t i = 0; i < corners.size(); ++i) {
            bool keepPoint = true;
            
            for (const auto& kept : cleaned) {
                double dx = corners[i].x - kept.x;
                double dy = corners[i].y - kept.y;
                double dist = std::sqrt(dx * dx + dy * dy);
                
                if (dist < minDistance) {
                    keepPoint = false;
                    break;
                }
            }
            
            if (keepPoint) {
                cleaned.push_back(corners[i]);
            }
        }
        
        return cleaned;
    }
    
    // For more than 4 corners, be more aggressive
    std::vector<Point> cleaned;
    const double minDistance = 8.0; // Minimum distance between corners
    
    for (size_t i = 0; i < corners.size(); ++i) {
        bool keepPoint = true;
        
        // Check if this point is too close to any already kept point
        for (const auto& kept : cleaned) {
            double dx = corners[i].x - kept.x;
            double dy = corners[i].y - kept.y;
            double dist = std::sqrt(dx * dx + dy * dy);
            
            if (dist < minDistance) {
                keepPoint = false;
                break;
            }
        }
        
        if (keepPoint) {
            cleaned.push_back(corners[i]);
        }
    }
    
    return cleaned;
}

std::vector<Point> RectangleDetector::selectBestCorners(const std::vector<Point>& corners) {
    if (corners.size() <= 4) return corners;
    
    // Find the convex hull to get the outermost points
    std::vector<Point> hull = convexHull(corners);
    
    if (hull.size() == 4) {
        return hull;
    } else if (hull.size() > 4) {
        // If we have more than 4 points in the hull, select the 4 most corner-like
        // by finding points with the largest angle changes
        std::vector<std::pair<double, Point>> angleCorners;
        
        for (size_t i = 0; i < hull.size(); ++i) {
            size_t prev = (i - 1 + hull.size()) % hull.size();
            size_t next = (i + 1) % hull.size();
            
            // Calculate angle at this point
            double angle = calculateCornerAngle(hull[prev], hull[i], hull[next]);
            angleCorners.push_back({angle, hull[i]});
        }
        
        // Sort by angle (largest angles are most corner-like)
        std::sort(angleCorners.begin(), angleCorners.end(), 
                 [](const auto& a, const auto& b) { return a.first > b.first; });
        
        // Take the 4 best corners
        std::vector<Point> bestCorners;
        for (int i = 0; i < 4 && i < angleCorners.size(); ++i) {
            bestCorners.push_back(angleCorners[i].second);
        }
        
        // Sort them in proper order around the shape
        return sortBoundaryPoints(bestCorners);
    }
    
    return hull;
}

double RectangleDetector::calculateCornerAngle(const Point& prev, const Point& current, const Point& next) {
    double dx1 = prev.x - current.x;
    double dy1 = prev.y - current.y;
    double dx2 = next.x - current.x;
    double dy2 = next.y - current.y;
    
    double angle1 = std::atan2(dy1, dx1);
    double angle2 = std::atan2(dy2, dx2);
    
    double angleDiff = std::abs(angle2 - angle1);
    if (angleDiff > M_PI) angleDiff = 2 * M_PI - angleDiff;
    
    return angleDiff;
}

std::vector<Point> RectangleDetector::extractBoundary(const std::vector<Point>& region, const Image& image) {
    std::vector<Point> boundary;
    
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
        boundary = sortBoundaryPoints(boundary);
    }
    
    return boundary;
}

std::vector<Point> RectangleDetector::sortBoundaryPoints(const std::vector<Point>& boundary) {
    if (boundary.size() < 3) return boundary;
    
    // Find centroid
    double centerX = 0, centerY = 0;
    for (const Point& p : boundary) {
        centerX += p.x;
        centerY += p.y;
    }
    centerX /= boundary.size();
    centerY /= boundary.size();
    
    // Sort points by angle from center
    std::vector<Point> sorted = boundary;
    std::sort(sorted.begin(), sorted.end(), [centerX, centerY](const Point& a, const Point& b) {
        double angleA = std::atan2(a.y - centerY, a.x - centerX);
        double angleB = std::atan2(b.y - centerY, b.x - centerX);
        return angleA < angleB;
    });
    
    return sorted;
}

Point RectangleDetector::calculateContourCentroid(const std::vector<Point>& contour) {
    if (contour.empty()) {
        return Point(0, 0);
    }
    
    // Use polygon area centroid formula for better accuracy
    double area = 0.0;
    double centroidX = 0.0;
    double centroidY = 0.0;
    
    // Calculate signed area and centroid using the shoelace formula
    for (size_t i = 0; i < contour.size(); ++i) {
        size_t j = (i + 1) % contour.size();
        double cross = contour[i].x * contour[j].y - contour[j].x * contour[i].y;
        area += cross;
        centroidX += (contour[i].x + contour[j].x) * cross;
        centroidY += (contour[i].y + contour[j].y) * cross;
    }
    
    area *= 0.5;
    
    if (std::abs(area) < 1e-6) {
        // Fallback to simple average if area is too small
        double sumX = 0.0, sumY = 0.0;
        for (const Point& p : contour) {
            sumX += p.x;
            sumY += p.y;
        }
        return Point(static_cast<int>(sumX / contour.size()), 
                     static_cast<int>(sumY / contour.size()));
    }
    
    centroidX /= (6.0 * area);
    centroidY /= (6.0 * area);
    
    return Point(static_cast<int>(centroidX), static_cast<int>(centroidY));
}

std::vector<Point> RectangleDetector::convexHull(const std::vector<Point>& points) {
    if (points.size() < 3) return points;
    
    std::vector<Point> sortedPoints = points;
    
    // Sort points lexicographically (by x, then by y)
    std::sort(sortedPoints.begin(), sortedPoints.end(), [](const Point& a, const Point& b) {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
    });
    
    // Build lower hull
    std::vector<Point> lower;
    for (const auto& p : sortedPoints) {
        while (lower.size() >= 2 && cross(lower[lower.size()-2], lower[lower.size()-1], p) <= 0) {
            lower.pop_back();
        }
        lower.push_back(p);
    }
    
    // Build upper hull
    std::vector<Point> upper;
    for (auto it = sortedPoints.rbegin(); it != sortedPoints.rend(); ++it) {
        while (upper.size() >= 2 && cross(upper[upper.size()-2], upper[upper.size()-1], *it) <= 0) {
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

double RectangleDetector::cross(const Point& O, const Point& A, const Point& B) {
    return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
}