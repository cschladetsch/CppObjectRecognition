#pragma once

#include "RectangleDetector.hpp"
#include <string>
#include <fstream>

struct ColorPixel {
    unsigned char r, g, b;
    ColorPixel(unsigned char red = 0, unsigned char green = 0, unsigned char blue = 0) 
        : r(red), g(green), b(blue) {}
};

struct ColorImage {
    int width, height;
    std::vector<std::vector<ColorPixel>> pixels;
    
    ColorImage(int w, int h) : width(w), height(h) {
        pixels.resize(height, std::vector<ColorPixel>(width));
    }
};

class ImageProcessor {
public:
    static Image LoadPGMImage(const std::string& filepath);
    static void SavePGMImage(const Image& image, const std::string& filepath);
    static void SavePPMImage(const ColorImage& image, const std::string& filepath);
    static ColorImage CreateColorImage(const Image& grayImage, const std::vector<Rectangle>& rectangles);
    static Image ApplyThreshold(const Image& image, int threshold = 127);
    static Image ApplyGaussianBlur(const Image& image, int kernelSize = 5);
    static void DrawRectangles(Image& image, const std::vector<Rectangle>& rectangles);
    static Image CreateTestImage(int width, int height);
    static Image CreateTestImageWithMixedShapes(int width, int height);
    static void DrawCircle(Image& image, int centerX, int centerY, int radius, int color = 255);
    static void DrawFilledCircle(Image& image, int centerX, int centerY, int radius, int color = 255);
    static void DrawTriangle(Image& image, const Point& p1, const Point& p2, const Point& p3, int color = 255);
    static void DrawFilledTriangle(Image& image, const Point& p1, const Point& p2, const Point& p3, int color = 255);
    static void DrawEllipse(Image& image, int centerX, int centerY, int radiusX, int radiusY, double angle = 0.0, int color = 255);
    static void DrawFilledEllipse(Image& image, int centerX, int centerY, int radiusX, int radiusY, double angle = 0.0, int color = 255);
    
private:
    static std::vector<std::vector<double>> CreateGaussianKernel(int size);
    static void CreateRotatedRectangle(Image& image, int centerX, int centerY, int rectWidth, int rectHeight, double angleRadians);
    static void FillRotatedRectangle(Image& image, const std::vector<std::pair<int, int>>& corners);
    static bool IsPointInPolygon(int x, int y, const std::vector<std::pair<int, int>>& polygon);
    static std::vector<Point> CleanupRectangleCorners(const std::vector<Point>& corners);
    static std::vector<Point> GenerateRectangleCorners(const Rectangle& rect);
    static void DrawLine(Image& image, const Point& p1, const Point& p2);
    static void DrawColorLine(ColorImage& image, const Point& p1, const Point& p2, const ColorPixel& color);
};

