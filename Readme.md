# Rectangle Detection System

A C++ computer vision application that detects rectangles in images using custom image processing algorithms with comprehensive visual testing capabilities.

Does not use any libraries, just naked C++.

## Demo

Rects could be rotated, in this demo image they are not.

![Demo](resources/Demo.png)

## Features

- **Custom Rectangle Detection**: Pure C++ implementation without external dependencies
- **Shape Discrimination**: Accurately detects only rectangles while ignoring circles, triangles, ellipses, and other shapes
- **Rotated Rectangle Support**: Detects rectangles at any angle from 0° to 360°
- **Comprehensive Visual Testing**: Automated test suite with 6 different test scenarios
- **Real-time Processing**: Optimized algorithms for fast detection
- **Interactive Interface**: Terminal-based UI with keyboard controls
- **Test Image Generation**: Built-in generator for synthetic test images with rotated rectangles
- **Thick Visual Outlines**: 4-pixel thick red outlines for clear visualization of detected rectangles
- **Cross-platform**: Supports Linux, macOS, and Windows (via WSL)
- **Performance Optimized**: 
  - Multi-threaded processing with OpenMP
  - Efficient scanline flood fill algorithm
  - Optimized contour approximation
  - Cache-friendly data structures

## Project Structure

```
CppRectangleRecognition/
├── Include/
│   └── ShapeDetector/
│       ├── ImageProcessor.hpp     # Image processing utilities
│       └── RectangleDetector.hpp  # Rectangle detection algorithms
├── Source/
│   ├── ImageProcessor.cpp         # Image processing implementation
│   ├── Main.cpp                   # Main application
│   ├── RectangleDetector.cpp      # Rectangle detection implementation
│   └── VisualTest.cpp             # Visual testing suite
├── Test/                          # Unit tests
│   ├── TestGeometry.cpp           # Geometry structure tests
│   ├── TestImageProcessor.cpp     # Image processing tests
│   ├── TestMain.cpp               # Test runner
│   ├── TestRectangleDetector.cpp  # Rectangle detection tests
│   └── TestPerformance.cpp        # Performance benchmarks
├── build/                         # Build directory (generated)
├── resources/                     # Demo images and resources
├── b                              # Build script
├── r                              # Run script  
├── v                              # Visual test script
└── CMakeLists.txt                # CMake configuration
```

## Building

### Prerequisites

- CMake 3.10 or higher
- C++23 compatible compiler (GCC 11+, Clang 14+) for `std::numbers`
- ImageMagick (optional, for PNG conversion in visual tests)

### Build Instructions

Use scripts:

```bash
$ ./b # Build the project and run unit tests
$ ./r # Build and run the main rectangle detection application
$ ./v # Build and run visual test suite with automatic image viewing
```

Or manually:

```bash
$ mkdir build && cd build && cmake .. && make
```

### Running

```bash
cd build
./CppRectangleRecognition
```

## Usage

### Main Application

```bash
cd build
./CppRectangleRecognition input_image.pgm
```

- Input: PGM grayscale images
- Output: PPM color images with detected rectangles outlined in red
- Detected rectangles are displayed with red boundary outlines

### Visual Testing Suite

```bash
./v  # Runs comprehensive visual tests
```

The visual testing suite generates 6 different test scenarios:

1. **circles_only.png** - Multiple circles (should detect 0 rectangles)
2. **triangles_only.png** - Multiple triangles (should detect 0 rectangles)  
3. **rectangles_only.png** - Multiple axis-aligned rectangles (should detect all)
4. **mixed_shapes.png** - Mixed shapes with rectangles, circles, triangles, ellipses (should detect only rectangles)
5. **rotated_rectangles.png** - 22+ rectangles at various angles from 0° to 165° in 15° increments (should detect all rotated rectangles)
6. **complex_scene.png** - Complex scene with many shapes (should detect only rectangles)

#### Rotated Rectangle Test Details

The rotated rectangles test includes:
- **Row 1**: 0° to 75° in 15° increments (6 rectangles)
- **Row 2**: 90° to 165° in 15° increments (6 rectangles) 
- **Row 3**: -90° to -15° in 15° increments (6 rectangles)
- **Row 4**: Different sized rectangles (22.5°, 67.5°, -67.5°, 112.5°) - 4 rectangles
- **Row 5**: Square rectangles at various angles (18°, 54°, -36°, 126°) - 4 rectangles

**Total: 26 rotated rectangles** demonstrating detection at every angle

## Algorithm Details

The rectangle detection system uses advanced shape discrimination:

