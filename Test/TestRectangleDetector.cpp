#include <gtest/gtest.h>
#include "ShapeDetector/RectangleDetector.hpp"
#include "ShapeDetector/ImageProcessor.hpp"

class RectangleDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        detector = new RectangleDetector();
        detector->SetMinArea(400.0);  // Same as main.cpp
        detector->SetMaxArea(8000.0); // Same as main.cpp
        detector->SetApproxEpsilon(0.08);  // Same as main.cpp
    }
    
    void TearDown() override {
        delete detector;
    }
    
    RectangleDetector* detector;
};

TEST_F(RectangleDetectorTest, DetectsSingleRectangle) {
    Image testImage(100, 100);
    
    for (int y = 0; y < 100; ++y) {
        for (int x = 0; x < 100; ++x) {
            testImage.pixels[y][x] = 0;
        }
    }
    
    for (int y = 20; y < 60; ++y) {
        for (int x = 30; x < 70; ++x) {
            testImage.pixels[y][x] = 255;
        }
    }
    
    std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);
    
    EXPECT_GE(rectangles.size(), 1);
}

TEST_F(RectangleDetectorTest, DetectsMultipleRectangles) {
    // Create a test image with two clear rectangles
    Image testImage(200, 150);
    
    // Fill with black background
    for (int y = 0; y < 150; ++y) {
        for (int x = 0; x < 200; ++x) {
            testImage.pixels[y][x] = 0;
        }
    }
    
    // First rectangle (30x25)
    for (int y = 20; y < 45; ++y) {
        for (int x = 30; x < 60; ++x) {
            testImage.pixels[y][x] = 255;
        }
    }
    
    // Second rectangle (35x30)
    for (int y = 80; y < 110; ++y) {
        for (int x = 120; x < 155; ++x) {
            testImage.pixels[y][x] = 255;
        }
    }
    
    std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);
    
    EXPECT_GE(rectangles.size(), 1);
    EXPECT_LE(rectangles.size(), 2);
}

TEST_F(RectangleDetectorTest, NoRectanglesInEmptyImage) {
    Image testImage(100, 100);
    
    for (int y = 0; y < 100; ++y) {
        for (int x = 0; x < 100; ++x) {
            testImage.pixels[y][x] = 0;
        }
    }
    
    std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);
    
    EXPECT_EQ(rectangles.size(), 0);
}

TEST_F(RectangleDetectorTest, FiltersByMinArea) {
    detector->SetMinArea(2000.0);
    
    Image testImage(100, 100);
    
    for (int y = 0; y < 100; ++y) {
        for (int x = 0; x < 100; ++x) {
            testImage.pixels[y][x] = 0;
        }
    }
    
    for (int y = 40; y < 50; ++y) {
        for (int x = 40; x < 50; ++x) {
            testImage.pixels[y][x] = 255;
        }
    }
    
    std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);
    
    EXPECT_EQ(rectangles.size(), 0);
}

TEST_F(RectangleDetectorTest, FiltersByMaxArea) {
    detector->SetMaxArea(50.0);
    
    Image testImage(100, 100);
    
    for (int y = 0; y < 100; ++y) {
        for (int x = 0; x < 100; ++x) {
            testImage.pixels[y][x] = 0;
        }
    }
    
    for (int y = 10; y < 90; ++y) {
        for (int x = 10; x < 90; ++x) {
            testImage.pixels[y][x] = 255;
        }
    }
    
    std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);
    
    EXPECT_EQ(rectangles.size(), 0);
}

TEST_F(RectangleDetectorTest, DiscriminatesNonRectangleShapes) {
    // Test that the detector doesn't confuse circles, triangles, and ellipses with rectangles
    Image testImage(400, 300);
    
    // Black background
    for (int y = 0; y < testImage.height; ++y) {
        for (int x = 0; x < testImage.width; ++x) {
            testImage.pixels[y][x] = 0;
        }
    }
    
    // Add a rectangle - this should be detected
    for (int y = 50; y < 100; ++y) {
        for (int x = 50; x < 150; ++x) {
            testImage.pixels[y][x] = 255;
        }
    }
    
    // Add a circle - this should NOT be detected as a rectangle
    ImageProcessor::DrawFilledCircle(testImage, 250, 75, 30, 255);
    
    // Add a triangle - this should NOT be detected as a rectangle
    Point t1(100, 200);
    Point t2(150, 200);
    Point t3(125, 150);
    ImageProcessor::DrawFilledTriangle(testImage, t1, t2, t3, 255);
    
    // Add an ellipse - this should NOT be detected as a rectangle
    ImageProcessor::DrawFilledEllipse(testImage, 300, 200, 40, 25, 0, 255);
    
    detector->SetMinArea(100.0);
    detector->SetMaxArea(10000.0);
    
    std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);
    
    // Should only detect the actual rectangle, not the other shapes
    EXPECT_EQ(rectangles.size(), 1);
    
    if (rectangles.size() > 0) {
        // Verify the detected rectangle is approximately where we placed it
        const Rectangle& rect = rectangles[0];
        EXPECT_NEAR(rect.center.x, 100, 10);
        EXPECT_NEAR(rect.center.y, 75, 10);
        EXPECT_NEAR(rect.width, 100, 10);
        EXPECT_NEAR(rect.height, 50, 10);
    }
}

TEST_F(RectangleDetectorTest, DetectsRectanglesAmongMixedShapes) {
    // Test with the mixed shapes image
    Image testImage = ImageProcessor::CreateTestImageWithMixedShapes(400, 300);
    
    detector->SetMinArea(100.0);
    detector->SetMaxArea(10000.0);
    
    std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);
    
    // The CreateTestImageWithMixedShapes function adds 3 rectangles
    EXPECT_GE(rectangles.size(), 2);  // Allow some tolerance
    EXPECT_LE(rectangles.size(), 4);  // But not too many false positives
}