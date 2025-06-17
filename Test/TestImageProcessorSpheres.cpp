#include "ShapeDetector/RectangleDetector.hpp"  // Includes Sphere definition
#include "ShapeDetector/ImageProcessor.hpp"
#include "ShapeDetector/SphereDetector.hpp"
#include <gtest/gtest.h>
#include <cmath>

class ImageProcessorSpheresTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create test spheres
    testSpheres.push_back(Sphere{{50, 50}, 20, 0.8});
    testSpheres.push_back(Sphere{{150, 100}, 30, 0.9});
    testSpheres.push_back(Sphere{{100, 150}, 25, 0.85});
  }

  std::vector<Sphere> testSpheres;
  std::vector<Rectangle> testRectangles; // Empty for sphere-only tests

  bool IsBluePixel(const ColorPixel& pixel) {
    // Blue pixel should have high blue value, low red/green
    return pixel.b > 200 && pixel.r < 50 && pixel.g < 50;
  }

  bool IsPixelNearSphere(int x, int y, const Sphere& sphere, int thickness = 4) {
    int dx = x - sphere.center.x;
    int dy = y - sphere.center.y;
    double distance = sqrt(dx * dx + dy * dy);
    double minDist = sphere.radius - thickness;
    double maxDist = sphere.radius + thickness;
    return distance >= minDist && distance <= maxDist;
  }
};

TEST_F(ImageProcessorSpheresTest, DrawSpheresCreatesBlueOutlines) {
  ColorImage testImage(200, 200);
  
  // Initialize with gray background
  for (int y = 0; y < 200; ++y) {
    for (int x = 0; x < 200; ++x) {
      testImage.pixels[y][x] = ColorPixel(128, 128, 128);
    }
  }
  
  ImageProcessor::DrawSpheres(testImage, testSpheres);
  
  // Check that blue pixels are drawn at sphere boundaries
  bool foundBluePixels = false;
  for (int y = 0; y < 200; ++y) {
    for (int x = 0; x < 200; ++x) {
      if (IsBluePixel(testImage.pixels[y][x])) {
        foundBluePixels = true;
        
        // Verify this pixel is near at least one sphere boundary
        bool nearSphere = false;
        for (const auto& sphere : testSpheres) {
          if (IsPixelNearSphere(x, y, sphere)) {
            nearSphere = true;
            break;
          }
        }
        EXPECT_TRUE(nearSphere) << "Blue pixel at (" << x << "," << y << ") not near any sphere";
      }
    }
  }
  
  EXPECT_TRUE(foundBluePixels) << "No blue pixels found in image";
}

TEST_F(ImageProcessorSpheresTest, CreateColorImageWithSpheresWorks) {
  Image grayImage(250, 200);
  
  // Create grayscale test image
  for (int y = 0; y < 200; ++y) {
    for (int x = 0; x < 250; ++x) {
      grayImage.pixels[y][x] = (x + y) % 256; // Gradient pattern
    }
  }
  
  // Add a test rectangle
  testRectangles.push_back(Rectangle{{200, 50}, 40, 30, 0.5});
  
  ColorImage colorImage = ImageProcessor::CreateColorImageWithSpheres(
      grayImage, testRectangles, testSpheres);
  
  EXPECT_EQ(colorImage.width, 250);
  EXPECT_EQ(colorImage.height, 200);
  
  // Verify background is preserved as grayscale
  bool foundGrayscaleBackground = false;
  for (int y = 10; y < 40; ++y) {  // Check area unlikely to have drawn shapes
    for (int x = 10; x < 40; ++x) {
      const ColorPixel& pixel = colorImage.pixels[y][x];
      if (pixel.r == pixel.g && pixel.g == pixel.b) {
        foundGrayscaleBackground = true;
        EXPECT_EQ(pixel.r, grayImage.pixels[y][x]);
        break;
      }
    }
    if (foundGrayscaleBackground) break;
  }
  EXPECT_TRUE(foundGrayscaleBackground);
  
  // Verify blue sphere outlines exist
  bool foundBlueOutlines = false;
  for (int y = 0; y < 200; ++y) {
    for (int x = 0; x < 250; ++x) {
      if (IsBluePixel(colorImage.pixels[y][x])) {
        foundBlueOutlines = true;
        break;
      }
    }
    if (foundBlueOutlines) break;
  }
  EXPECT_TRUE(foundBlueOutlines);
  
  // Verify red rectangle outlines exist
  bool foundRedOutlines = false;
  for (int y = 0; y < 200; ++y) {
    for (int x = 0; x < 250; ++x) {
      const ColorPixel& pixel = colorImage.pixels[y][x];
      if (pixel.r > 200 && pixel.g < 50 && pixel.b < 50) {
        foundRedOutlines = true;
        break;
      }
    }
    if (foundRedOutlines) break;
  }
  EXPECT_TRUE(foundRedOutlines);
}

