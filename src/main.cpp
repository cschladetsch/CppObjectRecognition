#include "shape_detector/rectangle_detector.h"
#include "shape_detector/image_processor.h"
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <fstream>

void processImage(RectangleDetector& detector, int testNumber) {
    std::cout << "\n=== Test " << testNumber << " ===\n";
    std::cout << "Creating test image with rectangles...\n";
    Image testImage = ImageProcessor::createTestImage(400, 300);
    
    std::cout << "Detecting rectangles...\n";
    std::vector<Rectangle> rectangles = detector.detectRectangles(testImage);
    
    std::cout << "Found " << rectangles.size() << " rectangles:\n";
    
    for (size_t i = 0; i < rectangles.size(); ++i) {
        const Rectangle& rect = rectangles[i];
        std::cout << "Rectangle " << (i + 1) << ":\n";
        std::cout << "  Center: (" << rect.center.x << ", " << rect.center.y << ")\n";
        std::cout << "  Size: " << rect.width << " x " << rect.height << "\n";
        std::cout << "  Angle: " << rect.angle << " degrees\n";
        std::cout << "  Mathematical representation: center=(" << rect.center.x << "," << rect.center.y 
                  << "), size=" << rect.width << "x" << rect.height 
                  << ", angle=" << rect.angle << "°\n";
        std::cout << "\n";
    }
    
    std::cout << "Creating color output image with detected rectangles...\n";
    ColorImage outputImage = ImageProcessor::createColorImage(testImage, rectangles);
    
    std::cout << "Saving output image...\n";
    ImageProcessor::savePPMImage(outputImage, "output.ppm");
    
    std::cout << "Processing complete! Output saved as: output.ppm\n";
    
    // Convert to PNG and display the result
    std::cout << "Converting to PNG and displaying result...\n";
    
    // Convert PPM to PNG using ImageMagick
    int convertResult = system("convert output.ppm output.png 2>/dev/null");
    if (convertResult == 0) {
        std::cout << "Converted to output.png\n";
        
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
            system("explorer.exe output.png 2>/dev/null &");
        } else {
            // Native Linux - try various image viewers
            std::cout << "Trying to open with image viewer...\n";
            if (system("which display >/dev/null 2>&1") == 0) {
                system("display output.ppm &");
            } else if (system("which eog >/dev/null 2>&1") == 0) {
                system("eog output.png &");
            } else if (system("which feh >/dev/null 2>&1") == 0) {
                system("feh output.png &");
            } else if (system("which xdg-open >/dev/null 2>&1") == 0) {
                system("xdg-open output.png &");
            } else {
                std::cout << "No image viewer found. Please view output.png manually.\n";
            }
        }
    } else {
        std::cout << "Warning: Could not convert to PNG. ImageMagick may not be installed.\n";
        std::cout << "You can view the raw PPM file: output.ppm\n";
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
    std::cout << "Press SPACE to generate new test, 'q' to quit\n\n";
    
    RectangleDetector detector;
    detector.setMinArea(200.0);
    detector.setMaxArea(8000.0);
    detector.setApproxEpsilon(0.05);
    
    int testNumber = 1;
    
    // Generate initial test
    processImage(detector, testNumber++);
    
    while (true) {
        std::cout << "\nPress SPACE for new test, 'q' to quit: ";
        std::cout.flush();
        
        char input = getChar();
        std::cout << "\n";
        
        if (input == ' ') {
            processImage(detector, testNumber++);
        } else if (input == 'q' || input == 'Q') {
            std::cout << "Exiting...\n";
            break;
        } else {
            std::cout << "Unknown command. Press SPACE for new test, 'q' to quit.\n";
        }
    }
    
    return 0;
}