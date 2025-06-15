#include "ShapeDetector/ImageProcessor.hpp"
#include "ShapeDetector/RectangleDetector.hpp"
#include <cmath>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <termios.h>
#include <unistd.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void processImage(RectangleDetector &detector, int testNumber,
                  bool useMixedShapes = false) {
  std::cout << "\n=== Test " << testNumber << " ===\n";
  std::cout << "Creating test image with "
            << (useMixedShapes ? "mixed shapes" : "rectangles only") << "...\n";
  Image testImage =
      useMixedShapes ? ImageProcessor::CreateTestImageWithMixedShapes(400, 300)
                     : ImageProcessor::CreateTestImage(400, 300);

  std::cout << "Detecting rectangles...\n";
  std::vector<Rectangle> rectangles = detector.DetectRectangles(testImage);

  std::cout << "Found " << rectangles.size() << " rectangles:\n";

  for (size_t i = 0; i < rectangles.size(); ++i) {
    const Rectangle &rect = rectangles[i];
    std::cout << "Rectangle " << (i + 1) << ":\n";
    std::cout << "  Center: (" << rect.center.x << ", " << rect.center.y
              << ")\n";
    std::cout << "  Size: " << rect.width << " x " << rect.height << "\n";
    std::cout << "  Angle: " << rect.angle << " radians ("
              << (rect.angle * 180.0 / M_PI) << " degrees)\n";
    std::cout << "  Mathematical representation: center=(" << rect.center.x
              << "," << rect.center.y << "), size=" << rect.width << "x"
              << rect.height << ", angle=" << rect.angle << " rad\n";
    std::cout << "\n";
  }

  std::cout << "Creating color output image with detected rectangles...\n";
  ColorImage outputImage =
      ImageProcessor::CreateColorImage(testImage, rectangles);

  std::cout << "Saving output image...\n";
  ImageProcessor::SavePNGImage(outputImage, "Output/Images/output.png");

  std::cout << "Processing complete! Output saved as: Output/Images/output.png\n";

  // Display the result
  std::cout << "Displaying result...\n";

  int convertResult = 0;
  if (convertResult == 0) {
    std::cout << "Image saved successfully\n";

    // Check if we're in WSL and try to open with Windows explorer
    std::ifstream versionFile("/proc/version");
    std::string versionContent;
    if (versionFile) {
      std::getline(versionFile, versionContent);
      versionFile.close();
    }

    if (versionContent.find("microsoft") != std::string::npos ||
        versionContent.find("Microsoft") != std::string::npos) {
      // WSL detected - use Windows explorer
      std::cout << "Opening with Windows explorer...\n";
      system("explorer.exe Output/Images/output.png 2>/dev/null &");
    } else {
      // Native Linux - try various image viewers
      std::cout << "Trying to open with image viewer...\n";
      if (system("which eog >/dev/null 2>&1") == 0) {
        system("eog Output/Images/output.png &");
      } else if (system("which feh >/dev/null 2>&1") == 0) {
        system("feh Output/Images/output.png &");
      } else if (system("which xdg-open >/dev/null 2>&1") == 0) {
        system("xdg-open Output/Images/output.png &");
      } else {
        std::cout
            << "No image viewer found. Please view Output/Images/output.png manually.\n";
      }
    }
  } else {
    std::cout << "Warning: Could not save PNG file.\n";
    std::cout << "Please check Output/Images/ directory permissions.\n";
  }
}

char getChar() {
  char ch;
  struct termios old_termios, new_termios;

  // Get current terminal settings
  tcgetattr(STDIN_FILENO, &old_termios);
  new_termios = old_termios;

  // Disable canonical mode and echo
  new_termios.c_lflag &= ~(ICANON | ECHO);
  new_termios.c_cc[VMIN] = 1;
  new_termios.c_cc[VTIME] = 0;

  // Apply new settings
  tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

  // Read character
  ch = getchar();

  // Restore original settings
  tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);

  return ch;
}

int main() {
  std::cout << "Rectangle Detection System\n";
  std::cout << "=========================\n";
  std::cout << "Controls:\n";
  std::cout << "  SPACE - Generate new test with rectangles only\n";
  std::cout << "  M     - Generate new test with mixed shapes\n";
  std::cout << "  Q     - Quit\n\n";

  RectangleDetector detector;
  detector.SetMinArea(200.0);
  detector.SetMaxArea(8000.0);
  detector.SetApproxEpsilon(0.05);

  int testNumber = 1;

  // Generate initial test
  processImage(detector, testNumber++);

  while (true) {
    std::cout << "\nPress SPACE (rectangles), M (mixed shapes), or Q (quit): ";
    std::cout.flush();

    char input = getChar();
    std::cout << "\n";

    if (input == ' ') {
      processImage(detector, testNumber++, false);
    } else if (input == 'm' || input == 'M') {
      processImage(detector, testNumber++, true);
    } else if (input == 'q' || input == 'Q') {
      std::cout << "Exiting...\n";
      break;
    } else {
      std::cout << "Unknown command. Press SPACE (rectangles), M (mixed "
                   "shapes), or Q (quit).\n";
    }
  }

  return 0;
}