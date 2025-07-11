#pragma once

#include <array>
#include <bitset>
#include <stack>
#include <vector>

struct Point {
  int x, y;
  Point(int x = 0, int y = 0) : x(x), y(y) {}
};

struct Rectangle {
  Point center;
  int width, height;
  double angle; // angle in radians
};

struct Obloid {
  Point center;
  int radius;
  double confidence; // detection confidence score
};

struct Sphere {
  Point center;
  int radius;
  double confidence; // detection confidence score
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

  std::vector<Rectangle> DetectRectangles(const Image &image);
  void SetMinArea(double minArea);
  void SetMaxArea(double maxArea);
  void SetApproxEpsilon(double epsilon);

private:
  double minArea_;
  double maxArea_;
  double approxEpsilon_;

  // Cache for expensive calculations
  mutable std::vector<double> distanceCache_;
  mutable std::vector<double> angleCache_;

  std::vector<std::vector<Point>> FindContours(const Image &image) const;
  bool IsRectangle(const std::vector<Point> &contour) const;
  Rectangle CreateRectangle(const std::vector<Point> &contour) const;
  Image PreprocessImage(const Image &image) const;
  std::vector<Point> ApproximateContour(const std::vector<Point> &contour,
                                        double epsilon) const;
  void ScanlineFillContour(const Image &image, int startX, int startY,
                           std::vector<Point> &contour,
                           std::vector<std::vector<bool>> &visited) const;
  double CalculatePerimeter(const std::vector<Point> &contour) const;
  double CalculateArea(const std::vector<Point> &contour) const;
  void DouglasPeuckerRecursive(const std::vector<Point> &contour, int start,
                               int end, double epsilon,
                               std::vector<bool> &keep) const;
  double PointToLineDistanceSquared(const Point &point, const Point &lineStart,
                                    const Point &lineEnd) const;
  std::vector<Point> ConvexHull(std::vector<Point> points) const;
  double Cross(const Point &O, const Point &A, const Point &B) const;
  bool IsValidQuadrilateral(const std::vector<Point> &quad) const;
  std::vector<Point> ExtractBoundary(const std::vector<Point> &region,
                                     const Image &image) const;
  std::vector<Point> SortBoundaryPointsRadix(std::vector<Point> boundary) const;
  std::vector<Point> CleanupCorners(const std::vector<Point> &corners) const;
  std::array<Point, 4>
  SelectBestCorners(const std::vector<Point> &corners) const;
  double CalculateCornerAngle(const Point &prev, const Point &current,
                              const Point &next) const;
  double CalculateCornerAngleFast(const Point &prev, const Point &current,
                                  const Point &next) const;
  Point CalculateContourCentroid(const std::vector<Point> &contour) const;
  bool IsCircularShape(const std::vector<Point> &contour,
                       const std::vector<Point> &approx) const;
  std::vector<Point>
  FindCornersRotationInvariant(const std::vector<Point> &contour) const;
  double CalculateCurvature(const std::vector<Point> &contour, size_t index,
                            int windowSize = 3) const;
  std::vector<Point>
  SmoothContourForRotation(const std::vector<Point> &contour) const;
  std::vector<Point>
  FindRectangleUsingHoughLines(const std::vector<Point> &contour) const;
  std::vector<std::pair<Point, Point>>
  DetectLines(const std::vector<Point> &contour) const;
  bool AreLinesPerpendicular(const std::pair<Point, Point> &line1,
                             const std::pair<Point, Point> &line2,
                             double tolerance = 0.2) const;
  bool IsLikelyCircularContour(const std::vector<Point> &contour) const;
  bool IsRectangleUsingMoments(const std::vector<Point> &contour) const;
  std::vector<Point>
  FindRectangleCornersMomentBased(const std::vector<Point> &contour) const;
  double CalculateHuMoment(const std::vector<Point> &contour, int p,
                           int q) const;
  Point CalculateCentroid(const std::vector<Point> &contour) const;
  double CalculateOrientation(const std::vector<Point> &contour) const;
  std::vector<Point> RotateContourToCanonical(const std::vector<Point> &contour,
                                              double angle) const;
  Image ApplyGaussianBlur(const Image &image, double sigma) const;
  void ProcessContoursAtScale(const std::vector<std::vector<Point>> &contours,
                              std::vector<Rectangle> &rectangles, double scale,
                              const Image &scaledImage);
  void RemoveDuplicateRectangles(std::vector<Rectangle> &rectangles) const;
  Image PreprocessImageEnhanced(const Image &image) const;
  Image PreprocessImageMorphological(const Image &image) const;
  Image PreprocessImageMultiThreshold(const Image &image) const;
  Image PreprocessImageAggressive(const Image &image) const;
  std::vector<Rectangle>
  DetectRectanglesUsingHoughLines(const Image &image) const;
  Image ApplyMorphologyClose(const Image &image, int kernelSize) const;
  Image ApplyMorphologyOpen(const Image &image, int kernelSize) const;
};
