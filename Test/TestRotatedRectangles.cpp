#include "ShapeDetector/RectangleDetector.hpp"
#include "ShapeDetector/ImageProcessor.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <numbers>

class RotatedRectangleTest : public ::testing::Test {
protected:
    void SetUp() override {
        detector.SetMinArea(200.0);
        detector.SetMaxArea(15000.0);
        detector.SetApproxEpsilon(0.02);
    }
    
    RectangleDetector detector;
};

TEST_F(RotatedRectangleTest, DetectsRectangleAt0Degrees) {
    Image image(200, 200);
    // Fill with black
    for (int y = 0; y < 200; ++y) {
        for (int x = 0; x < 200; ++x) {
            image.pixels[y][x] = 0;
        }
    }
    
    // Create rectangle at 0 degrees
    ImageProcessor::CreateRotatedRectangle(image, 100, 100, 80, 50, 0.0);
    
    std::vector<Rectangle> rectangles = detector.DetectRectangles(image);
    EXPECT_GE(rectangles.size(), 1) << "Should detect rectangle at 0 degrees";
}

TEST_F(RotatedRectangleTest, DetectsRectangleAt45Degrees) {
    Image image(200, 200);
    // Fill with black
    for (int y = 0; y < 200; ++y) {
        for (int x = 0; x < 200; ++x) {
            image.pixels[y][x] = 0;
        }
    }
    
    // Create rectangle at 45 degrees
    ImageProcessor::CreateRotatedRectangle(image, 100, 100, 80, 50, std::numbers::pi / 4.0);
    
    std::vector<Rectangle> rectangles = detector.DetectRectangles(image);
    EXPECT_GE(rectangles.size(), 1) << "Should detect rectangle at 45 degrees";
}

TEST_F(RotatedRectangleTest, DetectsRectangleAt90Degrees) {
    Image image(200, 200);
    // Fill with black
    for (int y = 0; y < 200; ++y) {
        for (int x = 0; x < 200; ++x) {
            image.pixels[y][x] = 0;
        }
    }
    
    // Create rectangle at 90 degrees
    ImageProcessor::CreateRotatedRectangle(image, 100, 100, 80, 50, std::numbers::pi / 2.0);
    
    std::vector<Rectangle> rectangles = detector.DetectRectangles(image);
    EXPECT_GE(rectangles.size(), 1) << "Should detect rectangle at 90 degrees";
}

TEST_F(RotatedRectangleTest, DetectsRectangleAt135Degrees) {
    Image image(200, 200);
    // Fill with black
    for (int y = 0; y < 200; ++y) {
        for (int x = 0; x < 200; ++x) {
            image.pixels[y][x] = 0;
        }
    }
    
    // Create rectangle at 135 degrees
    ImageProcessor::CreateRotatedRectangle(image, 100, 100, 80, 50, 3.0 * std::numbers::pi / 4.0);
    
    std::vector<Rectangle> rectangles = detector.DetectRectangles(image);
    EXPECT_GE(rectangles.size(), 1) << "Should detect rectangle at 135 degrees";
}

TEST_F(RotatedRectangleTest, DetectsMultipleRotatedRectangles) {
    Image image(600, 400);
    // Fill with black
    for (int y = 0; y < 400; ++y) {
        for (int x = 0; x < 600; ++x) {
            image.pixels[y][x] = 0;
        }
    }
    
    // Create rectangles at different angles
    std::vector<double> angles = {0.0, std::numbers::pi/6, std::numbers::pi/4, std::numbers::pi/3, std::numbers::pi/2};
    
    for (size_t i = 0; i < angles.size(); ++i) {
        int centerX = 100 + (i * 100);
        int centerY = 200;
        ImageProcessor::CreateRotatedRectangle(image, centerX, centerY, 80, 50, angles[i]);
    }
    
    std::vector<Rectangle> rectangles = detector.DetectRectangles(image);
    
    std::cout << "Multiple rotated rectangles test - detected " << rectangles.size() << " rectangles:" << std::endl;
    for (size_t i = 0; i < rectangles.size(); ++i) {
        std::cout << "  Rectangle " << i << ": center=(" << rectangles[i].center.x << "," 
                  << rectangles[i].center.y << "), size=" << rectangles[i].width << "x" 
                  << rectangles[i].height << ", angle=" << rectangles[i].angle << std::endl;
    }
    
    // We should detect at least 3 out of 5 rectangles (60% success rate)
    EXPECT_GE(rectangles.size(), 3) << "Should detect most rotated rectangles";
}

TEST_F(RotatedRectangleTest, DetectsRectanglesAtAllCommonAngles) {
    // Test rectangles at 15-degree increments
    std::vector<double> testAngles;
    for (int deg = 0; deg < 180; deg += 15) {
        testAngles.push_back(deg * std::numbers::pi / 180.0);
    }
    
    int totalDetected = 0;
    int totalTested = 0;
    
    for (double angle : testAngles) {
        Image image(200, 200);
        // Fill with black
        for (int y = 0; y < 200; ++y) {
            for (int x = 0; x < 200; ++x) {
                image.pixels[y][x] = 0;
            }
        }
        
        ImageProcessor::CreateRotatedRectangle(image, 100, 100, 80, 50, angle);
        
        std::vector<Rectangle> rectangles = detector.DetectRectangles(image);
        totalTested++;
        if (rectangles.size() > 0) {
            totalDetected++;
        }
    }
    
    double detectionRate = static_cast<double>(totalDetected) / totalTested;
    std::cout << "Detection rate across all angles: " << totalDetected << "/" << totalTested 
              << " (" << (detectionRate * 100.0) << "%)" << std::endl;
    
    // We should detect at least 70% of rectangles across all angles
    EXPECT_GE(detectionRate, 0.7) << "Should have good detection rate across all angles";
}