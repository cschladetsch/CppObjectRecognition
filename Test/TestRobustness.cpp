#include "ShapeDetector/ImageProcessor.hpp"
#include "ShapeDetector/RectangleDetector.hpp"
#include <cmath>
#include <gtest/gtest.h>
#include <numbers>

class RobustnessTest : public ::testing::Test {
protected:
  void SetUp() override {
    detector = std::make_unique<RectangleDetector>();
    detector->SetMinArea(200.0);
    detector->SetMaxArea(20000.0);
    detector->SetApproxEpsilon(0.02);
  }

  std::unique_ptr<RectangleDetector> detector;
};

TEST_F(RobustnessTest, HandlesEmptyImage) {
  Image testImage(100, 100);

  // Fill with black (empty)
  for (int y = 0; y < 100; ++y) {
    for (int x = 0; x < 100; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  EXPECT_EQ(rectangles.size(), 0)
      << "Empty image should not detect any rectangles";
}

TEST_F(RobustnessTest, HandlesAllWhiteImage) {
  Image testImage(100, 100);

  // Fill with white
  for (int y = 0; y < 100; ++y) {
    for (int x = 0; x < 100; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  // May detect the entire image as one large rectangle or nothing
  EXPECT_LE(rectangles.size(), 1)
      << "All white image should detect at most one rectangle";
}

TEST_F(RobustnessTest, HandlesSinglePixelShapes) {
  Image testImage(100, 100);

  // Fill with black
  for (int y = 0; y < 100; ++y) {
    for (int x = 0; x < 100; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  // Add single white pixels
  testImage.pixels[50][50] = 255;
  testImage.pixels[60][60] = 255;
  testImage.pixels[70][70] = 255;

  detector->SetMinArea(1.0); // Very low threshold
  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  // Single pixels should not be detected as rectangles
  EXPECT_EQ(rectangles.size(), 0)
      << "Single pixels should not be detected as rectangles";
}

TEST_F(RobustnessTest, HandlesCheckerboardPattern) {
  Image testImage(200, 200);

  // Create checkerboard pattern
  for (int y = 0; y < 200; ++y) {
    for (int x = 0; x < 200; ++x) {
      int squareSize = 20;
      int checkX = x / squareSize;
      int checkY = y / squareSize;
      testImage.pixels[y][x] = ((checkX + checkY) % 2) ? 255 : 0;
    }
  }

  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  // Checkerboard should detect multiple rectangles (the white squares)
  EXPECT_GE(rectangles.size(), 10)
      << "Checkerboard should detect multiple rectangle squares";
  EXPECT_LE(rectangles.size(), 100)
      << "Should not detect excessive false positives";
}

TEST_F(RobustnessTest, HandlesGradientImages) {
  Image testImage(300, 200);

  // Create horizontal gradient
  for (int y = 0; y < 200; ++y) {
    for (int x = 0; x < 300; ++x) {
      testImage.pixels[y][x] = (x * 255) / 299; // Gradient from 0 to 255
    }
  }

  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  // Gradient should not be detected as rectangles
  EXPECT_LE(rectangles.size(), 2)
      << "Gradient image should not detect many false rectangles";
}

TEST_F(RobustnessTest, HandlesImageWithHoles) {
  Image testImage(300, 300);

  // Fill with black
  for (int y = 0; y < 300; ++y) {
    for (int x = 0; x < 300; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  // Create rectangle with hole in middle
  for (int y = 50; y < 150; ++y) {
    for (int x = 50; x < 200; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  // Create hole
  for (int y = 80; y < 120; ++y) {
    for (int x = 100; x < 150; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  // Should handle the complex shape gracefully
  EXPECT_GE(rectangles.size(), 0);
  EXPECT_LE(rectangles.size(), 5)
      << "Rectangle with hole should not cause excessive detections";
}

TEST_F(RobustnessTest, HandlesRotatedRectangleAtExtremeAngles) {
  Image testImage(400, 400);

  // Fill with black
  for (int y = 0; y < 400; ++y) {
    for (int x = 0; x < 400; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  // Test angles that are known to be problematic
  std::vector<double> extremeAngles = {1.0, 179.0, 91.0, 89.0, 45.1, 134.9};

  for (double angle : extremeAngles) {
    // Clear image
    for (int y = 0; y < 400; ++y) {
      for (int x = 0; x < 400; ++x) {
        testImage.pixels[y][x] = 0;
      }
    }

    // Create rotated rectangle
    double angleRad = angle * std::numbers::pi / 180.0;
    ImageProcessor::CreateRotatedRectangle(testImage, 200, 200, 100, 60,
                                           angleRad);

    std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

    EXPECT_GE(rectangles.size(), 1)
        << "Should detect rectangle at extreme angle " << angle << "°";
  }
}

TEST_F(RobustnessTest, HandlesVeryThinRectangles) {
  Image testImage(400, 200);

  // Fill with black
  for (int y = 0; y < 200; ++y) {
    for (int x = 0; x < 400; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  // Very thin horizontal rectangle (200x3)
  for (int y = 50; y < 53; ++y) {
    for (int x = 100; x < 300; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  // Very thin vertical rectangle (3x100)
  for (int y = 80; y < 180; ++y) {
    for (int x = 350; x < 353; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  detector->SetMinArea(100.0); // Lower threshold for thin shapes
  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  // May or may not detect very thin rectangles depending on algorithm
  // robustness
  EXPECT_GE(rectangles.size(), 0);
  EXPECT_LE(rectangles.size(), 2)
      << "Should handle thin rectangles without false positives";
}

TEST_F(RobustnessTest, HandlesAlmostRectangularShapes) {
  Image testImage(300, 300);

  // Fill with black
  for (int y = 0; y < 300; ++y) {
    for (int x = 0; x < 300; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  // Rectangle with slightly rounded corners
  for (int y = 51; y < 149; ++y) {
    for (int x = 51; x < 199; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  // Remove corner pixels to simulate rounding
  testImage.pixels[51][51] = 0;
  testImage.pixels[51][198] = 0;
  testImage.pixels[148][51] = 0;
  testImage.pixels[148][198] = 0;

  // Trapezoid (almost rectangle)
  for (int y = 200; y < 250; ++y) {
    int width = 80 - (y - 200) / 5; // Gradually narrowing
    int startX = 150 - width / 2;
    int endX = 150 + width / 2;
    for (int x = startX; x < endX; ++x) {
      if (x >= 0 && x < 300) {
        testImage.pixels[y][x] = 255;
      }
    }
  }

  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  // Should be tolerant of almost-rectangular shapes
  EXPECT_GE(rectangles.size(), 1) << "Should detect almost-rectangular shapes";
  EXPECT_LE(rectangles.size(), 3)
      << "Should not detect too many false positives";
}

TEST_F(RobustnessTest, HandlesMultipleScales) {
  Image testImage(600, 600);

  // Fill with black
  for (int y = 0; y < 600; ++y) {
    for (int x = 0; x < 600; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  // Large rectangle
  for (int y = 50; y < 200; ++y) {
    for (int x = 50; x < 250; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  // Medium rectangle
  for (int y = 300; y < 400; ++y) {
    for (int x = 300; x < 450; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  // Small rectangle
  for (int y = 500; y < 540; ++y) {
    for (int x = 500; x < 560; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }

  detector->SetMinArea(100.0);
  detector->SetMaxArea(100000.0);
  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  EXPECT_EQ(rectangles.size(), 3)
      << "Should detect rectangles of different scales";

  // Verify different sizes are detected
  std::vector<int> areas;
  for (const auto &rect : rectangles) {
    areas.push_back(rect.width * rect.height);
  }

  std::sort(areas.begin(), areas.end());

  // Should have small, medium, and large rectangles
  EXPECT_LT(areas[0], 5000) << "Should detect small rectangle";
  EXPECT_GT(areas[2], 15000) << "Should detect large rectangle";
}

TEST_F(RobustnessTest, StressTestWithRandomShapes) {
  Image testImage(500, 500);

  srand(12345); // Fixed seed for reproducibility

  // Fill with black
  for (int y = 0; y < 500; ++y) {
    for (int x = 0; x < 500; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  // Create random rectangles
  int expectedRects = 0;
  for (int i = 0; i < 20; ++i) {
    int x = rand() % 400;
    int y = rand() % 400;
    int w = 30 + rand() % 70;
    int h = 30 + rand() % 70;

    if (x + w < 500 && y + h < 500) {
      for (int py = y; py < y + h; ++py) {
        for (int px = x; px < x + w; ++px) {
          testImage.pixels[py][px] = 255;
        }
      }
      expectedRects++;
    }
  }

  // Add some random noise
  for (int i = 0; i < 1000; ++i) {
    int x = rand() % 500;
    int y = rand() % 500;
    testImage.pixels[y][x] = rand() % 256;
  }

  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  // Should detect some rectangles despite noise and overlaps
  // Random overlaps and noise make this test inherently variable
  EXPECT_GE(rectangles.size(), std::max(1, expectedRects / 5))
      << "Should detect some rectangles in noisy environment";
  EXPECT_LE(rectangles.size(), expectedRects * 2)
      << "Should not have excessive false positives";

  std::cout << "Stress test: Created " << expectedRects
            << " rectangles, detected " << rectangles.size() << std::endl;
}

TEST_F(RobustnessTest, HandlesIncompleteRectangles) {
  Image testImage(300, 300);

  // Fill with black
  for (int y = 0; y < 300; ++y) {
    for (int x = 0; x < 300; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }

  // Rectangle missing one side
  for (int y = 50; y < 150; ++y) {
    for (int x = 50; x < 200; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }
  // Remove top side
  for (int x = 50; x < 200; ++x) {
    testImage.pixels[50][x] = 0;
  }

  // Rectangle with gap in one side
  for (int y = 200; y < 250; ++y) {
    for (int x = 200; x < 280; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }
  // Create gap in right side
  for (int y = 220; y < 230; ++y) {
    testImage.pixels[y][279] = 0;
  }

  std::vector<Rectangle> rectangles = detector->DetectRectangles(testImage);

  // Should handle incomplete rectangles gracefully
  EXPECT_GE(rectangles.size(), 0);
  EXPECT_LE(rectangles.size(), 3)
      << "Incomplete rectangles should not cause excessive detections";
}