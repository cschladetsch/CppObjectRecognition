#include "ShapeDetector/SphereDetector.hpp"
#include "ShapeDetector/ImageProcessor.hpp"
#include "ShapeDetector/RectangleDetector.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <iostream>
#ifdef _OPENMP
#include <omp.h>
#endif

class ObloidIntegrationTest : public ::testing::Test {
protected:
  void SetUp() override {
    sphereDetector = new SphereDetector();
    sphereDetector->SetMinRadius(5);
    sphereDetector->SetMaxRadius(80);
    sphereDetector->SetCircularityThreshold(0.75);
    sphereDetector->SetConfidenceThreshold(0.6);
    
    rectangleDetector = new RectangleDetector();
    rectangleDetector->SetMinArea(200.0);
    rectangleDetector->SetMaxArea(8000.0);
    rectangleDetector->SetApproxEpsilon(0.05);
  }

  void TearDown() override {
    delete sphereDetector;
    delete rectangleDetector;
  }

  SphereDetector *sphereDetector;
  RectangleDetector *rectangleDetector;

  Image CreateMixedShapeImage(int width, int height) {
    Image image(width, height);
    
    // Fill with black background
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        image.pixels[y][x] = 0;
      }
    }
    
    // Add various obloid shapes with safer parameters
    ImageProcessor::DrawFilledCircle(image, 60, 60, 25, 255);                    // Perfect circle
    if (width >= 210 && height >= 92) {
      ImageProcessor::DrawFilledEllipse(image, 180, 70, 30, 22, 0.0, 255);      // Ellipse
    }
    if (width >= 148 && height >= 178) {
      ImageProcessor::DrawFilledEllipse(image, 120, 150, 20, 28, M_PI/3, 255);  // Rotated ellipse
    }
    
    // Add some rectangles too
    for (int y = 200; y < 240; ++y) {
      for (int x = 50; x < 100; ++x) {
        image.pixels[y][x] = 255;
      }
    }
    
    for (int y = 180; y < 220; ++y) {
      for (int x = 200; x < 250; ++x) {
        image.pixels[y][x] = 255;
      }
    }
    
    return image;
  }
};

TEST_F(ObloidIntegrationTest, DetectsObloidAndRectangleShapes) {
  Image testImage = CreateMixedShapeImage(300, 250);
  
  std::vector<Sphere> obloids = sphereDetector->DetectSpheres(testImage);
  std::vector<Rectangle> rectangles = rectangleDetector->DetectRectangles(testImage);
  
  // Should detect some obloids
  EXPECT_GE(obloids.size(), 1);
  EXPECT_LE(obloids.size(), 4);
  
  // Should detect some rectangles
  EXPECT_GE(rectangles.size(), 1);
  EXPECT_LE(rectangles.size(), 3);
  
  // Verify obloid properties
  for (const auto& obloid : obloids) {
    EXPECT_GE(obloid.radius, 15);
    EXPECT_LE(obloid.radius, 80);
    EXPECT_GT(obloid.confidence, 0.6);
  }
  
  // Verify rectangle properties
  for (const auto& rect : rectangles) {
    EXPECT_GE(rect.width * rect.height, 200.0);
    EXPECT_LE(rect.width * rect.height, 8000.0);
  }
}