TEST_F(ImageProcessorSpheresTest, DrawSpheresCreatesProperThickness) {
  ColorImage testImage(100, 100);
  
  // Initialize with black background
  for (int y = 0; y < 100; ++y) {
    for (int x = 0; x < 100; ++x) {
      testImage.pixels[y][x] = ColorPixel(0, 0, 0);
    }
  }
  
  std::vector<Sphere> singleSphere;
  singleSphere.push_back(Sphere{{50, 50}, 25, 0.8});
  
  ImageProcessor::DrawSpheres(testImage, singleSphere);
  
  // Count blue pixels at different distances from center
  int centerX = 50, centerY = 50, radius = 25, thickness = 4;
  std::vector<int> pixelCountAtDistance(radius + thickness + 5, 0);
  
  for (int y = 0; y < 100; ++y) {
    for (int x = 0; x < 100; ++x) {
      if (IsBluePixel(testImage.pixels[y][x])) {
        int dx = x - centerX;
        int dy = y - centerY;
        int distance = static_cast<int>(round(sqrt(dx * dx + dy * dy)));
        if (distance < pixelCountAtDistance.size()) {
          pixelCountAtDistance[distance]++;
        }
      }
    }
  }
  
  // Verify pixels exist in the expected thickness range
  int minExpectedRadius = radius - thickness/2;
  int maxExpectedRadius = radius + thickness/2;
  
  bool foundPixelsInRange = false;
  for (int r = minExpectedRadius; r <= maxExpectedRadius; ++r) {
    if (r >= 0 && r < pixelCountAtDistance.size() && pixelCountAtDistance[r] > 0) {
      foundPixelsInRange = true;
    }
  }
  EXPECT_TRUE(foundPixelsInRange);
  
  // Verify most pixels are near the expected radius
  int pixelsNearRadius = 0;
  for (int r = radius - 2; r <= radius + 2; ++r) {
    if (r >= 0 && r < pixelCountAtDistance.size()) {
      pixelsNearRadius += pixelCountAtDistance[r];
    }
  }
  EXPECT_GT(pixelsNearRadius, 0);
}

TEST_F(ImageProcessorSpheresTest, HandlesEmptySpheresVector) {
  ColorImage testImage(100, 100);
  
  // Initialize with white background
  for (int y = 0; y < 100; ++y) {
    for (int x = 0; x < 100; ++x) {
      testImage.pixels[y][x] = ColorPixel(255, 255, 255);
    }
  }
  
  std::vector<Sphere> emptySpheres;
  ImageProcessor::DrawSpheres(testImage, emptySpheres);
  
  // Verify image remains unchanged (all white)
  for (int y = 0; y < 100; ++y) {
    for (int x = 0; x < 100; ++x) {
      const ColorPixel& pixel = testImage.pixels[y][x];
      EXPECT_EQ(pixel.r, 255);
      EXPECT_EQ(pixel.g, 255);
      EXPECT_EQ(pixel.b, 255);
    }
  }
}