1. **Image Preprocessing**: Binary thresholding for clean edge detection
2. **Contour Extraction**: Efficient scanline flood fill to find connected components
3. **Boundary Approximation**: Douglas-Peucker algorithm for contour simplification
4. **Shape Analysis**: 
   - 4-corner validation (must have exactly 4 vertices)
   - Corner angle verification (angles must be ~90°)
   - Parallel side detection (opposite sides must be parallel)
   - Area filtering (configurable min/max area thresholds)
   - Rectangularity ratio (contour area vs bounding box area)
5. **Rotation Handling**: Detects rectangles at any angle using geometric transformations
6. **Shape Discrimination**: Specifically rejects circles, triangles, ellipses, and irregular polygons

## Testing

The project includes comprehensive unit tests and performance benchmarks.

### Unit Tests

Run the test suite:

```bash
cd build
make test
```

Or run tests directly:

```bash
./tests
```

#### Test Coverage

The test suite includes:

- **TestGeometry.cpp**: Tests for geometric primitives (Point, Rectangle, Image structures)
- **TestImageProcessor.cpp**: Tests for image I/O, filtering, and manipulation functions
- **TestRectangleDetector.cpp**: Comprehensive shape discrimination tests:
  - `OnlyDetectsCircles_ShouldFindZero` - Verifies circles are not detected as rectangles
  - `OnlyDetectsTriangles_ShouldFindZero` - Verifies triangles are not detected as rectangles
  - `OnlyDetectsEllipses_ShouldFindZero` - Verifies ellipses are not detected as rectangles
  - `DetectsOnlyRectanglesAmongMixedShapes` - Verifies selective rectangle detection
  - `DetectsOnlySquaresAsRectangles` - Verifies squares are correctly identified as rectangles
- **TestMain.cpp**: Google Test framework runner

All tests use Google Test framework and run automatically during the build process.

### Performance Testing

The project includes a comprehensive performance benchmark tool that measures rectangle detection speed across various scenarios.

Run performance benchmarks:

```bash
cd build
./TestPerformance
```

#### What TestPerformance Measures

1. **Scalability Testing**: Tests detection performance on images of increasing sizes (100x100 to 1600x1600 pixels)
2. **Complex Scene Testing**: Evaluates performance on images with many small rectangles (grid pattern)
3. **Timing Precision**: Automatically switches between nanoseconds (ns), microseconds (µs), and milliseconds (ms) for accurate measurements
4. **Processing Rate**: Calculates pixels processed per unit time

#### Example Output

```
Performance Test for Rectangle Detection
========================================

Testing with image size: 100x100
  - Detected 1 rectangles
  - Time taken: 21 ms
  - Processing rate: 454 pixels/ms

Testing with image size: 200x200
  - Detected 1 rectangles
  - Time taken: 3 ms
  - Processing rate: 10000 pixels/ms

Testing with image size: 400x400
  - Detected 3 rectangles
  - Time taken: 9 ms
  - Processing rate: 16000 pixels/ms

Testing with image size: 800x800
  - Detected 4 rectangles
  - Time taken: 29 ms
  - Processing rate: 21333 pixels/ms

Testing with image size: 1600x1600
  - Detected 4 rectangles
  - Time taken: 93 ms
  - Processing rate: 27234 pixels/ms

Testing with complex image (many small rectangles)...
  - Detected 400 rectangles
  - Time taken: 63 ms
  - Average time per rectangle: 159 µs
```

#### Performance Characteristics

- **Linear Scalability**: Processing time scales roughly linearly with image area
- **Optimized for Larger Images**: Better pixels/ms rate on larger images due to algorithm efficiency
- **Sub-millisecond Detection**: Individual rectangles can be detected in microseconds
- **No External Dependencies**: Pure C++ implementation ensures consistent performance

## Configuration

Rectangle detection parameters can be adjusted in `Main.cpp`:

- `SetMinArea()`: Minimum rectangle area threshold
- `SetMaxArea()`: Maximum rectangle area threshold  
- `SetApproxEpsilon()`: Contour approximation precision

Example configuration:
```cpp
RectangleDetector detector;
detector.SetMinArea(100.0);      // Ignore rectangles smaller than 100 pixels²
detector.SetMaxArea(50000.0);    // Ignore rectangles larger than 50000 pixels²
detector.SetApproxEpsilon(0.02); // Contour approximation precision (2% of perimeter)
```

## Output

The application generates:
- Console output with detected rectangle coordinates and properties
- `output.ppm`: Raw image output with detected rectangles highlighted
- `output.png`: PNG version (if ImageMagick is available)

## License

This project is licensed under the MIT Licence. See the LICENSE file.
