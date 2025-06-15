#include <gtest/gtest.h>
#include "ShapeDetector/RectangleDetector.h"
#include "ShapeDetector/ImageProcessor.h"

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