#include "ShapeDetector/ImageProcessor.hpp"
#include "ShapeDetector/RectangleDetector.hpp"
#include <cmath>
#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>
#include <numbers>

class ComprehensiveRotationTest : public ::testing::Test {
protected:
  void SetUp() override {
    detector.SetMinArea(200.0);
    detector.SetMaxArea(15000.0);
    detector.SetApproxEpsilon(
        0.015); // Tighter approximation for better corner detection
  }

  RectangleDetector detector;

  // Test rectangle at specific angle
  bool TestAngle(double angleDegrees) {
    double angleRadians = angleDegrees * std::numbers::pi / 180.0;

    Image image(300, 300);
    // Fill with black
    for (int y = 0; y < 300; ++y) {
      for (int x = 0; x < 300; ++x) {
        image.pixels[y][x] = 0;
      }
    }

    // Create rotated rectangle
    ImageProcessor::CreateRotatedRectangle(image, 150, 150, 80, 50,
                                           angleRadians);

    std::vector<Rectangle> rectangles = detector.DetectRectangles(image);
    return rectangles.size() >= 1;
  }
};

TEST_F(ComprehensiveRotationTest, TestEvery5Degrees) {
  std::vector<double> testAngles;
  std::vector<bool> results;

  std::cout << "\n=== COMPREHENSIVE ROTATION TEST ===\n";
  std::cout << "Testing rectangle detection every 5 degrees from 0° to 180°\n";
  std::cout << "Angle\tDetected\tStatus\n";
  std::cout << "-----\t--------\t------\n";

  int totalTests = 0;
  int successfulDetections = 0;

  for (int angle = 0; angle <= 180; angle += 5) {
    bool detected = TestAngle(angle);
    testAngles.push_back(angle);
    results.push_back(detected);

    totalTests++;
    if (detected)
      successfulDetections++;

    std::cout << std::setw(3) << angle << "°\t" << (detected ? "YES" : "NO ")
              << "\t\t" << (detected ? "PASS" : "FAIL") << std::endl;
  }

  double successRate = (double)successfulDetections / totalTests * 100.0;

  std::cout << "\n=== SUMMARY ===\n";
  std::cout << "Total angles tested: " << totalTests << std::endl;
  std::cout << "Successful detections: " << successfulDetections << std::endl;
  std::cout << "Success rate: " << std::fixed << std::setprecision(1)
            << successRate << "%\n";

  // Analyze failure patterns
  std::cout << "\nFailed angles: ";
  bool firstFailure = true;
  for (size_t i = 0; i < testAngles.size(); ++i) {
    if (!results[i]) {
      if (!firstFailure)
        std::cout << ", ";
      std::cout << (int)testAngles[i] << "°";
      firstFailure = false;
    }
  }
  if (firstFailure) {
    std::cout << "None! Perfect detection!";
  }
  std::cout << std::endl;

  // We should achieve at least 70% success rate across all angles
  EXPECT_GE(successRate, 70.0)
      << "Should detect rectangles at most rotation angles";

  // Critical angles that must work (axis-aligned)
  EXPECT_TRUE(results[0]) << "Must detect rectangle at 0°";   // 0°
  EXPECT_TRUE(results[18]) << "Must detect rectangle at 90°"; // 90°

  std::cout << "\n";
}

TEST_F(ComprehensiveRotationTest, TestProblematicAngles) {
  std::cout << "\n=== TESTING HISTORICALLY PROBLEMATIC ANGLES ===\n";

  // These are angles that typically cause issues due to pixel discretization
  std::vector<double> problematicAngles = {
      22.5, 67.5, 112.5, 157.5, // 45° ± offsets
      30,   60,   120,   150,   // 30° increments
      37.5, 52.5, 127.5, 142.5  // Other steep angles
  };

  int successCount = 0;
  for (double angle : problematicAngles) {
    bool detected = TestAngle(angle);
    std::cout << "Angle " << std::setw(5) << angle
              << "°: " << (detected ? "DETECTED" : "MISSED") << std::endl;
    if (detected)
      successCount++;
  }

  double problematicSuccessRate =
      (double)successCount / problematicAngles.size() * 100.0;
  std::cout << "\nProblematic angles success rate: " << std::fixed
            << std::setprecision(1) << problematicSuccessRate << "%\n";

  // Should handle at least 50% of problematic angles
  EXPECT_GE(problematicSuccessRate, 50.0)
      << "Should handle most problematic angles";
}

