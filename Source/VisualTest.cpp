#include "ShapeDetector/ImageProcessor.hpp"
#include "ShapeDetector/RectangleDetector.hpp"
#include <cmath>
#include <iostream>
#include <numbers>
#include <string>

void CreateAndTestImage(const std::string &testName, Image testImage,
                        const std::string &description) {
  // Create detector with reasonable settings
  RectangleDetector detector;
  detector.SetMinArea(200.0);
  detector.SetMaxArea(15000.0);
  detector.SetApproxEpsilon(0.02);

  // Detect rectangles
  std::vector<Rectangle> rectangles = detector.DetectRectangles(testImage);

  // Create color image and draw rectangles on it
  ColorImage colorImage =
      ImageProcessor::CreateColorImage(testImage, rectangles);

  // Save the result as PNG
  std::string filename = "Output/Images/visual_test_" + testName + ".png";
  ImageProcessor::SavePNGImage(colorImage, filename);

  std::cout << testName << " (" << description << "):" << std::endl;
  std::cout << "  Detected " << rectangles.size() << " rectangles" << std::endl;
  std::cout << "  Saved to: " << filename << std::endl;

  for (size_t i = 0; i < rectangles.size(); ++i) {
    std::cout << "    Rectangle " << i << ": center=(" << rectangles[i].center.x
              << "," << rectangles[i].center.y
              << "), size=" << rectangles[i].width << "x"
              << rectangles[i].height << ", angle=" << rectangles[i].angle
              << std::endl;
  }
  std::cout << std::endl;
}

Image CreateCirclesOnlyImage() {
  Image image(400, 300);

  // Black background
  for (int y = 0; y < 300; ++y) {
    for (int x = 0; x < 400; ++x) {
      image.pixels[y][x] = 0;
    }
  }

  // Add circles of various sizes
  ImageProcessor::DrawFilledCircle(image, 80, 80, 30, 255);
  ImageProcessor::DrawFilledCircle(image, 200, 80, 40, 255);
  ImageProcessor::DrawFilledCircle(image, 320, 80, 35, 255);
  ImageProcessor::DrawFilledCircle(image, 140, 200, 25, 255);
  ImageProcessor::DrawFilledCircle(image, 260, 220, 45, 255);

  return image;
}

Image CreateTrianglesOnlyImage() {
  Image image(400, 300);

  // Black background
  for (int y = 0; y < 300; ++y) {
    for (int x = 0; x < 400; ++x) {
      image.pixels[y][x] = 0;
    }
  }

  // Add triangles
  ImageProcessor::DrawFilledTriangle(image, Point(60, 60), Point(120, 60),
                                     Point(90, 20), 255);
  ImageProcessor::DrawFilledTriangle(image, Point(180, 100), Point(240, 140),
                                     Point(140, 140), 255);
  ImageProcessor::DrawFilledTriangle(image, Point(300, 50), Point(370, 50),
                                     Point(335, 120), 255);
  ImageProcessor::DrawFilledTriangle(image, Point(100, 180), Point(160, 220),
                                     Point(70, 240), 255);
  ImageProcessor::DrawFilledTriangle(image, Point(280, 180), Point(350, 200),
                                     Point(310, 260), 255);

  return image;
}

Image CreateRectanglesOnlyImage() {
  Image image(500, 400);

  // Black background
  for (int y = 0; y < 400; ++y) {
    for (int x = 0; x < 500; ++x) {
      image.pixels[y][x] = 0;
    }
  }

  // Add rectangles of different sizes
  // Rectangle 1: 80x50
  for (int y = 50; y < 100; ++y) {
    for (int x = 50; x < 130; ++x) {
      image.pixels[y][x] = 255;
    }
  }

  // Rectangle 2: 60x40
  for (int y = 150; y < 190; ++y) {
    for (int x = 200; x < 260; ++x) {
      image.pixels[y][x] = 255;
    }
  }

  // Rectangle 3: 100x70 (larger)
  for (int y = 250; y < 320; ++y) {
    for (int x = 300; x < 400; ++x) {
      image.pixels[y][x] = 255;
    }
  }

  // Rectangle 4: 50x50 (square)
  for (int y = 80; y < 130; ++y) {
    for (int x = 350; x < 400; ++x) {
      image.pixels[y][x] = 255;
    }
  }

  return image;
}

