#include <gtest/gtest.h>
#include "shape_detector/rectangle_detector.h"

class GeometryTest : public ::testing::Test {
protected:
    void SetUp() override {
        detector = new RectangleDetector();
    }
    
    void TearDown() override {
        delete detector;
    }
    
    RectangleDetector* detector;
};

TEST_F(GeometryTest, PointConstructorWorks) {
    Point p1;
    EXPECT_EQ(p1.x, 0);
    EXPECT_EQ(p1.y, 0);
    
    Point p2(10, 20);
    EXPECT_EQ(p2.x, 10);
    EXPECT_EQ(p2.y, 20);
}

TEST_F(GeometryTest, ImageConstructorWorks) {
    Image img(100, 80);
    
    EXPECT_EQ(img.width, 100);
    EXPECT_EQ(img.height, 80);
    EXPECT_EQ(img.pixels.size(), 80);
    EXPECT_EQ(img.pixels[0].size(), 100);
    
    // Check that all pixels are initialized to 0
    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            EXPECT_EQ(img.pixels[y][x], 0);
        }
    }
}

TEST_F(GeometryTest, RectangleHasCorrectFields) {
    Rectangle rect;
    
    // Test that Rectangle has all expected fields
    rect.center = Point(50, 50);
    rect.width = 100;
    rect.height = 80;
    rect.angle = 45.0;
    
    EXPECT_EQ(rect.center.x, 50);
    EXPECT_EQ(rect.center.y, 50);
    EXPECT_EQ(rect.width, 100);
    EXPECT_EQ(rect.height, 80);
    EXPECT_DOUBLE_EQ(rect.angle, 45.0);
}

TEST_F(GeometryTest, DetectorSettingsWork) {
    detector->setMinArea(100.0);
    detector->setMaxArea(2000.0);
    detector->setApproxEpsilon(0.05);
    
    // If settings are stored correctly, they should affect detection
    // We can't directly test private members, but we can test behavior
    Image smallImage(20, 20);
    
    // Fill with black
    for (int y = 0; y < 20; ++y) {
        for (int x = 0; x < 20; ++x) {
            smallImage.pixels[y][x] = 0;
        }
    }
    
    // Add a tiny rectangle (area < minArea)
    for (int y = 8; y < 12; ++y) {
        for (int x = 8; x < 12; ++x) {
            smallImage.pixels[y][x] = 255;
        }
    }
    
    std::vector<Rectangle> rectangles = detector->detectRectangles(smallImage);
    
    // Should not detect the small rectangle due to minArea constraint
    EXPECT_EQ(rectangles.size(), 0);
}