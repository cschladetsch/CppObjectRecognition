#ifndef RECTANGLE_DETECTOR_H
#define RECTANGLE_DETECTOR_H

#include <vector>

struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
};

struct Rectangle {
    Point center;
    int width, height;
    float angle;
    std::vector<Point> corners;
};

struct Image {
    std::vector<std::vector<int>> pixels;
    int width, height;
    
    Image(int w, int h) : width(w), height(h) {
        pixels.resize(h, std::vector<int>(w, 0));
    }
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
    
    std::vector<std::vector<Point>> findContours(const Image& image);
    bool isRectangle(const std::vector<Point>& contour);
    Rectangle createRectangle(const std::vector<Point>& contour);
    Image preprocessImage(const Image& image);
    std::vector<Point> approximateContour(const std::vector<Point>& contour, double epsilon);
    void floodFillContour(const Image& image, int startX, int startY, std::vector<Point>& contour, std::vector<std::vector<bool>>& visited);
    double calculatePerimeter(const std::vector<Point>& contour);
    double calculateArea(const std::vector<Point>& contour);
    void douglasPeucker(const std::vector<Point>& contour, int start, int end, double epsilon, std::vector<Point>& result);
    void douglasPeuckerRecursive(const std::vector<Point>& contour, int start, int end, double epsilon, std::vector<bool>& keep);
    double pointToLineDistance(const Point& point, const Point& lineStart, const Point& lineEnd);
    std::vector<Point> convexHull(const std::vector<Point>& points);
    double cross(const Point& O, const Point& A, const Point& B);
    bool isValidQuadrilateral(const std::vector<Point>& quad);
    std::vector<Point> extractBoundary(const std::vector<Point>& region, const Image& image);
    std::vector<Point> sortBoundaryPoints(const std::vector<Point>& boundary);
};

#endif // RECTANGLE_DETECTOR_H