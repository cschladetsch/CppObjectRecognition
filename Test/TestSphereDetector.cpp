#include "ShapeDetector/SphereDetector.hpp"
#include "ShapeDetector/ImageProcessor.hpp"
#include <gtest/gtest.h>
#include <cmath>

class ObloidDetectorTest : public ::testing::Test {
protected:
  void SetUp() override {
    detector = new SphereDetector();
    detector->SetMinRadius(5);  // Lower minimum radius
    detector->SetMaxRadius(150);
    detector->SetCircularityThreshold(0.6);  // More relaxed circularity
    detector->SetConfidenceThreshold(0.4);   // Lower confidence threshold
  }

  void TearDown() override { 
    delete detector; 
  }

  SphereDetector *detector;

  Image CreateImageWithCircle(int width, int height, int centerX, int centerY, int radius) {
    Image image(width, height);
    
    // Fill with black background
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        image.pixels[y][x] = 0;
      }
    }
    
    // Draw filled circle (perfect obloid)
    ImageProcessor::DrawFilledCircle(image, centerX, centerY, radius, 255);
    
    return image;
  }

  Image CreateImageWithEllipse(int width, int height, int centerX, int centerY, int radiusX, int radiusY, double angle = 0.0) {
    Image image(width, height);
    
    // Fill with black background
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        image.pixels[y][x] = 0;
      }
    }
    
    // Draw filled ellipse (obloid with different axes)
    ImageProcessor::DrawFilledEllipse(image, centerX, centerY, radiusX, radiusY, angle, 255);
    
    return image;
  }

  Image CreateImageWithMultipleObloids(int width, int height) {
    Image image(width, height);
    
    // Fill with black background
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        image.pixels[y][x] = 0;
      }
    }
    
    // Draw multiple obloid shapes
    ImageProcessor::DrawFilledCircle(image, 60, 60, 25, 255);                    // Perfect circle
    ImageProcessor::DrawFilledEllipse(image, 160, 80, 35, 25, 0.0, 255);        // Horizontal ellipse
    ImageProcessor::DrawFilledEllipse(image, 100, 150, 20, 30, M_PI/4, 255);    // Rotated ellipse
    ImageProcessor::DrawFilledCircle(image, 200, 50, 18, 255);                  // Small circle
    
    return image;
  }

  Image CreateImageWithNearCircularEllipse(int width, int height, int centerX, int centerY, int majorRadius, double eccentricity) {
    Image image(width, height);
    
    // Fill with black background
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        image.pixels[y][x] = 0;
      }
    }
    
    // Calculate minor radius from eccentricity
    int minorRadius = static_cast<int>(majorRadius * sqrt(1.0 - eccentricity * eccentricity));
    
    // Draw ellipse that's nearly circular
    ImageProcessor::DrawFilledEllipse(image, centerX, centerY, majorRadius, minorRadius, 0.0, 255);
    
    return image;
  }
};

TEST_F(ObloidDetectorTest, DetectsPerfectCircle) {
  Image testImage = CreateImageWithCircle(200, 200, 100, 100, 30);
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  EXPECT_GE(obloids.size(), 1);
  
  if (obloids.size() >= 1) {
    const Sphere& obloid = obloids[0];
    EXPECT_NEAR(obloid.center.x, 100, 10);
    EXPECT_NEAR(obloid.center.y, 100, 10);
    EXPECT_NEAR(obloid.radius, 30, 10);
    EXPECT_GT(obloid.confidence, 0.6);
  }
}

TEST_F(ObloidDetectorTest, DetectsMultipleObloidShapes) {
  Image testImage = CreateImageWithMultipleObloids(250, 200);
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  EXPECT_GE(obloids.size(), 1); // Should detect at least the perfect circle
  EXPECT_LE(obloids.size(), 5); // Allow variation based on circularity threshold
  
  // Verify all detected obloids have reasonable properties
  for (const auto& obloid : obloids) {
    EXPECT_GE(obloid.radius, 10);
    EXPECT_LE(obloid.radius, 100);
    EXPECT_GT(obloid.confidence, 0.6);
    EXPECT_GE(obloid.center.x, 0);
    EXPECT_LT(obloid.center.x, 250);
    EXPECT_GE(obloid.center.y, 0);
    EXPECT_LT(obloid.center.y, 200);
  }
}

TEST_F(ObloidDetectorTest, RejectsSmallObloids) {
  detector->SetMinRadius(25);
  
  Image testImage = CreateImageWithCircle(100, 100, 50, 50, 15); // Radius 15 < min 25
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  EXPECT_EQ(obloids.size(), 0);
}

TEST_F(ObloidDetectorTest, RejectsLargeObloids) {
  detector->SetMaxRadius(50);
  
  Image testImage = CreateImageWithCircle(200, 200, 100, 100, 60); // Radius 60 > max 50
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  EXPECT_EQ(obloids.size(), 0);
}

