#include "ShapeDetector/ImageProcessor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <random>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Image ImageProcessor::loadPGMImage(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filepath << std::endl;
        return Image(0, 0);
    }
    
    std::string format;
    file >> format;
    
    if (format != "P5") {
        std::cerr << "Unsupported format, expected PGM P5: " << format << std::endl;
        return Image(0, 0);
    }
    
    int width, height, maxval;
    file >> width >> height >> maxval;
    file.ignore(); // Skip whitespace after header
    
    Image image(width, height);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned char pixel;
            file.read((char*)&pixel, 1);
            image.pixels[y][x] = static_cast<int>(pixel);
        }
    }
    
    return image;
}


void ImageProcessor::savePGMImage(const Image& image, const std::string& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot create file: " << filepath << std::endl;
        return;
    }
    
    file << "P5\n";
    file << image.width << " " << image.height << "\n";
    file << "255\n";  // Maximum grey value
    
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            unsigned char pixel = static_cast<unsigned char>(image.pixels[y][x]);
            file.write((char*)&pixel, 1);
        }
    }
}

void ImageProcessor::savePPMImage(const ColorImage& image, const std::string& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot create file: " << filepath << std::endl;
        return;
    }
    
    file << "P6\n";
    file << image.width << " " << image.height << "\n";
    file << "255\n";  // Maximum color value
    
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const ColorPixel& pixel = image.pixels[y][x];
            file.write((char*)&pixel.r, 1);
            file.write((char*)&pixel.g, 1);
            file.write((char*)&pixel.b, 1);
        }
    }
}

ColorImage ImageProcessor::createColorImage(const Image& grayImage, const std::vector<Rectangle>& rectangles) {
    ColorImage colorImage(grayImage.width, grayImage.height);
    
    // Convert grayscale to color (white pixels become grey, black stays black)
    for (int y = 0; y < grayImage.height; ++y) {
        for (int x = 0; x < grayImage.width; ++x) {
            unsigned char grayValue = static_cast<unsigned char>(grayImage.pixels[y][x]);
            if (grayValue == 255) {
                // Convert white rectangles to medium grey
                colorImage.pixels[y][x] = ColorPixel(128, 128, 128);
            } else {
                // Keep black background as black
                colorImage.pixels[y][x] = ColorPixel(grayValue, grayValue, grayValue);
            }
        }
    }
    
    // Draw red rectangle boundaries
    for (const auto& rect : rectangles) {
        std::vector<Point> corners = generateRectangleCorners(rect);
        
        // Draw lines between consecutive corners in red
        for (size_t i = 0; i < 4; ++i) {
            Point p1 = corners[i];
            Point p2 = corners[(i + 1) % 4];
            
            drawColorLine(colorImage, p1, p2, ColorPixel(255, 0, 0)); // Red color
        }
    }
    
    return colorImage;
}


Image ImageProcessor::applyThreshold(const Image& image, int threshold) {
    Image result = image;
    
    for (int y = 0; y < result.height; ++y) {
        for (int x = 0; x < result.width; ++x) {
            result.pixels[y][x] = (result.pixels[y][x] > threshold) ? 255 : 0;
        }
    }
    
    return result;
}

Image ImageProcessor::applyGaussianBlur(const Image& image, int kernelSize) {
    if (kernelSize % 2 == 0) kernelSize++;
    
    std::vector<std::vector<double>> kernel = createGaussianKernel(kernelSize);
    Image result(image.width, image.height);
    
    int halfKernel = kernelSize / 2;
    
    for (int y = halfKernel; y < image.height - halfKernel; ++y) {
        for (int x = halfKernel; x < image.width - halfKernel; ++x) {
            double sum = 0.0;
            
            for (int ky = -halfKernel; ky <= halfKernel; ++ky) {
                for (int kx = -halfKernel; kx <= halfKernel; ++kx) {
                    sum += image.pixels[y + ky][x + kx] * kernel[ky + halfKernel][kx + halfKernel];
                }
            }
            
            result.pixels[y][x] = static_cast<int>(sum);
        }
    }
    
    return result;
}

void ImageProcessor::drawRectangles(Image& image, const std::vector<Rectangle>& rectangles) {
    for (const auto& rect : rectangles) {
        // Generate corners from center, width, height, and angle
        std::vector<Point> corners = generateRectangleCorners(rect);
        
        // Draw lines between consecutive corners
        for (size_t i = 0; i < 4; ++i) {
            Point p1 = corners[i];
            Point p2 = corners[(i + 1) % 4];
            
            drawLine(image, p1, p2);
        }
    }
}