TEST_F(ComprehensiveRotationTest, TestSteepAngleRange) {
  std::cout << "\n=== TESTING STEEP ANGLES (60° - 120°) ===\n";
  std::cout << "These angles are most affected by pixel discretization\n";

  int successCount = 0;
  int totalCount = 0;

  for (int angle = 60; angle <= 120; angle += 5) {
    bool detected = TestAngle(angle);
    totalCount++;
    if (detected)
      successCount++;

    std::cout << "Angle " << std::setw(3) << angle
              << "°: " << (detected ? "PASS" : "FAIL") << std::endl;
  }

  double steepAngleRate = (double)successCount / totalCount * 100.0;
  std::cout << "\nSteep angle range (60°-120°) success rate: " << std::fixed
            << std::setprecision(1) << steepAngleRate << "%\n";

  // This is the critical test - can we handle steep angles?
  EXPECT_GE(steepAngleRate, 40.0)
      << "Should handle steep angles better than traditional methods";
}

TEST_F(ComprehensiveRotationTest, TestMultipleRectanglesAtDifferentAngles) {
  std::cout << "\n=== TESTING MULTIPLE RECTANGLES AT DIFFERENT ANGLES ===\n";

  Image image(800, 600);
  // Fill with black
  for (int y = 0; y < 600; ++y) {
    for (int x = 0; x < 800; ++x) {
      image.pixels[y][x] = 0;
    }
  }

  // Create rectangles at various challenging angles
  std::vector<double> testAngles = {0, 22.5, 45, 67.5, 90, 112.5, 135, 157.5};

  for (size_t i = 0; i < testAngles.size(); ++i) {
    double angleRadians = testAngles[i] * std::numbers::pi / 180.0;
    int centerX = 100 + (i % 4) * 150;
    int centerY = 150 + (i / 4) * 200;

    ImageProcessor::CreateRotatedRectangle(image, centerX, centerY, 80, 50,
                                           angleRadians);
  }

  std::vector<Rectangle> rectangles = detector.DetectRectangles(image);

  std::cout << "Created rectangles at angles: ";
  for (size_t i = 0; i < testAngles.size(); ++i) {
    if (i > 0)
      std::cout << ", ";
    std::cout << testAngles[i] << "°";
  }
  std::cout << std::endl;

  std::cout << "Detected " << rectangles.size() << " rectangles:\n";
  for (size_t i = 0; i < rectangles.size(); ++i) {
    double angleDeg = rectangles[i].angle * 180.0 / std::numbers::pi;
    // Normalize angle to [0, 180) range
    while (angleDeg < 0)
      angleDeg += 180;
    while (angleDeg >= 180)
      angleDeg -= 180;

    std::cout << "  Rectangle " << (i + 1) << ": center=("
              << rectangles[i].center.x << "," << rectangles[i].center.y
              << "), angle=" << std::fixed << std::setprecision(1) << angleDeg
              << "°\n";
  }

  double multiDetectionRate =
      (double)rectangles.size() / testAngles.size() * 100.0;
  std::cout << "Multi-rectangle detection rate: " << std::fixed
            << std::setprecision(1) << multiDetectionRate << "%\n";

  // Should detect at least half of the rectangles in a complex scene
  EXPECT_GE(rectangles.size(), testAngles.size() / 2)
      << "Should detect multiple rectangles at different angles";
}

TEST_F(ComprehensiveRotationTest, CompareWithBaseline) {
  std::cout << "\n=== BASELINE COMPARISON ===\n";
  std::cout << "Comparing current performance with expected baseline\n";

  // Test a representative sample of angles
  std::vector<double> sampleAngles = {0,  15,  30,  45,  60,  75,
                                      90, 105, 120, 135, 150, 165};
  int detectedCount = 0;

  for (double angle : sampleAngles) {
    if (TestAngle(angle)) {
      detectedCount++;
    }
  }

  double currentPerformance =
      (double)detectedCount / sampleAngles.size() * 100.0;

  std::cout << "Current system performance: " << std::fixed
            << std::setprecision(1) << currentPerformance << "%\n";
  std::cout << "Expected baseline (traditional methods): ~30-40%\n";
  std::cout << "Target performance (moment-based): >60%\n";

  if (currentPerformance > 60.0) {
    std::cout << "EXCELLENT: Significantly above target!\n";
  } else if (currentPerformance > 40.0) {
    std::cout << "GOOD: Above traditional baseline\n";
  } else {
    std::cout << "NEEDS IMPROVEMENT: Below expectations\n";
  }

  // Should significantly outperform traditional methods
  EXPECT_GT(currentPerformance, 40.0)
      << "Should outperform traditional contour-based methods";
}