TEST_F(ObloidIntegrationTest, CreatesColorImageWithBothShapes) {
  Image testImage = CreateMixedShapeImage(300, 250);
  
  std::vector<Sphere> obloids = sphereDetector->DetectSpheres(testImage);
  std::vector<Rectangle> rectangles = rectangleDetector->DetectRectangles(testImage);
  
  ColorImage colorImage = ImageProcessor::CreateColorImageWithSpheres(
      testImage, rectangles, obloids);
  
  EXPECT_EQ(colorImage.width, 300);
  EXPECT_EQ(colorImage.height, 250);
  
  // Check for blue pixels (sphere/obloid outlines)
  bool foundBluePixels = false;
  for (int y = 0; y < 250 && !foundBluePixels; ++y) {
    for (int x = 0; x < 300; ++x) {
      const ColorPixel& pixel = colorImage.pixels[y][x];
      if (pixel.b > 200 && pixel.r < 50 && pixel.g < 50) {
        foundBluePixels = true;
        break;
      }
    }
  }
  
  // Check for red pixels (rectangle outlines)
  bool foundRedPixels = false;
  for (int y = 0; y < 250 && !foundRedPixels; ++y) {
    for (int x = 0; x < 300; ++x) {
      const ColorPixel& pixel = colorImage.pixels[y][x];
      if (pixel.r > 200 && pixel.g < 50 && pixel.b < 50) {
        foundRedPixels = true;
        break;
      }
    }
  }
  
  // Should have both types if shapes were detected
  if (obloids.size() > 0) {
    EXPECT_TRUE(foundBluePixels);
  }
  if (rectangles.size() > 0) {
    EXPECT_TRUE(foundRedPixels);
  }
}

TEST_F(ObloidIntegrationTest, HandlesDifferentObloidSizes) {
  Image testImage(200, 200);
  
  // Fill with black background
  for (int y = 0; y < 200; ++y) {
    for (int x = 0; x < 200; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }
  
  // Add obloids of different sizes
  ImageProcessor::DrawFilledCircle(testImage, 40, 40, 18, 255);    // Small
  ImageProcessor::DrawFilledCircle(testImage, 100, 100, 35, 255);  // Medium
  ImageProcessor::DrawFilledCircle(testImage, 160, 160, 25, 255);  // Medium-small
  ImageProcessor::DrawFilledEllipse(testImage, 60, 140, 40, 28, 0.0, 255); // Large ellipse
  
  std::vector<Sphere> obloids = sphereDetector->DetectSpheres(testImage);
  
  // Should detect multiple obloids of different sizes
  EXPECT_GE(obloids.size(), 2);
  EXPECT_LE(obloids.size(), 5);
  
  // Verify size diversity
  if (obloids.size() >= 2) {
    int minRadius = obloids[0].radius;
    int maxRadius = obloids[0].radius;
    
    for (const auto& obloid : obloids) {
      minRadius = std::min(minRadius, obloid.radius);
      maxRadius = std::max(maxRadius, obloid.radius);
    }
    
    // Should have some size variation
    EXPECT_GT(maxRadius - minRadius, 5);
  }
}

TEST_F(ObloidIntegrationTest, WorksWithTestImageGeneration) {
  // Test with the actual mixed shapes test image generator
  Image testImage = ImageProcessor::CreateTestImageWithMixedShapes(400, 300);
  
  std::vector<Sphere> obloids = sphereDetector->DetectSpheres(testImage);
  std::vector<Rectangle> rectangles = rectangleDetector->DetectRectangles(testImage);
  
  // Should detect some shapes from the generated test image
  EXPECT_GE(obloids.size() + rectangles.size(), 1);
  
  // Verify all detected obloids are valid
  for (const auto& obloid : obloids) {
    EXPECT_GE(obloid.center.x, 0);
    EXPECT_LT(obloid.center.x, 400);
    EXPECT_GE(obloid.center.y, 0);
    EXPECT_LT(obloid.center.y, 300);
    EXPECT_GE(obloid.radius, 15);
    EXPECT_LE(obloid.radius, 80);
    EXPECT_GT(obloid.confidence, 0.6);
  }
}

TEST_F(ObloidIntegrationTest, HandlesEmptyDetectionGracefully) {
  Image emptyImage(100, 100);
  
  // Fill with black background only
  for (int y = 0; y < 100; ++y) {
    for (int x = 0; x < 100; ++x) {
      emptyImage.pixels[y][x] = 0;
    }
  }
  
  std::vector<Sphere> obloids = sphereDetector->DetectSpheres(emptyImage);
  std::vector<Rectangle> rectangles = rectangleDetector->DetectRectangles(emptyImage);
  
  // Should handle empty detection without crashing
  EXPECT_EQ(obloids.size(), 0);
  EXPECT_EQ(rectangles.size(), 0);
  
  // Should create color image even with no shapes
  ColorImage colorImage = ImageProcessor::CreateColorImageWithSpheres(
      emptyImage, rectangles, obloids);
  
  EXPECT_EQ(colorImage.width, 100);
  EXPECT_EQ(colorImage.height, 100);
  
  // Should be all grayscale (no colored outlines)
  for (int y = 0; y < 100; ++y) {
    for (int x = 0; x < 100; ++x) {
      const ColorPixel& pixel = colorImage.pixels[y][x];
      EXPECT_EQ(pixel.r, pixel.g);
      EXPECT_EQ(pixel.g, pixel.b);
      EXPECT_EQ(pixel.r, 0); // Should be black
    }
  }
}

TEST_F(ObloidIntegrationTest, PerformanceWithManyObloids) {
  Image testImage(400, 400);
  
  // Fill with black background
  for (int y = 0; y < 400; ++y) {
    for (int x = 0; x < 400; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }
  
  // Add many small obloids
  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 10; ++j) {
      int centerX = 20 + i * 35;
      int centerY = 20 + j * 35;
      int radius = 8 + (i + j) % 8;
      
      if (centerX + radius < 400 && centerY + radius < 400) {
        ImageProcessor::DrawFilledCircle(testImage, centerX, centerY, radius, 255);
      }
    }
  }
  
  auto start = std::chrono::high_resolution_clock::now();
  std::vector<Sphere> obloids = sphereDetector->DetectSpheres(testImage);
  auto end = std::chrono::high_resolution_clock::now();
  
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  
  // Should complete within reasonable time (adjust threshold as needed)
  EXPECT_LT(duration.count(), 5000); // Less than 5 seconds
  
  // Should detect a reasonable number of the obloids
  EXPECT_GE(obloids.size(), 20); // At least some of them
  EXPECT_LE(obloids.size(), 120); // Not too many false positives
}

