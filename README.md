# C++ Shape Detection System

A high-performance computer vision application for detecting rectangles and spheres/obloids in images. Features rotation-invariant detection algorithms with comprehensive testing suite.

## Overview

- **Dual Shape Detection**: Detects rectangles and spheres/obloids (circles, ellipses)
- **Rotation Invariant**: 100% detection accuracy across all rotation angles (0°-180°)
- **High Performance**: Multi-threaded processing with OpenMP parallelization
- **No Dependencies**: Pure C++ implementation without external CV libraries
- **Comprehensive Testing**: 87 unit tests with full rotation coverage

## Performance Metrics

- Processing speed: Up to 6,259 pixels/ms
- OpenMP parallel loops: 25+
- Test coverage: 87/87 tests passing
- Rotation accuracy: 100% (all angles 0°-180°)

## Building

### Requirements

- CMake 3.10+
- C++23 compiler (GCC 11+, Clang 14+)
- OpenMP (optional but recommended)
- ImageMagick (optional, for PNG conversion)

### Build Commands

```bash
./b  # Build project and run tests
./r  # Build and run main application
./v  # Build and run visual tests
```

Or manually:

```bash
mkdir build && cd build
cmake .. && make
```

## Usage

### Main Application

```bash
./Output/CppRectangleRecognition
```

Input: PGM grayscale images  
Output: PNG images with detected shapes outlined

### Test Suite

```bash
./Output/tests                # Run all unit tests
./Output/TestPerformance      # Run performance benchmarks
./Output/VisualTest          # Generate visual test images
```

## Project Structure

```
CppRectangleRecognition/
├── Include/              # Header files
│   └── ShapeDetector/    # Detection algorithms
├── Source/               # Implementation files
├── Test/                 # Unit tests
├── Output/               # Executables and output images
├── build/                # Build artifacts
└── CMakeLists.txt        # Build configuration
```

## Algorithm Overview

### Detection Pipeline

1. **Multi-Strategy Preprocessing**
   - Standard contour detection
   - Enhanced edge detection
   - Morphological processing
   - Multi-threshold analysis
   - Edge-preserving filtering

2. **Shape Analysis**
   - Rectangle: Corner validation, parallel sides, rotation invariance
   - Sphere/Obloid: Circularity analysis, ellipticity metrics, radial consistency

3. **Validation**
   - Multi-level validation (strict/moderate/relaxed)
   - Moment-based shape classification
   - Geometric property verification

## Configuration

### Rectangle Detector

```cpp
detector.SetMinArea(100.0);      // Minimum area threshold
detector.SetMaxArea(50000.0);    // Maximum area threshold
detector.SetApproxEpsilon(0.02); // Contour approximation precision
```

### Sphere Detector

```cpp
detector.SetMinRadius(10);       // Minimum radius
detector.SetMaxRadius(200);      // Maximum radius
detector.SetCircularityThreshold(0.7); // Circularity threshold
```

## Performance Optimizations

- **Compiler**: -O3, -march=native, -flto, -ffast-math
- **Parallelization**: OpenMP on all critical loops
- **Algorithm**: Early exit conditions, pre-allocated caches
- **Memory**: Cache-friendly access patterns

## Testing

The project includes comprehensive test coverage:

- Geometry primitives testing
- Image processing validation
- Rectangle detection (all rotations)
- Sphere/obloid detection
- Performance benchmarks
- Visual verification suite

## Output Examples

Visual test suite generates test scenarios:

- `visual_test_circles_only.png`: Circle detection
- `visual_test_rectangles_only.png`: Rectangle detection
- `visual_test_rotated_rectangles.png`: Rotation testing
- `visual_test_mixed_shapes.png`: Multi-shape discrimination
- `visual_test_complex_scene.png`: Complex scene analysis

## License

MIT License