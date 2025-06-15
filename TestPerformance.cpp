#include <iostream>
#include <chrono>
#include <vector>
#include "ShapeDetector/RectangleDetector.h"
#include "ShapeDetector/ImageProcessor.h"

using namespace std::chrono;

void testPerformance() {
    std::cout << "Performance Test for Rectangle Detection\n";
    std::cout << "========================================\n\n";
    
    // Test with different image sizes
    std::vector<int> sizes = {100, 200, 400, 800, 1600};
    
    for (int size : sizes) {
        std::cout << "Testing with image size: " << size << "x" << size << "\n";
        
        // Create test image with multiple rectangles
        Image testImage = ImageProcessor::createTestImage(size, size);
        
        // Create detector
        RectangleDetector detector;
        detector.setMinArea(100.0);
        detector.setMaxArea(size * size * 0.5);
        
        // Measure detection time
        auto start = high_resolution_clock::now();
        std::vector<Rectangle> rectangles = detector.detectRectangles(testImage);
        auto end = high_resolution_clock::now();
        
        auto duration = duration_cast<milliseconds>(end - start);
        
        std::cout << "  - Detected " << rectangles.size() << " rectangles\n";
        std::cout << "  - Time taken: " << duration.count() << " ms\n";
        std::cout << "  - Processing rate: " << (size * size) / (duration.count() + 1) << " pixels/ms\n\n";
    }
    
    // Test with a complex image (many small rectangles)
    std::cout << "Testing with complex image (many small rectangles)...\n";
    Image complexImage(1000, 1000);
    
    // Create grid of small rectangles
    for (int y = 10; y < 990; y += 50) {
        for (int x = 10; x < 990; x += 50) {
            // Draw small rectangle
            for (int dy = 0; dy < 30; dy++) {
                for (int dx = 0; dx < 30; dx++) {
                    if (dy == 0 || dy == 29 || dx == 0 || dx == 29) {
                        complexImage.pixels[y + dy][x + dx] = 255;
                    }
                }
            }
        }
    }
    
    RectangleDetector detector;
    detector.setMinArea(50.0);
    detector.setMaxArea(10000.0);
    
    auto start = high_resolution_clock::now();
    std::vector<Rectangle> rectangles = detector.detectRectangles(complexImage);
    auto end = high_resolution_clock::now();
    
    auto duration = duration_cast<milliseconds>(end - start);
    
    std::cout << "  - Detected " << rectangles.size() << " rectangles\n";
    std::cout << "  - Time taken: " << duration.count() << " ms\n";
    std::cout << "  - Average time per rectangle: " << (rectangles.size() > 0 ? duration.count() / rectangles.size() : 0) << " ms\n";
}

int main() {
    testPerformance();
    return 0;
}