Image CreateMixedShapesImage() {
  Image image(600, 450);

  // Black background
  for (int y = 0; y < 450; ++y) {
    for (int x = 0; x < 600; ++x) {
      image.pixels[y][x] = 0;
    }
  }

  // Add rectangles (should be detected)
  for (int y = 50; y < 100; ++y) {
    for (int x = 50; x < 130; ++x) {
      image.pixels[y][x] = 255; // Rectangle 1
    }
  }

  for (int y = 200; y < 280; ++y) {
    for (int x = 400; x < 500; ++x) {
      image.pixels[y][x] = 255; // Rectangle 2
    }
  }

  for (int y = 350; y < 400; ++y) {
    for (int x = 150; x < 220; ++x) {
      image.pixels[y][x] = 255; // Rectangle 3
    }
  }

  // Add rotated rectangles (should also be detected)
  ImageProcessor::CreateRotatedRectangle(image, 450, 120, 60, 40,
                                         std::numbers::pi / 6); // 30 degrees
  ImageProcessor::CreateRotatedRectangle(image, 500, 320, 70, 45,
                                         -std::numbers::pi / 4); // -45 degrees

  // Add circles (should NOT be detected)
  ImageProcessor::DrawFilledCircle(image, 250, 100, 35, 255);
  ImageProcessor::DrawFilledCircle(image, 350, 120, 40,
                                   255); // Moved to avoid overlap
  ImageProcessor::DrawFilledCircle(image, 100, 300, 30, 255);

  // Add triangles (should NOT be detected)
  ImageProcessor::DrawFilledTriangle(image, Point(300, 200), Point(370, 200),
                                     Point(335, 150), 255);
  ImageProcessor::DrawFilledTriangle(image, Point(500, 350), Point(570, 390),
                                     Point(480, 400), 255);

  // Add ellipses (should NOT be detected)
  ImageProcessor::DrawFilledEllipse(image, 180, 250, 45, 25, 0.5, 255);
  ImageProcessor::DrawFilledEllipse(image, 380, 380, 35, 20, 1.2, 255);

  return image;
}

Image CreateComplexSceneImage() {
  Image image(800, 600);

  // Black background
  for (int y = 0; y < 600; ++y) {
    for (int x = 0; x < 800; ++x) {
      image.pixels[y][x] = 0;
    }
  }

  // Add many rectangles of different sizes and positions
  for (int y = 100; y < 150; ++y) {
    for (int x = 100; x < 180; ++x) {
      image.pixels[y][x] = 255; // Rectangle 1
    }
  }

  for (int y = 200; y < 260; ++y) {
    for (int x = 300; x < 380; ++x) {
      image.pixels[y][x] = 255; // Rectangle 2
    }
  }

  for (int y = 350; y < 420; ++y) {
    for (int x = 500; x < 600; ++x) {
      image.pixels[y][x] = 255; // Rectangle 3
    }
  }

  for (int y = 450; y < 550; ++y) {
    for (int x = 150; x < 250; ++x) {
      image.pixels[y][x] = 255; // Rectangle 4
    }
  }

  for (int y = 50; y < 120; ++y) {
    for (int x = 600; x < 700; ++x) {
      image.pixels[y][x] = 255; // Rectangle 5
    }
  }

  // Add various non-rectangles scattered throughout
  ImageProcessor::DrawFilledCircle(image, 250, 150, 25, 255);
  ImageProcessor::DrawFilledCircle(image, 450, 200, 30, 255);
  ImageProcessor::DrawFilledCircle(image, 350, 450, 35, 255);
  ImageProcessor::DrawFilledCircle(image, 650, 300, 40, 255);

  ImageProcessor::DrawFilledTriangle(image, Point(200, 350), Point(280, 350),
                                     Point(240, 300), 255);
  ImageProcessor::DrawFilledTriangle(image, Point(550, 150), Point(620, 180),
                                     Point(530, 200), 255);
  ImageProcessor::DrawFilledTriangle(image, Point(700, 450), Point(770, 480),
                                     Point(720, 520), 255);

  ImageProcessor::DrawFilledEllipse(image, 400, 100, 40, 20, 0.3, 255);
  ImageProcessor::DrawFilledEllipse(image, 200, 500, 35, 25, 1.8, 255);
  ImageProcessor::DrawFilledEllipse(image, 600, 500, 45, 30, 2.5, 255);

  return image;
}

