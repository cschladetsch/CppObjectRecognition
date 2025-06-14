#pragma once

#include <vector>
#include <array>
#include <bitset>
#include <stack>

struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
};

struct Rectangle {
    Point center;
    int width, height;
    double angle;  // angle in degrees
};

struct Image {
    std::vector<std::vector<int>> pixels;
    int width, height;
    
    Image(int w, int h) : width(w), height(h) {
        pixels.resize(h, std::vector<int>(w, 0));
    }
};

// Optimized data structure for scanline flood fill
struct ScanlineSegment {
    int y, x1, x2;
    int parentY;
    ScanlineSegment(int y, int x1, int x2, int parentY) 
        : y(y), x1(x1), x2(x2), parentY(parentY) {}
};

class RectangleDetector {
public:
    RectangleDetector();
    ~RectangleDetector();
    
    std::vector<Rectangle> detectRectangles(const Image& image);
    void setMinArea(double minArea);
    void setMaxArea(double maxArea);
    void setApproxEpsilon(double epsilon);
    
private:
    double minArea_;
    double maxArea_;
    double approxEpsilon_;
    
    // Cache for expensive calculations
    mutable std::vector<double> distanceCache_;
    mutable std::vector<double> angleCache_;
    
    std::vector<std::vector<Point>> findContours(const Image& image) const;
    bool isRectangle(const std::vector<Point>& contour) const;
    Rectangle createRectangle(const std::vector<Point>& contour) const;
    Image preprocessImage(const Image& image) const;
    std::vector<Point> approximateContour(const std::vector<Point>& contour, double epsilon) const;
    void scanlineFillContour(const Image& image, int startX, int startY, std::vector<Point>& contour, std::vector<std::vector<bool>>& visited) const;
    double calculatePerimeter(const std::vector<Point>& contour) const;
    double calculateArea(const std::vector<Point>& contour) const;
    void douglasPeuckerRecursive(const std::vector<Point>& contour, int start, int end, double epsilon, std::vector<bool>& keep) const;
    double pointToLineDistanceSquared(const Point& point, const Point& lineStart, const Point& lineEnd) const;
    std::vector<Point> convexHull(std::vector<Point> points) const;
    double cross(const Point& O, const Point& A, const Point& B) const;
    bool isValidQuadrilateral(const std::vector<Point>& quad) const;
    std::vector<Point> extractBoundary(const std::vector<Point>& region, const Image& image) const;
    std::vector<Point> sortBoundaryPointsRadix(std::vector<Point> boundary) const;
    std::vector<Point> cleanupCorners(const std::vector<Point>& corners) const;
    std::array<Point, 4> selectBestCorners(const std::vector<Point>& corners) const;
    double calculateCornerAngle(const Point& prev, const Point& current, const Point& next) const;
    double calculateCornerAngleFast(const Point& prev, const Point& current, const Point& next) const;
    Point calculateContourCentroid(const std::vector<Point>& contour) const;
};

