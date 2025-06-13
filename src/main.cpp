#include "shape_detector/rectangle_detector.h"
#include "shape_detector/image_processor.h"
#include <iostream>

int main() {
    std::cout << "Rectangle Detection System\n";
    std::cout << "=========================\n\n";
    
    RectangleDetector detector;
    detector.setMinArea(400.0);  // Reduce to catch smaller rectangles
    detector.setMaxArea(8000.0);
    detector.setApproxEpsilon(0.08);  // Increase tolerance for rotated rectangles
    
    std::cout << "Creating test image with rectangles...\n";
    Image testImage = ImageProcessor::createTestImage(400, 300);
    
    std::cout << "Saving test image...\n";
    ImageProcessor::savePBMImage(testImage, "test_input.pbm");
    
    std::cout << "Detecting rectangles...\n";
    std::vector<Rectangle> rectangles = detector.detectRectangles(testImage);
    
    std::cout << "Found " << rectangles.size() << " rectangles:\n";
    
    for (size_t i = 0; i < rectangles.size(); ++i) {
        const Rectangle& rect = rectangles[i];
        std::cout << "Rectangle " << (i + 1) << ":\n";
        std::cout << "  Center: (" << rect.center.x << ", " << rect.center.y << ")\n";
        std::cout << "  Size: " << rect.width << " x " << rect.height << "\n";
        std::cout << "  Angle: " << rect.angle << " degrees\n";
        std::cout << "  Corners:\n";
        
        for (size_t j = 0; j < rect.corners.size(); ++j) {
            std::cout << "    (" << rect.corners[j].x << ", " << rect.corners[j].y << ")\n";
        }
        std::cout << "\n";
    }
    
    // Create output image showing only rectangle outlines on black background
    Image outputImage(testImage.width, testImage.height);
    
    // Fill with black background
    for (int y = 0; y < outputImage.height; ++y) {
        for (int x = 0; x < outputImage.width; ++x) {
            outputImage.pixels[y][x] = 0;
        }
    }
    
    // Draw white rectangle outlines
    ImageProcessor::drawRectangles(outputImage, rectangles);
    
    std::cout << "Saving output image with detected rectangles...\n";
    ImageProcessor::savePBMImage(outputImage, "test_output.pbm");
    
    std::cout << "Processing complete!\n";
    std::cout << "Input image saved as: test_input.pbm\n";
    std::cout << "Output image saved as: test_output.pbm\n";
    
    return 0;
}