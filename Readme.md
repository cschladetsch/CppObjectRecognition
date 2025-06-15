# Rectangle Detection System

A C++ computer vision application that detects rectangles in images using custom image processing algorithms.

Does not use any libraries, just naked C++.

## Demo

![Demo](resources/Demo.png)

## Features

- **Custom Rectangle Detection**: Pure C++ implementation without external dependencies
- **Real-time Processing**: Optimized algorithms for fast detection
- **Interactive Interface**: Terminal-based UI with keyboard controls
- **Test Image Generation**: Built-in generator for synthetic test images with rotated rectangles
- **Visualization**: Color-coded output showing detected rectangles with:
  - Red outlines for boundaries
  - Green dots for centers
  - Blue lines for orientation
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
│       ├── ImageProcessor.h       # Image processing utilities
│       └── RectangleDetector.h    # Rectangle detection algorithms
├── Source/
│   ├── ImageProcessor.cpp         # Image processing implementation
│   ├── Main.cpp                   # Main application
│   └── RectangleDetector.cpp      # Rectangle detection implementation
├── Test/                          # Unit tests
│   ├── TestGeometry.cpp           # Geometry structure tests
│   ├── TestImageProcessor.cpp     # Image processing tests
│   ├── TestMain.cpp               # Test runner
│   ├── TestRectangleDetector.cpp  # Rectangle detection tests
│   └── TestPerformance.cpp        # Performance benchmarks
├── build/                         # Build directory (generated)
├── resources/                     # Demo images and resources
└── CMakeLists.txt                # CMake configuration
```

## Building

### Prerequisites

- CMake 3.10 or higher
- C++17 compatible compiler (GCC, Clang)
- ImageMagick (optional, for PNG conversion)

### Build Instructions

Use scripts:

```bash
$ ./b # just build the project and test
$ ./r # calls b then runs  the app
$ ./v # calls r then opens the result using explorer or equivalent on current platform
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

- Press **SPACE** to generate a new test image with random rectangles
- Press **Q** to quit the application
- Detected rectangles are displayed with:
  - Red outlines showing rectangle boundaries
  - Green dots marking rectangle centers
  - Blue lines indicating rectangle orientation
- Output images are saved as `output.ppm` and converted to `output.png` if ImageMagick is available

## Algorithm Details

The rectangle detection system uses:

1. **Test Image Generation**: Creates synthetic images with rotated rectangles
2. **Edge Detection**: Identifies potential rectangle boundaries
3. **Contour Analysis**: Finds closed contours in the image
4. **Shape Approximation**: Simplifies contours to identify rectangular shapes
5. **Filtering**: Removes false positives based on area and aspect ratio

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
- **TestRectangleDetector.cpp**: Tests for rectangle detection algorithms and edge cases
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