Image CreateRotatedRectanglesImage() {
  Image image(900, 700);

  // Black background
  for (int y = 0; y < 700; ++y) {
    for (int x = 0; x < 900; ++x) {
      image.pixels[y][x] = 0;
    }
  }

  // Grid of rotated rectangles at various angles
  // Row 1: 0° to 90° in 15° increments
  ImageProcessor::CreateRotatedRectangle(image, 100, 100, 80, 50, 0.0); // 0°
  ImageProcessor::CreateRotatedRectangle(image, 250, 100, 80, 50,
                                         std::numbers::pi / 12); // 15°
  ImageProcessor::CreateRotatedRectangle(image, 400, 100, 80, 50,
                                         std::numbers::pi / 6); // 30°
  ImageProcessor::CreateRotatedRectangle(image, 550, 100, 80, 50,
                                         std::numbers::pi / 4); // 45°
  ImageProcessor::CreateRotatedRectangle(image, 700, 100, 80, 50,
                                         std::numbers::pi / 3); // 60°
  ImageProcessor::CreateRotatedRectangle(image, 800, 100, 80, 50,
                                         5 * std::numbers::pi / 12); // 75°

  // Row 2: 90° to 180° in 15° increments
  ImageProcessor::CreateRotatedRectangle(image, 100, 250, 80, 50,
                                         std::numbers::pi / 2); // 90°
  ImageProcessor::CreateRotatedRectangle(image, 250, 250, 80, 50,
                                         7 * std::numbers::pi / 12); // 105°
  ImageProcessor::CreateRotatedRectangle(image, 400, 250, 80, 50,
                                         2 * std::numbers::pi / 3); // 120°
  ImageProcessor::CreateRotatedRectangle(image, 550, 250, 80, 50,
                                         3 * std::numbers::pi / 4); // 135°
  ImageProcessor::CreateRotatedRectangle(image, 700, 250, 80, 50,
                                         5 * std::numbers::pi / 6); // 150°
  ImageProcessor::CreateRotatedRectangle(image, 800, 250, 80, 50,
                                         11 * std::numbers::pi / 12); // 165°

  // Row 3: Negative angles -90° to 0° in 15° increments
  ImageProcessor::CreateRotatedRectangle(image, 100, 400, 80, 50,
                                         -std::numbers::pi / 2); // -90°
  ImageProcessor::CreateRotatedRectangle(image, 250, 400, 80, 50,
                                         -5 * std::numbers::pi / 12); // -75°
  ImageProcessor::CreateRotatedRectangle(image, 400, 400, 80, 50,
                                         -std::numbers::pi / 3); // -60°
  ImageProcessor::CreateRotatedRectangle(image, 550, 400, 80, 50,
                                         -std::numbers::pi / 4); // -45°
  ImageProcessor::CreateRotatedRectangle(image, 700, 400, 80, 50,
                                         -std::numbers::pi / 6); // -30°
  ImageProcessor::CreateRotatedRectangle(image, 800, 400, 80, 50,
                                         -std::numbers::pi / 12); // -15°

  // Row 4: Different sized rectangles at various angles
  ImageProcessor::CreateRotatedRectangle(image, 150, 550, 100, 60,
                                         std::numbers::pi / 8); // 22.5°, larger
  ImageProcessor::CreateRotatedRectangle(
      image, 350, 550, 60, 40, 3 * std::numbers::pi / 8); // 67.5°, smaller
  ImageProcessor::CreateRotatedRectangle(
      image, 550, 550, 90, 30, -3 * std::numbers::pi / 8); // -67.5°, wide
  ImageProcessor::CreateRotatedRectangle(
      image, 750, 550, 40, 80, 5 * std::numbers::pi / 8); // 112.5°, tall

  // Row 5: Square rectangles at various angles
  ImageProcessor::CreateRotatedRectangle(image, 200, 650, 60, 60,
                                         std::numbers::pi / 10); // 18°, square
  ImageProcessor::CreateRotatedRectangle(
      image, 400, 650, 60, 60, 3 * std::numbers::pi / 10); // 54°, square
  ImageProcessor::CreateRotatedRectangle(image, 600, 650, 60, 60,
                                         -std::numbers::pi / 5); // -36°, square
  ImageProcessor::CreateRotatedRectangle(
      image, 800, 650, 60, 60, 7 * std::numbers::pi / 10); // 126°, square

  return image;
}

int main() {
  std::cout << "=== Visual Rectangle Detection Tests ===" << std::endl
            << std::endl;

  // Test 1: Only circles (should detect 0 rectangles)
  Image circlesImage = CreateCirclesOnlyImage();
  CreateAndTestImage("circles_only", circlesImage,
                     "Multiple circles - should detect 0 rectangles");

  // Test 2: Only triangles (should detect 0 rectangles)
  Image trianglesImage = CreateTrianglesOnlyImage();
  CreateAndTestImage("triangles_only", trianglesImage,
                     "Multiple triangles - should detect 0 rectangles");

  // Test 3: Only rectangles (should detect all rectangles)
  Image rectanglesImage = CreateRectanglesOnlyImage();
  CreateAndTestImage("rectangles_only", rectanglesImage,
                     "Multiple rectangles - should detect all rectangles");

  // Test 4: Mixed shapes (should detect only rectangles)
  Image mixedImage = CreateMixedShapesImage();
  CreateAndTestImage("mixed_shapes", mixedImage,
                     "Mixed shapes - should detect only rectangles");

  // Test 5: Rotated rectangles (should detect all rotated rectangles)
  Image rotatedImage = CreateRotatedRectanglesImage();
  CreateAndTestImage(
      "rotated_rectangles", rotatedImage,
      "Rotated rectangles at various angles - should detect all");

  // Test 6: Complex scene (should detect only rectangles among many shapes)
  Image complexImage = CreateComplexSceneImage();
  CreateAndTestImage("complex_scene", complexImage,
                     "Complex scene - should detect only rectangles");

  std::cout << "=== All visual tests completed ===" << std::endl;
  std::cout << "Check the generated .png files in Output/Images/ to see the results!"
            << std::endl;
  std::cout << "Red outlines indicate detected rectangles." << std::endl;

  return 0;
}