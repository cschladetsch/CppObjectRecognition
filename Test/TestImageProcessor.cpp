#include "ShapeDetector/ImageProcessor.hpp"
#include <fstream>
#include <gtest/gtest.h>

class ImageProcessorTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create a simple test PGM file
    createTestPGMFile("test_input.pgm", 8, 6);
  }

  void TearDown() override {
    std::remove("test_input.pgm");
    std::remove("test_output.pgm");
  }

  void createTestPGMFile(const std::string &filename, int width, int height) {
    std::ofstream file(filename, std::ios::binary);
    file << "P5\n" << width << " " << height << "\n255\n";

    // Create a simple pattern with greyscale values
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        unsigned char pixel = ((x + y) % 2 == 0) ? 255 : 0;
        file.write((char *)&pixel, 1);
      }
    }
  }
};

TEST_F(ImageProcessorTest, LoadsPGMImageCorrectly) {
  Image image = ImageProcessor::LoadPGMImage("test_input.pgm");

  EXPECT_EQ(image.width, 8);
  EXPECT_EQ(image.height, 6);
  EXPECT_EQ(image.pixels.size(), 6);
  EXPECT_EQ(image.pixels[0].size(), 8);
}

TEST_F(ImageProcessorTest, SavesPGMImageCorrectly) {
  Image testImage(4, 4);

  for (int y = 0; y < 4; ++y) {
    for (int x = 0; x < 4; ++x) {
      testImage.pixels[y][x] = (x + y) % 2 == 0 ? 255 : 128;
    }
  }

  ImageProcessor::SavePGMImage(testImage, "test_output.pgm");

  std::ifstream file("test_output.pgm", std::ios::binary);
  EXPECT_TRUE(file.is_open());

  std::string line;
  std::getline(file, line);
  EXPECT_EQ(line, "P5");

  std::getline(file, line);
  EXPECT_EQ(line, "4 4");

  std::getline(file, line);
  EXPECT_EQ(line, "255");
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

  Image result = ImageProcessor::ApplyThreshold(testImage, 127);

  EXPECT_EQ(result.pixels[0][0], 0);   // 0 < 127
  EXPECT_EQ(result.pixels[0][1], 0);   // 50 < 127
  EXPECT_EQ(result.pixels[0][2], 0);   // 100 < 127
  EXPECT_EQ(result.pixels[1][0], 255); // 150 > 127
  EXPECT_EQ(result.pixels[1][1], 255); // 200 > 127
  EXPECT_EQ(result.pixels[1][2], 255); // 255 > 127
}

TEST_F(ImageProcessorTest, CreatesTestImageWithCorrectDimensions) {
  Image testImage = ImageProcessor::CreateTestImage(100, 80);

  EXPECT_EQ(testImage.width, 100);
  EXPECT_EQ(testImage.height, 80);
  EXPECT_EQ(testImage.pixels.size(), 80);
  EXPECT_EQ(testImage.pixels[0].size(), 100);
}

TEST_F(ImageProcessorTest, TestImageHasWhiteRectanglesOnBlackBackground) {
  Image testImage = ImageProcessor::CreateTestImage(100, 80);

  bool hasBlackPixels = false;
  bool hasWhitePixels = false;

  for (int y = 0; y < testImage.height; ++y) {
    for (int x = 0; x < testImage.width; ++x) {
      if (testImage.pixels[y][x] == 0)
        hasBlackPixels = true;
      if (testImage.pixels[y][x] == 255)
        hasWhitePixels = true;
    }
  }

  EXPECT_TRUE(hasBlackPixels);
  EXPECT_TRUE(hasWhitePixels);
}

TEST_F(ImageProcessorTest, DrawsRectanglesOnImage) {
  Image testImage(50, 50);

  for (int y = 0; y < 50; ++y) {
    for (int x = 0; x < 50; ++x) {
      testImage.pixels[y][x] = 0; // Black background
    }
  }

  std::vector<Rectangle> rectangles;
  Rectangle rect;
  rect.center = Point(15, 15);
  rect.width = 10;
  rect.height = 10;
  rect.angle = 0.0;
  rectangles.push_back(rect);

  ImageProcessor::DrawRectangles(testImage, rectangles);

  // Check that some pixels were modified to draw the rectangle outline
  // Since we started with black background (0), drawing should change pixels to
  // white (255)
  bool hasDrawnPixels = false;
  for (int y = 0; y < 50; ++y) {
    for (int x = 0; x < 50; ++x) {
      if (testImage.pixels[y][x] == 255) {
        hasDrawnPixels = true;
        break;
      }
    }
    if (hasDrawnPixels)
      break;
  }

  EXPECT_TRUE(hasDrawnPixels);
}