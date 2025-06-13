#include <gtest/gtest.h>
#include "shape_detector/image_processor.h"
#include <fstream>

class ImageProcessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple test PBM file
        createTestPBMFile("test_input.pbm", 8, 6);
    }
    
    void TearDown() override {
        std::remove("test_input.pbm");
        std::remove("test_output.pbm");
    }
    
    void createTestPBMFile(const std::string& filename, int width, int height) {
        std::ofstream file(filename, std::ios::binary);
        file << "P4\n" << width << " " << height << "\n";
        
        // Create a simple pattern: alternating black/white pixels
        unsigned char data[] = {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA};
        file.write((char*)data, 6);
    }
};

TEST_F(ImageProcessorTest, LoadsPBMImageCorrectly) {
    Image image = ImageProcessor::loadPBMImage("test_input.pbm");
    
    EXPECT_EQ(image.width, 8);
    EXPECT_EQ(image.height, 6);
    EXPECT_EQ(image.pixels.size(), 6);
    EXPECT_EQ(image.pixels[0].size(), 8);
}

TEST_F(ImageProcessorTest, SavesPBMImageCorrectly) {
    Image testImage(4, 4);
    
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 4; ++x) {
            testImage.pixels[y][x] = (x + y) % 2 == 0 ? 255 : 0;
        }
    }
    
    ImageProcessor::savePBMImage(testImage, "test_output.pbm");
    
    std::ifstream file("test_output.pbm", std::ios::binary);
    EXPECT_TRUE(file.is_open());
    
    std::string line;
    std::getline(file, line);
    EXPECT_EQ(line, "P4");
    
    std::getline(file, line);
    EXPECT_EQ(line, "4 4");
}

TEST_F(ImageProcessorTest, AppliesThresholdCorrectly) {
    Image testImage(3, 3);
    
    int values[] = {0, 50, 100, 150, 200, 255, 75, 125, 175};
    int idx = 0;
    
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            testImage.pixels[y][x] = values[idx++];
        }
    }
    
    Image result = ImageProcessor::applyThreshold(testImage, 127);
    
    EXPECT_EQ(result.pixels[0][0], 0);   // 0 < 127
    EXPECT_EQ(result.pixels[0][1], 0);   // 50 < 127
    EXPECT_EQ(result.pixels[0][2], 0);   // 100 < 127
    EXPECT_EQ(result.pixels[1][0], 255); // 150 > 127
    EXPECT_EQ(result.pixels[1][1], 255); // 200 > 127
    EXPECT_EQ(result.pixels[1][2], 255); // 255 > 127
}

TEST_F(ImageProcessorTest, CreatesTestImageWithCorrectDimensions) {
    Image testImage = ImageProcessor::createTestImage(100, 80);
    
    EXPECT_EQ(testImage.width, 100);
    EXPECT_EQ(testImage.height, 80);
    EXPECT_EQ(testImage.pixels.size(), 80);
    EXPECT_EQ(testImage.pixels[0].size(), 100);
}

TEST_F(ImageProcessorTest, TestImageHasWhiteRectanglesOnBlackBackground) {
    Image testImage = ImageProcessor::createTestImage(100, 80);
    
    bool hasBlackPixels = false;
    bool hasWhitePixels = false;
    
    for (int y = 0; y < testImage.height; ++y) {
        for (int x = 0; x < testImage.width; ++x) {
            if (testImage.pixels[y][x] == 0) hasBlackPixels = true;
            if (testImage.pixels[y][x] == 255) hasWhitePixels = true;
        }
    }
    
    EXPECT_TRUE(hasBlackPixels);
    EXPECT_TRUE(hasWhitePixels);
}

TEST_F(ImageProcessorTest, DrawsRectanglesOnImage) {
    Image testImage(50, 50);
    
    for (int y = 0; y < 50; ++y) {
        for (int x = 0; x < 50; ++x) {
            testImage.pixels[y][x] = 255;
        }
    }
    
    std::vector<Rectangle> rectangles;
    Rectangle rect;
    rect.corners = {Point(10, 10), Point(20, 10), Point(20, 20), Point(10, 20)};
    rectangles.push_back(rect);
    
    ImageProcessor::drawRectangles(testImage, rectangles);
    
    // Check that some pixels were modified to draw the rectangle outline
    bool hasDrawnPixels = false;
    for (int y = 0; y < 50; ++y) {
        for (int x = 0; x < 50; ++x) {
            if (testImage.pixels[y][x] == 128) {
                hasDrawnPixels = true;
                break;
            }
        }
        if (hasDrawnPixels) break;
    }
    
    EXPECT_TRUE(hasDrawnPixels);
}