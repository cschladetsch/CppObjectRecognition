#include "shape_detector/image_processor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <ctime>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Image ImageProcessor::loadPBMImage(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filepath << std::endl;
        return Image(0, 0);
    }
    
    std::string format;
    file >> format;
    
    if (format != "P4") {
        std::cerr << "Unsupported format, expected PBM P4: " << format << std::endl;
        return Image(0, 0);
    }
    
    int width, height;
    file >> width >> height;
    file.ignore(); // Skip whitespace after header
    
    Image image(width, height);
    
    int bytesPerRow = (width + 7) / 8;
    
    for (int y = 0; y < height; ++y) {
        for (int byteIdx = 0; byteIdx < bytesPerRow; ++byteIdx) {
            unsigned char byte;
            file.read((char*)&byte, 1);
            
            for (int bit = 7; bit >= 0 && (byteIdx * 8 + (7 - bit)) < width; --bit) {
                int x = byteIdx * 8 + (7 - bit);
                image.pixels[y][x] = ((byte >> bit) & 1) ? 0 : 255; // PBM: 0=white, 1=black
            }
        }
    }
    
    return image;
}

void ImageProcessor::savePBMImage(const Image& image, const std::string& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot create file: " << filepath << std::endl;
        return;
    }
    
    file << "P4\n";
    file << image.width << " " << image.height << "\n";
    
    int bytesPerRow = (image.width + 7) / 8;
    
    for (int y = 0; y < image.height; ++y) {
        for (int byteIdx = 0; byteIdx < bytesPerRow; ++byteIdx) {
            unsigned char byte = 0;
            
            for (int bit = 7; bit >= 0 && (byteIdx * 8 + (7 - bit)) < image.width; --bit) {
                int x = byteIdx * 8 + (7 - bit);
                if (image.pixels[y][x] < 127) { // Black pixel
                    byte |= (1 << bit);
                }
            }
            
            file.write((char*)&byte, 1);
        }
    }
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
        for (size_t i = 0; i < rect.corners.size(); ++i) {
            Point p1 = rect.corners[i];
            Point p2 = rect.corners[(i + 1) % rect.corners.size()];
            
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
    
    // Seed random generator with current time for different results each run
    srand(time(nullptr));
    
    // Create multiple rotated rectangles
    createRotatedRectangle(image, 150, 100, 80, 40, 15.0);  // Rectangle 1: 15 degrees
    createRotatedRectangle(image, 280, 180, 60, 35, -25.0); // Rectangle 2: -25 degrees
    createRotatedRectangle(image, 100, 220, 70, 30, 45.0);  // Rectangle 3: 45 degrees
    
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