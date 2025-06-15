#include "ShapeDetector/ImageProcessor.hpp"
#include "ShapeDetector/RectangleDetector.hpp"
#include <chrono>
#include <iostream>
#include <omp.h>
#include <vector>

using namespace std::chrono;

void testPerformance() {
  std::cout << "Performance Test for Rectangle Detection\n";
  std::cout << "========================================\n\n";

  // Test with different image sizes
  std::vector<int> sizes = {100, 200, 400, 800, 1600};

  for (int size : sizes) {
    std::cout << "Testing with image size: " << size << "x" << size << "\n";

    // Create test image with multiple rectangles
    Image testImage = ImageProcessor::CreateTestImage(size, size);

    // Create detector
    RectangleDetector detector;
    detector.SetMinArea(100.0);
    detector.SetMaxArea(size * size * 0.5);

    // Measure detection time
    auto start = high_resolution_clock::now();
    std::vector<Rectangle> rectangles = detector.DetectRectangles(testImage);
    auto end = high_resolution_clock::now();

    auto duration_ms = duration_cast<milliseconds>(end - start);
    auto duration_us = duration_cast<microseconds>(end - start);
    auto duration_ns = duration_cast<nanoseconds>(end - start);

    std::cout << "  - Detected " << rectangles.size() << " rectangles\n";

    if (duration_ms.count() == 0) {
      if (duration_us.count() == 0) {
        std::cout << "  - Time taken: " << duration_ns.count() << " ns\n";
        std::cout << "  - Processing rate: "
                  << (static_cast<long long>(size) * size * 1000000000) /
                         (duration_ns.count() + 1)
                  << " pixels/s\n\n";
      } else {
        std::cout << "  - Time taken: " << duration_us.count() << " µs\n";
        std::cout << "  - Processing rate: "
                  << (static_cast<long long>(size) * size * 1000000) /
                         (duration_us.count() + 1)
                  << " pixels/s\n\n";
      }
    } else {
      std::cout << "  - Time taken: " << duration_ms.count() << " ms\n";
      std::cout << "  - Processing rate: "
                << (size * size) / (duration_ms.count() + 1)
                << " pixels/ms\n\n";
    }
  }

  // Test with a complex image (many small rectangles)
  std::cout << "Testing with complex image (many small rectangles)...\n";
  Image complexImage(1000, 1000);

  // Create grid of small rectangles
#pragma omp parallel for
  for (int y = 10; y < 990; y += 50) {
    for (int x = 10; x < 990; x += 50) {
      // Draw small rectangle
      for (int dy = 0; dy < 30; dy++) {
        for (int dx = 0; dx < 30; dx++) {
          if (dy == 0 || dy == 29 || dx == 0 || dx == 29) {
            complexImage.pixels[y + dy][x + dx] = 255;
          }
        }
      }
    }
  }

  RectangleDetector detector;
  detector.SetMinArea(50.0);
  detector.SetMaxArea(10000.0);

  auto start = high_resolution_clock::now();
  std::vector<Rectangle> rectangles = detector.DetectRectangles(complexImage);
  auto end = high_resolution_clock::now();

  auto duration_ms = duration_cast<milliseconds>(end - start);
  auto duration_us = duration_cast<microseconds>(end - start);
  auto duration_ns = duration_cast<nanoseconds>(end - start);

  std::cout << "  - Detected " << rectangles.size() << " rectangles\n";

  if (duration_ms.count() == 0) {
    if (duration_us.count() == 0) {
      std::cout << "  - Time taken: " << duration_ns.count() << " ns\n";
      if (rectangles.size() > 0) {
        std::cout << "  - Average time per rectangle: "
                  << duration_ns.count() / rectangles.size() << " ns\n";
      }
    } else {
      std::cout << "  - Time taken: " << duration_us.count() << " µs\n";
      if (rectangles.size() > 0) {
        std::cout << "  - Average time per rectangle: "
                  << duration_us.count() / rectangles.size() << " µs\n";
      }
    }
  } else {
    std::cout << "  - Time taken: " << duration_ms.count() << " ms\n";
    if (rectangles.size() > 0) {
      auto avg_ms =
          static_cast<double>(duration_ms.count()) / rectangles.size();
      if (avg_ms < 1.0) {
        auto avg_us = duration_us.count() / rectangles.size();
        if (avg_us == 0) {
          std::cout << "  - Average time per rectangle: "
                    << duration_ns.count() / rectangles.size() << " ns\n";
        } else {
          std::cout << "  - Average time per rectangle: " << avg_us << " µs\n";
        }
      } else {
        std::cout << "  - Average time per rectangle: " << avg_ms << " ms\n";
      }
    }
  }

  // Test with mixed shapes to evaluate discrimination capability
  std::cout << "\nTesting rectangle detection with mixed shapes...\n";
  std::cout << "-----------------------------------------------\n\n";

  for (int size : sizes) {
    std::cout << "Testing mixed shapes image: " << size << "x" << size << "\n";

    // Create test image with mixed shapes
    Image mixedImage =
        ImageProcessor::CreateTestImageWithMixedShapes(size, size);

    // Create detector
    RectangleDetector mixedDetector;
    mixedDetector.SetMinArea(100.0);
    mixedDetector.SetMaxArea(size * size * 0.5);

    // Count actual rectangles in the mixed shapes image
    int expectedRectangles =
        3; // We know we add 3 rectangles in CreateTestImageWithMixedShapes

    // Measure detection time
    auto start = high_resolution_clock::now();
    std::vector<Rectangle> detectedRectangles =
        mixedDetector.DetectRectangles(mixedImage);
    auto end = high_resolution_clock::now();

    auto duration_ms = duration_cast<milliseconds>(end - start);
    auto duration_us = duration_cast<microseconds>(end - start);
    auto duration_ns = duration_cast<nanoseconds>(end - start);

    std::cout << "  - Expected rectangles: " << expectedRectangles << "\n";
    std::cout << "  - Detected rectangles: " << detectedRectangles.size()
              << "\n";
    std::cout << "  - Detection accuracy: "
              << (detectedRectangles.size() <= expectedRectangles
                      ? 100.0 * detectedRectangles.size() / expectedRectangles
                      : 100.0 * expectedRectangles / detectedRectangles.size())
              << "%\n";

    if (duration_ms.count() == 0) {
      if (duration_us.count() == 0) {
        std::cout << "  - Time taken: " << duration_ns.count() << " ns\n";
      } else {
        std::cout << "  - Time taken: " << duration_us.count() << " µs\n";
      }
    } else {
      std::cout << "  - Time taken: " << duration_ms.count() << " ms\n";
    }
    std::cout << "\n";
  }
}

int main() {
  testPerformance();
  return 0;
}