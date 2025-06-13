#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

#include "rectangle_detector.h"
#include <string>
#include <fstream>

class ImageProcessor {
public:
    static Image loadPBMImage(const std::string& filepath);
    static void savePBMImage(const Image& image, const std::string& filepath);
    static Image applyThreshold(const Image& image, int threshold = 127);
    static Image applyGaussianBlur(const Image& image, int kernelSize = 5);
    static void drawRectangles(Image& image, const std::vector<Rectangle>& rectangles);
    static Image createTestImage(int width, int height);
    
private:
    static std::vector<std::vector<double>> createGaussianKernel(int size);
    static void createRotatedRectangle(Image& image, int centerX, int centerY, int rectWidth, int rectHeight, double angleDegrees);
    static void fillRotatedRectangle(Image& image, const std::vector<std::pair<int, int>>& corners);
    static bool isPointInPolygon(int x, int y, const std::vector<std::pair<int, int>>& polygon);
};

#endif // IMAGE_PROCESSOR_H