TEST_F(ObloidIntegrationTest, ConsistentDetectionResults) {
  // Create a simple image with a single circle
  Image testImage(100, 100);
  
  // Fill with black
  for (int y = 0; y < 100; ++y) {
    for (int x = 0; x < 100; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }
  
  // Add a single circle
  ImageProcessor::DrawFilledCircle(testImage, 50, 50, 20, 255);
  
  // Run detection three times
  std::vector<Sphere> obloids1 = sphereDetector->DetectSpheres(testImage);
  std::vector<Sphere> obloids2 = sphereDetector->DetectSpheres(testImage);
  std::vector<Sphere> obloids3 = sphereDetector->DetectSpheres(testImage);
  
  // All detections should find the same number of spheres
  EXPECT_EQ(obloids1.size(), obloids2.size());
  EXPECT_EQ(obloids2.size(), obloids3.size());
  
  // If spheres are detected, they should be at the same location
  if (obloids1.size() > 0 && obloids1.size() == obloids2.size() && obloids1.size() == obloids3.size()) {
    // For a single circle, all detections should be identical
    EXPECT_NEAR(obloids1[0].center.x, obloids2[0].center.x, 1);
    EXPECT_NEAR(obloids1[0].center.y, obloids2[0].center.y, 1);
    EXPECT_NEAR(obloids1[0].radius, obloids2[0].radius, 1);
    
    EXPECT_NEAR(obloids2[0].center.x, obloids3[0].center.x, 1);
    EXPECT_NEAR(obloids2[0].center.y, obloids3[0].center.y, 1);
    EXPECT_NEAR(obloids2[0].radius, obloids3[0].radius, 1);
  }
}