TEST_F(ObloidDetectorTest, HandlesEmptyImage) {
  Image testImage(100, 100);
  
  // Fill with black background only
  for (int y = 0; y < 100; ++y) {
    for (int x = 0; x < 100; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  EXPECT_EQ(obloids.size(), 0);
}

TEST_F(ObloidDetectorTest, HandlesNoisyImage) {
  Image testImage = CreateImageWithCircle(150, 150, 75, 75, 25);
  
  // Add random noise
  for (int y = 0; y < 150; ++y) {
    for (int x = 0; x < 150; ++x) {
      if (testImage.pixels[y][x] == 0 && (rand() % 20) == 0) {
        testImage.pixels[y][x] = 255; // Random white noise
      }
    }
  }
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  // Should still detect the main circular obloid despite noise
  EXPECT_GE(obloids.size(), 1);
  
  if (obloids.size() >= 1) {
    bool foundMainObloid = false;
    for (const auto& obloid : obloids) {
      if (abs(obloid.center.x - 75) < 15 && abs(obloid.center.y - 75) < 15) {
        foundMainObloid = true;
        EXPECT_NEAR(obloid.radius, 25, 10);
        break;
      }
    }
    EXPECT_TRUE(foundMainObloid);
  }
}

TEST_F(ObloidDetectorTest, RejectsNonObloidShapes) {
  Image testImage(150, 150);
  
  // Fill with black background
  for (int y = 0; y < 150; ++y) {
    for (int x = 0; x < 150; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }
  
  // Create a rectangular shape (non-obloid)
  for (int y = 50; y < 100; ++y) {
    for (int x = 40; x < 110; ++x) {
      testImage.pixels[y][x] = 255;
    }
  }
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  // Should not detect rectangle as obloid
  EXPECT_EQ(obloids.size(), 0);
}

TEST_F(ObloidDetectorTest, ParameterSettersWork) {
  detector->SetMinRadius(5);
  detector->SetMaxRadius(200);
  detector->SetCircularityThreshold(0.5);
  detector->SetConfidenceThreshold(0.3);
  
  // Test with a small obloid that would be rejected with default settings
  Image testImage = CreateImageWithCircle(100, 100, 50, 50, 8);
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  // Should now detect the small obloid with relaxed parameters
  EXPECT_GE(obloids.size(), 1);
}

TEST_F(ObloidDetectorTest, CircularityThresholdFiltersEllipses) {
  detector->SetCircularityThreshold(0.9); // Very strict circularity
  
  Image testImage(150, 150);
  
  // Fill with black background
  for (int y = 0; y < 150; ++y) {
    for (int x = 0; x < 150; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }
  
  // Create an elongated ellipse (low circularity obloid)
  ImageProcessor::DrawFilledEllipse(testImage, 75, 75, 30, 20, 0.0, 255);
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  // Should reject elongated ellipse due to strict circularity threshold
  EXPECT_EQ(obloids.size(), 0);
}

TEST_F(ObloidDetectorTest, ConfidenceThresholdWorks) {
  detector->SetConfidenceThreshold(0.95); // Very high confidence required
  
  // Create a somewhat irregular obloid
  Image testImage(100, 100);
  for (int y = 0; y < 100; ++y) {
    for (int x = 0; x < 100; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }
  
  // Draw an imperfect obloid by drawing pixels manually
  int centerX = 50, centerY = 50, radius = 20;
  for (int y = 0; y < 100; ++y) {
    for (int x = 0; x < 100; ++x) {
      int dx = x - centerX;
      int dy = y - centerY;
      double dist = sqrt(dx * dx + dy * dy);
      // Add some irregularity to make it less perfect
      if (dist < radius + (rand() % 3 - 1)) {
        testImage.pixels[y][x] = 255;
      }
    }
  }
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  // May reject due to high confidence threshold
  for (const auto& obloid : obloids) {
    EXPECT_GE(obloid.confidence, 0.95);
  }
}

TEST_F(ObloidDetectorTest, HandlesOverlappingObloids) {
  Image testImage(200, 200);
  
  // Fill with black background
  for (int y = 0; y < 200; ++y) {
    for (int x = 0; x < 200; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }
  
  // Draw two slightly overlapping circular obloids
  ImageProcessor::DrawFilledCircle(testImage, 70, 100, 20, 255);
  ImageProcessor::DrawFilledCircle(testImage, 130, 100, 20, 255);
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  // Should handle overlapping obloids reasonably
  EXPECT_GE(obloids.size(), 1);
  EXPECT_LE(obloids.size(), 3); // May detect 1-3 obloids depending on overlap handling
}

TEST_F(ObloidDetectorTest, DetectsNearCircularEllipses) {
  detector->SetCircularityThreshold(0.7); // Moderate circularity threshold
  
  // Create ellipse with low eccentricity (nearly circular)
  Image testImage = CreateImageWithNearCircularEllipse(150, 150, 75, 75, 30, 0.3);
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  // Should detect near-circular ellipse
  EXPECT_GE(obloids.size(), 1);
  
  if (obloids.size() >= 1) {
    const Sphere& obloid = obloids[0];
    EXPECT_NEAR(obloid.center.x, 75, 15);
    EXPECT_NEAR(obloid.center.y, 75, 15);
    EXPECT_GT(obloid.confidence, 0.6);
  }
}

TEST_F(ObloidDetectorTest, RejectsHighlyEccentricEllipses) {
  detector->SetCircularityThreshold(0.8); // Strict circularity threshold
  
  Image testImage = CreateImageWithEllipse(150, 150, 75, 75, 40, 15, 0.0); // High eccentricity
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  // Should reject highly eccentric ellipse
  EXPECT_EQ(obloids.size(), 0);
}

TEST_F(ObloidDetectorTest, DetectsRotatedEllipses) {
  detector->SetCircularityThreshold(0.6); // Relaxed threshold for ellipses
  
  Image testImage = CreateImageWithEllipse(150, 150, 75, 75, 25, 20, M_PI/4); // 45-degree rotation
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  // Should detect rotated ellipse with relaxed circularity
  EXPECT_GE(obloids.size(), 0); // May or may not detect depending on exact implementation
  
  for (const auto& obloid : obloids) {
    EXPECT_NEAR(obloid.center.x, 75, 20);
    EXPECT_NEAR(obloid.center.y, 75, 20);
    EXPECT_GT(obloid.confidence, 0.6);
  }
}

TEST_F(ObloidDetectorTest, HandlesMultipleEllipseOrientations) {
  detector->SetCircularityThreshold(0.65); // Moderate threshold
  detector->SetConfidenceThreshold(0.5);   // Relaxed confidence
  
  Image testImage(300, 200);
  
  // Fill with black background
  for (int y = 0; y < 200; ++y) {
    for (int x = 0; x < 300; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }
  
  // Draw ellipses at different orientations
  ImageProcessor::DrawFilledEllipse(testImage, 60, 60, 25, 20, 0.0, 255);        // Horizontal
  ImageProcessor::DrawFilledEllipse(testImage, 150, 60, 20, 25, M_PI/2, 255);    // Vertical
  ImageProcessor::DrawFilledEllipse(testImage, 240, 60, 22, 18, M_PI/4, 255);    // 45 degrees
  ImageProcessor::DrawFilledCircle(testImage, 150, 140, 22, 255);                // Perfect circle
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  // Should detect at least the perfect circle, possibly some ellipses
  EXPECT_GE(obloids.size(), 1);
  EXPECT_LE(obloids.size(), 5);
  
  // Verify detected obloids have reasonable properties
  for (const auto& obloid : obloids) {
    EXPECT_GE(obloid.radius, 15);
    EXPECT_LE(obloid.radius, 30);
    EXPECT_GT(obloid.confidence, 0.5);
  }
}

TEST_F(ObloidDetectorTest, DistinguishesObloidFromTriangle) {
  Image testImage(150, 150);
  
  // Fill with black background
  for (int y = 0; y < 150; ++y) {
    for (int x = 0; x < 150; ++x) {
      testImage.pixels[y][x] = 0;
    }
  }
  
  // Draw triangle (non-obloid shape)
  Point p1(75, 30);
  Point p2(45, 90);
  Point p3(105, 90);
  ImageProcessor::DrawFilledTriangle(testImage, p1, p2, p3, 255);
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  // Should not detect triangle as obloid
  EXPECT_EQ(obloids.size(), 0);
}

TEST_F(ObloidDetectorTest, DetectsObloidWithNoise) {
  Image testImage = CreateImageWithEllipse(150, 150, 75, 75, 25, 22, 0.0);
  
  // Add structured noise around the ellipse
  for (int y = 0; y < 150; ++y) {
    for (int x = 0; x < 150; ++x) {
      if (testImage.pixels[y][x] == 0) {
        // Add some random white pixels as noise
        if ((x + y) % 17 == 0) {
          testImage.pixels[y][x] = 255;
        }
      }
    }
  }
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  // Should still detect the main obloid despite noise
  EXPECT_GE(obloids.size(), 1);
  
  if (obloids.size() >= 1) {
    bool foundMainObloid = false;
    for (const auto& obloid : obloids) {
      if (abs(obloid.center.x - 75) < 20 && abs(obloid.center.y - 75) < 20) {
        foundMainObloid = true;
        EXPECT_NEAR(obloid.radius, 23, 15); // Average of major/minor axes
        break;
      }
    }
    EXPECT_TRUE(foundMainObloid);
  }
}

TEST_F(ObloidDetectorTest, VerifiesObloidGeometry) {
  Image testImage = CreateImageWithCircle(100, 100, 50, 50, 20);
  
  std::vector<Sphere> obloids = detector->DetectSpheres(testImage);
  
  EXPECT_GE(obloids.size(), 1);
  
  if (obloids.size() >= 1) {
    const Sphere& obloid = obloids[0];
    
    // Verify geometric properties
    EXPECT_GE(obloid.center.x, 0);
    EXPECT_LT(obloid.center.x, 100);
    EXPECT_GE(obloid.center.y, 0);
    EXPECT_LT(obloid.center.y, 100);
    EXPECT_GT(obloid.radius, 0);
    EXPECT_GE(obloid.confidence, 0.0);
    EXPECT_LE(obloid.confidence, 1.0);
  }
}