std::vector<Point> ImageProcessor::generateRectangleCorners(const Rectangle& rect) {
    std::vector<Point> corners;
    
    double angleRad = rect.angle * M_PI / 180.0;
    double cosAngle = std::cos(angleRad);
    double sinAngle = std::sin(angleRad);
    
    // Half dimensions
    double halfW = rect.width / 2.0;
    double halfH = rect.height / 2.0;
    
    // Generate 4 corners relative to center
    std::vector<std::pair<double, double>> relativeCorners = {
        {-halfW, -halfH},  // Top-left
        {halfW, -halfH},   // Top-right
        {halfW, halfH},    // Bottom-right
        {-halfW, halfH}    // Bottom-left
    };
    
    // Rotate and translate to world coordinates
    for (const auto& corner : relativeCorners) {
        double rotatedX = corner.first * cosAngle - corner.second * sinAngle + rect.center.x;
        double rotatedY = corner.first * sinAngle + corner.second * cosAngle + rect.center.y;
        corners.push_back(Point(static_cast<int>(rotatedX), static_cast<int>(rotatedY)));
    }
    
    return corners;
}

std::vector<Point> ImageProcessor::cleanupRectangleCorners(const std::vector<Point>& corners) {
    if (corners.empty()) return corners;
    
    std::vector<Point> cleaned;
    const double minDistance = 3.0; // Minimum distance between corners
    
    for (const auto& corner : corners) {
        bool keepPoint = true;
        
        // Check if this point is too close to any already kept point
        for (const auto& kept : cleaned) {
            double dx = corner.x - kept.x;
            double dy = corner.y - kept.y;
            double dist = std::sqrt(dx * dx + dy * dy);
            
            if (dist < minDistance) {
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

void ImageProcessor::drawLine(Image& image, const Point& p1, const Point& p2) {
    // Bresenham's line algorithm
    int dx = std::abs(p2.x - p1.x);
    int dy = std::abs(p2.y - p1.y);
    int sx = (p1.x < p2.x) ? 1 : -1;
    int sy = (p1.y < p2.y) ? 1 : -1;
    int err = dx - dy;
    
    int x = p1.x, y = p1.y;
    
    while (true) {
        if (x >= 0 && x < image.width && y >= 0 && y < image.height) {
            image.pixels[y][x] = 255;
        }
        
        if (x == p2.x && y == p2.y) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

void ImageProcessor::drawColorLine(ColorImage& image, const Point& p1, const Point& p2, const ColorPixel& color) {
    // Bresenham's line algorithm for color images
    int dx = std::abs(p2.x - p1.x);
    int dy = std::abs(p2.y - p1.y);
    int sx = (p1.x < p2.x) ? 1 : -1;
    int sy = (p1.y < p2.y) ? 1 : -1;
    int err = dx - dy;
    
    int x = p1.x, y = p1.y;
    
    while (true) {
        if (x >= 0 && x < image.width && y >= 0 && y < image.height) {
            image.pixels[y][x] = color;
        }
        
        if (x == p2.x && y == p2.y) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

Image ImageProcessor::createTestImage(int width, int height) {
    Image image(width, height);
    
    // Fill with black background
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            image.pixels[y][x] = 0;
        }
    }
    
    // Modern C++ random number generation
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    // Generate non-overlapping rectangles
    struct RectSpec {
        int centerX, centerY, width, height;
        double angle;
    };
    
    std::vector<RectSpec> rectangles;
    std::uniform_int_distribution<int> numRectDist(3, 5); // 3-5 rectangles
    int numRectangles = numRectDist(gen);
    int maxAttempts = 20; // Maximum attempts to place each rectangle
    
    std::uniform_int_distribution<int> widthDist(40, 99);   // 40-99 pixels
    std::uniform_int_distribution<int> heightDist(25, 64);  // 25-64 pixels
    std::uniform_real_distribution<double> angleDist(-180.0, 180.0); // -180 to 180 degrees
    
    for (int i = 0; i < numRectangles; ++i) {
        bool placed = false;
        
        for (int attempt = 0; attempt < maxAttempts && !placed; ++attempt) {
            // Random dimensions
            int rectWidth = widthDist(gen);
            int rectHeight = heightDist(gen);
            
            // For rotated rectangles, use diagonal as margin to prevent clipping
            double diagonal = std::sqrt(rectWidth * rectWidth + rectHeight * rectHeight);
            int margin = static_cast<int>(diagonal / 2) + 30; // Diagonal-based margin
            
            // Ensure margin doesn't exceed image boundaries
            margin = std::min(margin, width / 3);
            margin = std::min(margin, height / 3);
            
            // Random position within valid bounds
            std::uniform_int_distribution<int> xDist(margin, width - margin - 1);
            std::uniform_int_distribution<int> yDist(margin, height - margin - 1);
            int centerX = xDist(gen);
            int centerY = yDist(gen);
            
            // Random angle
            double angle = angleDist(gen);
            
            // Check for overlap with existing rectangles
            bool overlaps = false;
            for (const auto& existing : rectangles) {
                // Simple distance-based check with generous margin
                int dx = centerX - existing.centerX;
                int dy = centerY - existing.centerY;
                double distance = std::sqrt(dx * dx + dy * dy);
                double minDistance = (rectWidth + rectHeight + existing.width + existing.height) / 2 + 30;
                
                if (distance < minDistance) {
                    overlaps = true;
                    break;
                }
            }
            
            if (!overlaps) {
                rectangles.push_back({centerX, centerY, rectWidth, rectHeight, angle});
                placed = true;
            }
        }
    }
    
    // Draw all non-overlapping rectangles
    for (const auto& rect : rectangles) {
        createRotatedRectangle(image, rect.centerX, rect.centerY, rect.width, rect.height, rect.angle);
    }
    
    return image;
}

void ImageProcessor::createRotatedRectangle(Image& image, int centerX, int centerY, 
                                          int rectWidth, int rectHeight, double angleDegrees) {
    double angleRad = angleDegrees * M_PI / 180.0;
    double cosAngle = std::cos(angleRad);
    double sinAngle = std::sin(angleRad);
    
    // Define rectangle corners relative to center
    std::vector<std::pair<double, double>> corners = {
        {-rectWidth/2.0, -rectHeight/2.0},  // Top-left
        {rectWidth/2.0, -rectHeight/2.0},   // Top-right
        {rectWidth/2.0, rectHeight/2.0},    // Bottom-right
        {-rectWidth/2.0, rectHeight/2.0}    // Bottom-left
    };
    
    // Rotate and translate corners
    std::vector<std::pair<int, int>> rotatedCorners;
    for (const auto& corner : corners) {
        double rotatedX = corner.first * cosAngle - corner.second * sinAngle + centerX;
        double rotatedY = corner.first * sinAngle + corner.second * cosAngle + centerY;
        rotatedCorners.push_back({static_cast<int>(rotatedX), static_cast<int>(rotatedY)});
    }
    
    // Fill the rotated rectangle using scan line algorithm
    fillRotatedRectangle(image, rotatedCorners);
}

void ImageProcessor::fillRotatedRectangle(Image& image, const std::vector<std::pair<int, int>>& corners) {
    if (corners.size() != 4) return;
    
    // Find bounding box
    int minX = corners[0].first, maxX = corners[0].first;
    int minY = corners[0].second, maxY = corners[0].second;
    
    for (const auto& corner : corners) {
        minX = std::min(minX, corner.first);
        maxX = std::max(maxX, corner.first);
        minY = std::min(minY, corner.second);
        maxY = std::max(maxY, corner.second);
    }
    
    // Ensure bounds are within image
    minX = std::max(0, minX);
    minY = std::max(0, minY);
    maxX = std::min(image.width - 1, maxX);
    maxY = std::min(image.height - 1, maxY);
    
    // For each point in bounding box, check if it's inside the rectangle
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            if (isPointInPolygon(x, y, corners)) {
                image.pixels[y][x] = 255;
            }
        }
    }
}

bool ImageProcessor::isPointInPolygon(int x, int y, const std::vector<std::pair<int, int>>& polygon) {
    int n = polygon.size();
    bool inside = false;
    
    for (int i = 0, j = n - 1; i < n; j = i++) {
        int xi = polygon[i].first, yi = polygon[i].second;
        int xj = polygon[j].first, yj = polygon[j].second;
        
        if (((yi > y) != (yj > y)) && (x < (xj - xi) * (y - yi) / (yj - yi) + xi)) {
            inside = !inside;
        }
    }
    
    return inside;
}

std::vector<std::vector<double>> ImageProcessor::createGaussianKernel(int size) {
    std::vector<std::vector<double>> kernel(size, std::vector<double>(size));
    double sigma = size / 3.0;
    double sum = 0.0;
    int center = size / 2;
    
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            int x = i - center;
            int y = j - center;
            kernel[i][j] = std::exp(-(x * x + y * y) / (2 * sigma * sigma));
            sum += kernel[i][j];
        }
    }
    
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            kernel[i][j] /= sum;
        }
    }
    
    return kernel;
}