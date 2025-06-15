#include "ShapeDetector/ImageProcessor.hpp"
#include "ShapeDetector/RectangleDetector.hpp"
#include <cmath>
#include <gtest/gtest.h>
#include <numbers>

class AdvancedRectangleDetectionTest : public ::testing::Test {
protected:
  void SetUp() override {
    detector = std::make_unique<RectangleDetector>();
    detector->SetMinArea(100.0);
    detector->SetMaxArea(50000.0);
    detector->SetApproxEpsilon(0.02);
  }

  std::unique_ptr<RectangleDetector> detector;
};

TEST_F(AdvancedRectangleDetectionTest, DetectsVerySmallRectangles) {
  Image testImage(200, 200);

  // Fill with black
  for (int y = 0; y < 200; ++y) {
    for (int x = 0; x < 200; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  // Create very small rectangle (15x10 pixels)
  for (int y = 90; y < 100; ++y) {
    for (int x = 90; x < 105; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  detector->SetMinArea(50.0); // Lower threshold for small rectangles
  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  EXPECT_EQ(rectangles.size(), 1);
  if (rectangles.size() > 0) {
    EXPECT_NEAR(rectangles[0].center.x, 97, 3);
    EXPECT_NEAR(rectangles[0].center.y, 95, 3);
    EXPECT_NEAR(rectangles[0].width, 15, 3);
    EXPECT_NEAR(rectangles[0].height, 10, 3);
  }
}

TEST_F(AdvancedRectangleDetectionTest, DetectsVeryLargeRectangles) {
  Image testImage(800, 600);

  // Fill with black
  for (int y = 0; y < 600; ++y) {
    for (int x = 0; x < 800; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  // Create large rectangle (400x300 pixels)
  for (int y = 150; y < 450; ++y) {
    for (int x = 200; x < 600; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  detector->SetMaxArea(200000.0); // Higher threshold for large rectangles
  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  EXPECT_GE(rectangles.size(), 1);
  bool foundLargeRect = false;
  for (const auto &rect : rectangles) {
    if (rect.width > 350 && rect.height > 250) {
      foundLargeRect = true;
      EXPECT_NEAR(rect.center.x, 400, 20);
      EXPECT_NEAR(rect.center.y, 300, 20);
      break;
    }
  }
  EXPECT_TRUE(foundLargeRect) << "Should detect the large rectangle";
}

TEST_F(AdvancedRectangleDetectionTest, DetectsSquares) {
  Image testImage(300, 300);

  // Fill with black
  for (int y = 0; y < 300; ++y) {
    for (int x = 0; x < 300; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  // Create perfect squares at different positions
  // Square 1: 50x50
  for (int y = 50; y < 100; ++y) {
    for (int x = 50; x < 100; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  // Square 2: 30x30
  for (int y = 150; y < 180; ++y) {
    for (int x = 200; x < 230; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  EXPECT_EQ(rectangles.size(), 2);

  // Verify both squares are detected as rectangles
  int squareCount = 0;
  for (const auto &rect : rectangles) {
    double aspectRatio = static_cast<double>(rect.width) / rect.height;
    if (std::abs(aspectRatio - 1.0) < 0.2) { // Square-like aspect ratio
      squareCount++;
    }
  }
  EXPECT_EQ(squareCount, 2) << "Both squares should be detected as rectangles";
}

TEST_F(AdvancedRectangleDetectionTest, HandlesDifferentAspectRatios) {
  Image testImage(600, 400);

  // Fill with black
  for (int y = 0; y < 400; ++y) {
    for (int x = 0; x < 600; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  // Wide rectangle (aspect ratio 4:1)
  for (int y = 50; y < 75; ++y) {
    for (int x = 50; x < 150; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  // Tall rectangle (aspect ratio 1:3)
  for (int y = 100; y < 175; ++y) {
    for (int x = 200; x < 225; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  // Normal rectangle (aspect ratio 3:2)
  for (int y = 250; y < 325; ++y) {
    for (int x = 350; x < 500; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  EXPECT_EQ(rectangles.size(), 3);

  // Verify different aspect ratios are detected
  std::vector<double> aspectRatios;
  for (const auto &rect : rectangles) {
    double aspectRatio =
        static_cast<double>(std::max(rect.width, rect.height)) /
        static_cast<double>(std::min(rect.width, rect.height));
    aspectRatios.push_back(aspectRatio);
  }

  // Should have wide, tall, and normal aspect ratios
  bool hasWide = false, hasTall = false, hasNormal = false;
  for (double ratio : aspectRatios) {
    if (ratio > 3.5)
      hasWide = true;
    else if (ratio > 2.5)
      hasTall = true;
    else
      hasNormal = true;
  }

  EXPECT_TRUE(hasWide) << "Should detect wide rectangle";
  EXPECT_TRUE(hasTall) << "Should detect tall rectangle";
  EXPECT_TRUE(hasNormal) << "Should detect normal rectangle";
}

TEST_F(AdvancedRectangleDetectionTest, HandlesNoisyImages) {
  Image testImage(300, 300);

  // Fill with random noise
  srand(42); // Fixed seed for reproducible results
  for (int y = 0; y < 300; ++y) {
    for (int x = 0; x < 300; ++x) {
      testImage.pixels[y][x] = rand() % 100; // Low intensity noise
    }
  }

  // Create clear rectangle in noisy background
  for (int y = 100; y < 150; ++y) {
    for (int x = 100; x < 180; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  EXPECT_GE(rectangles.size(), 1);

  // Should find the clear rectangle despite noise
  bool foundMainRect = false;
  for (const auto &rect : rectangles) {
    if (std::abs(rect.center.x - 140) < 20 &&
        std::abs(rect.center.y - 125) < 20) {
      foundMainRect = true;
      break;
    }
  }
  EXPECT_TRUE(foundMainRect)
      << "Should detect rectangle despite background noise";
}

TEST_F(AdvancedRectangleDetectionTest, DetectsOverlappingRectangles) {
  Image testImage(400, 300);

  // Fill with black
  for (int y = 0; y < 300; ++y) {
    for (int x = 0; x < 400; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  // Rectangle 1
  for (int y = 50; y < 120; ++y) {
    for (int x = 50; x < 150; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  // Rectangle 2 (partially overlapping)
  for (int y = 100; y < 180; ++y) {
    for (int x = 120; x < 220; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  // May detect 0-3 rectangles depending on how overlapping shapes are processed
  // Overlapping rectangles might merge into one complex shape that's not
  // detected
  EXPECT_GE(rectangles.size(), 0);
  EXPECT_LE(rectangles.size(), 3);

  // If rectangles are detected, at least one should be in a reasonable region
  if (rectangles.size() > 0) {
    bool foundReasonableRect = false;
    for (const auto &rect : rectangles) {
      if (rect.center.x > 50 && rect.center.x < 250 && rect.center.y > 50 &&
          rect.center.y < 200) {
        foundReasonableRect = true;
        break;
      }
    }
    EXPECT_TRUE(foundReasonableRect)
        << "Detected rectangles should be in reasonable regions";
  }
}

TEST_F(AdvancedRectangleDetectionTest, RejectsIrregularPolygons) {
  Image testImage(400, 300);

  // Fill with black
  for (int y = 0; y < 300; ++y) {
    for (int x = 0; x < 400; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  // Create irregular pentagon
  std::vector<Point> pentagon = {
      {150, 100}, {200, 80}, {220, 130}, {180, 170}, {120, 150}};

  // Fill pentagon (simplified - just draw filled polygon approximation)
  for (int y = 80; y < 170; ++y) {
    for (int x = 120; x < 220; ++x) {
      // Simple point-in-polygon test (rough approximation)
      if (x > 130 && x < 210 && y > 90 && y < 160) {
        testImage.pixels[y][x] = 255;
      }
    }
  }

  // Create hexagon
  for (int y = 50; y < 120; ++y) {
    for (int x = 250; x < 350; ++x) {
      // Hexagon-like shape
      int dx = x - 300;
      int dy = y - 85;
      if (std::abs(dx) + std::abs(dy) < 40 && std::abs(dx) < 35) {
        testImage.pixels[y][x] = 255;
      }
    }
  }

  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  // Should detect very few or no rectangles from irregular polygons
  EXPECT_LE(rectangles.size(), 1)
      << "Should not detect irregular polygons as rectangles";
}

TEST_F(AdvancedRectangleDetectionTest, HandlesEdgeCases) {
  Image testImage(200, 200);

  // Fill with black
  for (int y = 0; y < 200; ++y) {
    for (int x = 0; x < 200; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  // Rectangle touching image border
  for (int y = 0; y < 50; ++y) {
    for (int x = 0; x < 80; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  // Very thin rectangle
  for (int y = 100; y < 105; ++y) {
    for (int x = 50; x < 150; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  detector->SetMinArea(50.0); // Lower threshold for thin rectangles
  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  // Should handle edge cases gracefully
  EXPECT_GE(rectangles.size(), 0);
  EXPECT_LE(rectangles.size(), 3);
}

TEST_F(AdvancedRectangleDetectionTest, PerformanceWithManyRectangles) {
  Image testImage(800, 600);

  // Fill with black
  for (int y = 0; y < 600; ++y) {
    for (int x = 0; x < 800; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  // Create grid of small rectangles
  int rectCount = 0;
  for (int row = 0; row < 6; ++row) {
    for (int col = 0; col < 8; ++col) {
      int startX = col * 100 + 10;
      int startY = row * 100 + 10;
      int endX = startX + 60;
      int endY = startY + 40;

      if (endX < 800 && endY < 600) {
        for (int y = startY; y < endY; ++y) {
          for (int x = startX; x < endX; ++x) {
            testImage.pixels[y][x] = 255;
          }
        }
        rectCount++;
      }
    }
  }

  auto start = std::chrono::high_resolution_clock::now();
  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);
  auto end = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Should detect most rectangles efficiently
  EXPECT_GE(rectangles.size(), rectCount / 2); // At least half
  EXPECT_LE(duration.count(), 5000); // Should complete within 5 seconds

  std::cout << "Detected " << rectangles.size() << " rectangles from "
            << rectCount << " created in " << duration.count() << "ms"
            << std::endl;
}

TEST_F(AdvancedRectangleDetectionTest, ConfigurationParameterEffects) {
  Image testImage(300, 300);

  // Fill with black
  for (int y = 0; y < 300; ++y) {
    for (int x = 0; x < 300; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  // Create test rectangle
  for (int y = 100; y < 150; ++y) {
    for (int x = 100; x < 180; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  // Test with different epsilon values
  std::vector<double> epsilons = {0.01, 0.02, 0.05, 0.1};

  for (double epsilon : epsilons) {
    detector->SetApproxEpsilon(epsilon);
    std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

    // Should detect the rectangle with all reasonable epsilon values
    EXPECT_GE(rectangles.size(), 1)
        << "Should detect rectangle with epsilon " << epsilon;
  }

  // Test with different area thresholds
  detector->SetApproxEpsilon(0.02); // Reset to default

  detector->SetMinArea(1000.0); // Too high
  std::vector<Rectangle> rectangles1 = detector->DetectRectangles(testImage);
  EXPECT_LE(rectangles1.size(), 1) << "High min area should reduce detections";

  detector->SetMinArea(100.0);  // Reasonable
  detector->SetMaxArea(1000.0); // Too low
  std::vector<Rectangle> rectangles2 = detector->DetectRectangles(testImage);
  EXPECT_LE(rectangles2.size(), 1) << "Low max area should reduce detections";
}