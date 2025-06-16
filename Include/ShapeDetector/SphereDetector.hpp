#pragma once

#include "RectangleDetector.hpp"
#include <vector>
#include <cmath>

// Ensure Sphere is defined here since it's used in function signatures
#ifndef SPHERE_DEFINED
#define SPHERE_DEFINED
// Sphere struct is already defined in RectangleDetector.hpp
#endif

class ObloidDetector {
public:
  ObloidDetector();
  ~ObloidDetector();

  std::vector<Obloid> DetectObloids(const Image &image);
  void SetMinRadius(int minRadius);
  void SetMaxRadius(int maxRadius);
  void SetCircularityThreshold(double threshold);
  void SetConfidenceThreshold(double threshold);

private:
  int minRadius_;
  int maxRadius_;
  double circularityThreshold_;
  double confidenceThreshold_;

  // Cache for expensive calculations
  mutable std::vector<double> distanceCache_;
  mutable std::vector<double> radiusCache_;

  std::vector<std::vector<Point>> FindContours(const Image &image) const;
  bool IsObloid(const std::vector<Point> &contour, Obloid &obloid) const;
  Obloid CreateObloid(const std::vector<Point> &contour) const;
  Image PreprocessImage(const Image &image) const;
  double CalculateCircularity(const std::vector<Point> &contour) const;
  double CalculatePerimeter(const std::vector<Point> &contour) const;
  double CalculateArea(const std::vector<Point> &contour) const;
  Point CalculateCentroid(const std::vector<Point> &contour) const;
  int EstimateRadius(const std::vector<Point> &contour, const Point &center) const;
  double CalculateRadialVariance(const std::vector<Point> &contour, const Point &center, int radius) const;
  bool IsCircularContour(const std::vector<Point> &contour) const;
  void ScanlineFillContour(const Image &image, int startX, int startY,
                           std::vector<Point> &contour,
                           std::vector<std::vector<bool>> &visited) const;
  std::vector<Point> ExtractBoundary(const std::vector<Point> &region,
                                     const Image &image) const;
  void RemoveDuplicateObloids(std::vector<Obloid> &obloids) const;
  Image ApplyGaussianBlur(const Image &image, double sigma) const;
  bool ValidateCircleGeometry(const std::vector<Point> &contour, const Point &center, int radius) const;
  double CalculateCircleFitError(const std::vector<Point> &contour, const Point &center, int radius) const;
  Obloid FitCircleToContour(const std::vector<Point> &contour) const;
};

class SphereDetector {
public:
  SphereDetector();
  ~SphereDetector();

  std::vector<Sphere> DetectSpheres(const Image &image);
  void SetMinRadius(int minRadius);
  void SetMaxRadius(int maxRadius);
  void SetCircularityThreshold(double threshold);
  void SetConfidenceThreshold(double threshold);

private:
  int minRadius_;
  int maxRadius_;
  double circularityThreshold_;
  double confidenceThreshold_;

  // Cache for expensive calculations
  mutable std::vector<double> distanceCache_;
  mutable std::vector<double> radiusCache_;

  std::vector<std::vector<Point>> FindContours(const Image &image) const;
  bool IsSphere(const std::vector<Point> &contour, Sphere &sphere) const;
  Sphere CreateSphere(const std::vector<Point> &contour) const;
  Image PreprocessImage(const Image &image) const;
  double CalculateCircularity(const std::vector<Point> &contour) const;
  double CalculatePerimeter(const std::vector<Point> &contour) const;
  double CalculateArea(const std::vector<Point> &contour) const;
  Point CalculateCentroid(const std::vector<Point> &contour) const;
  int EstimateRadius(const std::vector<Point> &contour, const Point &center) const;
  double CalculateRadialVariance(const std::vector<Point> &contour, const Point &center, int radius) const;
  bool IsCircularContour(const std::vector<Point> &contour) const;
  void ScanlineFillContour(const Image &image, int startX, int startY,
                           std::vector<Point> &contour,
                           std::vector<std::vector<bool>> &visited) const;
  std::vector<Point> ExtractBoundary(const std::vector<Point> &region,
                                     const Image &image) const;
  void RemoveDuplicateSpheres(std::vector<Sphere> &spheres) const;
  Image ApplyGaussianBlur(const Image &image, double sigma) const;
  bool ValidateCircleGeometry(const std::vector<Point> &contour, const Point &center, int radius) const;
  double CalculateCircleFitError(const std::vector<Point> &contour, const Point &center, int radius) const;
  Sphere FitCircleToContour(const std::vector<Point> &contour) const;
};