TEST_F(ImageProcessorSpheresTest, HandlesSphereAtImageBoundary) {
  ColorImage testImage(100, 100);
  
  // Initialize with black background
  for (int y = 0; y < 100; ++y) {
    for (int x = 0; x < 100; ++x) {
      testImage.pixels[y][x] = ColorPixel(0, 0, 0);
    }
  }
  
  // Create sphere that extends beyond image boundary
  std::vector<Sphere> boundarySpheres;
  boundarySpheres.push_back(Sphere{{5, 5}, 10, 0.8});    // Near top-left corner
  boundarySpheres.push_back(Sphere{{95, 95}, 10, 0.8});  // Near bottom-right corner
  
  // Should not crash or cause out-of-bounds access
  EXPECT_NO_THROW(ImageProcessor::DrawSpheres(testImage, boundarySpheres));
  
  // Verify some blue pixels were drawn (within image bounds)
  bool foundBluePixels = false;
  for (int y = 0; y < 100; ++y) {
    for (int x = 0; x < 100; ++x) {
      if (IsBluePixel(testImage.pixels[y][x])) {
        foundBluePixels = true;
        break;
      }
    }
    if (foundBluePixels) break;
  }
  EXPECT_TRUE(foundBluePixels);
}

TEST_F(ImageProcessorSpheresTest, BlueColorIsCorrect) {
  ColorImage testImage(60, 60);
  
  // Initialize with black background
  for (int y = 0; y < 60; ++y) {
    for (int x = 0; x < 60; ++x) {
      testImage.pixels[y][x] = ColorPixel(0, 0, 0);
    }
  }
  
  std::vector<Sphere> singleSphere;
  singleSphere.push_back(Sphere{{30, 30}, 15, 0.8});
  
  ImageProcessor::DrawSpheres(testImage, singleSphere);
  
  // Find blue pixels and verify exact color values
  bool foundCorrectBlue = false;
  for (int y = 0; y < 60; ++y) {
    for (int x = 0; x < 60; ++x) {
      const ColorPixel& pixel = testImage.pixels[y][x];
      if (pixel.b > 0) {  // Found a non-black pixel
        EXPECT_EQ(pixel.r, 0) << "Red component should be 0 for blue outline";
        EXPECT_EQ(pixel.g, 0) << "Green component should be 0 for blue outline";
        EXPECT_EQ(pixel.b, 255) << "Blue component should be 255 for blue outline";
        foundCorrectBlue = true;
      }
    }
  }
  EXPECT_TRUE(foundCorrectBlue);
}

TEST_F(ImageProcessorSpheresTest, MultipleSpheresDoNotInterfere) {
  ColorImage testImage(200, 100);
  
  // Initialize with black background
  for (int y = 0; y < 100; ++y) {
    for (int x = 0; x < 200; ++x) {
      testImage.pixels[y][x] = ColorPixel(0, 0, 0);
    }
  }
  
  // Create well-separated spheres
  std::vector<Sphere> separatedSpheres;
  separatedSpheres.push_back(Sphere{{50, 50}, 20, 0.8});
  separatedSpheres.push_back(Sphere{{150, 50}, 20, 0.8});
  
  ImageProcessor::DrawSpheres(testImage, separatedSpheres);
  
  // Count blue pixels near each sphere
  int pixelsNearFirstSphere = 0;
  int pixelsNearSecondSphere = 0;
  
  for (int y = 0; y < 100; ++y) {
    for (int x = 0; x < 200; ++x) {
      if (IsBluePixel(testImage.pixels[y][x])) {
        if (IsPixelNearSphere(x, y, separatedSpheres[0])) {
          pixelsNearFirstSphere++;
        }
        if (IsPixelNearSphere(x, y, separatedSpheres[1])) {
          pixelsNearSecondSphere++;
        }
      }
    }
  }
  
  EXPECT_GT(pixelsNearFirstSphere, 0);
  EXPECT_GT(pixelsNearSecondSphere, 0);
}