#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

#include "rectangle_detector.h"
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
    static Image loadPGMImage(const std::string& filepath);
    static void savePGMImage(const Image& image, const std::string& filepath);
    static void savePPMImage(const ColorImage& image, const std::string& filepath);
    static ColorImage createColorImage(const Image& grayImage, const std::vector<Rectangle>& rectangles);
    static Image applyThreshold(const Image& image, int threshold = 127);
    static Image applyGaussianBlur(const Image& image, int kernelSize = 5);
    static void drawRectangles(Image& image, const std::vector<Rectangle>& rectangles);
    static Image createTestImage(int width, int height);
    
private:
    static std::vector<std::vector<double>> createGaussianKernel(int size);
    static void createRotatedRectangle(Image& image, int centerX, int centerY, int rectWidth, int rectHeight, double angleDegrees);
    static void fillRotatedRectangle(Image& image, const std::vector<std::pair<int, int>>& corners);
    static bool isPointInPolygon(int x, int y, const std::vector<std::pair<int, int>>& polygon);
    static std::vector<Point> cleanupRectangleCorners(const std::vector<Point>& corners);
    static std::vector<Point> generateRectangleCorners(const Rectangle& rect);
    static void drawLine(Image& image, const Point& p1, const Point& p2);
    static void drawColorLine(ColorImage& image, const Point& p1, const Point& p2, const ColorPixel& color);
};

#endif // IMAGE_PROCESSOR_H