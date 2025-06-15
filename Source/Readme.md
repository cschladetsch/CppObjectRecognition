# Source Directory

This directory contains the implementation files for the C++ Rectangle Recognition system.

## Directory Structure

```
Source/
├── Main.cpp              # Application entry point and CLI interface
├── RectangleDetector.cpp # Core rectangle detection implementation
├── ImageProcessor.cpp   # Image processing and manipulation implementation
└── VisualTest.cpp       # Visual testing and output generation
```

## Implementation Files Overview

### 🚀 **Main.cpp**
Application entry point providing command-line interface and user interaction.

**Features:**
```
┌─────────────────────────────────────────────────────┐
│                Main Application                     │
├─────────────────────────────────────────────────────┤
│ • Command-line argument parsing                     │
│ • Interactive keyboard controls                     │
│ • Performance benchmarking                         │
│ • Cross-platform image viewing                     │
│ • Real-time parameter adjustment                   │
└─────────────────────────────────────────────────────┘
```

**Command Line Interface:**
```bash
./CppRectangleRecognition [image_number] [options]

Arguments:
  image_number    Test image selector (1-6)
                  1: Single rectangle
                  2: Multiple rectangles  
                  3: Rotated rectangles
                  4: Mixed shapes
                  5: Complex scene
                  6: User-defined test

Options:
  --help         Show help information
  --benchmark    Run performance tests
  --visual       Enable visual output mode
```

**Interactive Controls:**
```
┌─────────────────────────────────────────────┐
│            Keyboard Controls                │
├─────────────────────────────────────────────┤
│  ESC/Q    │  Exit application               │
│  SPACE    │  Process current image          │
│  R        │  Reload and reprocess           │
│  S        │  Save results to file           │
│  1-6      │  Switch test images             │
│  +/-      │  Adjust detection sensitivity   │
│  Arrow    │  Fine-tune parameters           │
└─────────────────────────────────────────────┘
```

### 🧠 **RectangleDetector.cpp**
Core implementation of the state-of-the-art rectangle detection algorithms.

**Architecture Overview:**
```
┌─────────────────────────────────────────────────────────────────┐
│                    RectangleDetector Implementation              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────┐  │
│  │ Multi-Strategy  │    │ Shape Analysis  │    │ Validation  │  │
│  │   Pipeline      │───▶│    Engine       │───▶│   System    │  │
│  └─────────────────┘    └─────────────────┘    └─────────────┘  │
│           │                       │                       │     │
│           ▼                       ▼                       ▼     │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────┐  │
│  │ 5 Preprocessing │    │ Contour Extract │    │ Duplicate   │  │
│  │   Strategies    │    │ & Approximation │    │ Removal     │  │
│  └─────────────────┘    └─────────────────┘    └─────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

**Multi-Strategy Detection Pipeline:**

1. **Strategy 1: Standard Contour Detection**
   ```cpp
   Image PreprocessImage(const Image& image) {
       // Binary thresholding at optimal level
       // Gaussian blur for noise reduction
       // Standard edge detection
   }
   ```

2. **Strategy 2: Enhanced Edge Detection**
   ```cpp
   Image PreprocessImageEnhanced(const Image& image) {
       // Sobel edge detection for steep angles
       // Gradient-based preprocessing
       // Edge-preserving smoothing
   }
   ```

3. **Strategy 3: Morphological Processing**
   ```cpp
   Image PreprocessImageMorphological(const Image& image) {
       // Morphological closing (connects broken edges)
       // Morphological opening (removes noise)
       // Structure-preserving operations
   }
   ```

4. **Strategy 4: Multi-Threshold Analysis**
   ```cpp
   Image PreprocessImageMultiThreshold(const Image& image) {
       // Adaptive thresholding
       // Multiple threshold levels
       // Critical angle optimization
   }
   ```

5. **Strategy 5: Aggressive Edge-Preserving**
   ```cpp
   Image PreprocessImageAggressive(const Image& image) {
       // Median filtering for noise reduction
       // Bilateral-like filtering
       // Very low threshold for weak edges
   }
   ```

**Shape Validation Hierarchy:**
```
Level 1: STRICT VALIDATION
├─ 3+ valid corners (angle ≈ 90°)
├─ Low average angle deviation (< 0.4)
└─ High confidence rectangles

Level 2: MODERATE VALIDATION  
├─ 2+ valid corners
├─ Moderate angle deviation (< 0.6)
├─ Geometric quadrilateral checks
└─ Parallel side verification

Level 3: RELAXED VALIDATION
├─ 1+ valid corner
├─ Higher tolerance (< 0.8)
├─ Moment-based analysis
└─ Statistical shape properties
```

**Moment-Based Analysis:**
```cpp
bool IsRectangleUsingMoments(const std::vector<Point>& contour) {
    // Hu moments for rotation invariance
    // Moment ratio analysis
    // Skewness verification
    // Aspect ratio bounds
    // Circularity rejection (> 0.8)
    // Compactness analysis (> 0.7)
    // Ellipticity discrimination
}
```

**Performance Optimizations:**
```
┌──────────────────────────────────────────────┐
│              Optimization Techniques          │
├──────────────────────────────────────────────┤
│ • OpenMP parallel processing                 │
│ • Efficient memory management                │
│ • Cache-friendly algorithms                  │
│ • Early termination conditions              │
│ • Vectorized operations where possible      │
│ • Minimal memory allocations                │
└──────────────────────────────────────────────┘
```

### 🖼️ **ImageProcessor.cpp**
Comprehensive image processing and manipulation implementation.

**Core Processing Pipeline:**
```
Input Image
     │
     ▼
┌─────────────────────────────────────────────┐
│            Preprocessing                    │
├─────────────────────────────────────────────┤
│ • Gaussian blur (σ = 1.0-2.0)             │
│ • Noise reduction                          │
│ • Contrast enhancement                     │
└─────────────────────────────────────────────┘
     │
     ▼
┌─────────────────────────────────────────────┐
│            Edge Detection                   │
├─────────────────────────────────────────────┤
│ • Sobel operators (Gx, Gy)                │
│ • Gradient magnitude calculation          │
│ • Direction-aware processing              │
└─────────────────────────────────────────────┘
     │
     ▼
┌─────────────────────────────────────────────┐
│            Morphological Ops                │
├─────────────────────────────────────────────┤
│ • Closing: connects broken edges           │
│ • Opening: removes small noise             │
│ • Configurable kernel sizes               │
└─────────────────────────────────────────────┘
     │
     ▼
Binary Image Output
```

**Image I/O Support:**
```
Input Formats:        Output Formats:
┌─────────────┐      ┌─────────────┐
│ • PGM       │      │ • PGM       │
│ • PPM       │      │ • PPM       │
│ • Raw data  │ ───▶ │ • PNG       │
│ • Synthetic │      │ • Visual    │
└─────────────┘      └─────────────┘
```

**Shape Drawing Utilities:**
```cpp
// Geometric primitives
DrawFilledCircle(image, cx, cy, radius, value);
DrawFilledTriangle(image, p1, p2, p3, value);  
DrawFilledEllipse(image, cx, cy, a, b, angle, value);
CreateRotatedRectangle(image, cx, cy, w, h, angle);

// Test image generation
CreateTestImageWithMixedShapes(width, height);
CreateRotatedRectanglesTestImage(width, height);
```

### 🎨 **VisualTest.cpp**
Visual testing framework for comprehensive algorithm validation.

**Test Scenarios:**
```
Test Suite 1: BASIC SHAPES
┌─────────┐ ┌─────────┐ ┌─────────┐
│  Rect   │ │ Square  │ │ Multiple│
│         │ │         │ │ Rects   │
└─────────┘ └─────────┘ └─────────┘

Test Suite 2: ROTATED RECTANGLES
     ╱╲           ╱─────╲       ─────
    ╱  ╲         ╱       ╲        │
   ╱    ╲       ╱         ╲       │
  ╱      ╲     ╱           ╲      │
 ╱        ╲   ╱─────────────╲   ─────
  22.5°        45°           90°

Test Suite 3: MIXED SHAPES
┌────┐    ●      ▲     ◯     ⬟
│    │  Circle Triangle Ellipse Pentagon
└────┘

Test Suite 4: STRESS TESTING
Multiple overlapping shapes, noise, edge cases
```

**Visualization Features:**
```cpp
// Result overlays
void DrawDetectionResults(ColorImage& image, 
                         const std::vector<Rectangle>& rectangles);

// Performance metrics
void DisplayPerformanceStats(const DetectionResults& results);

// Comparison views
void ShowBeforeAfterComparison(const Image& original,
                              const Image& processed,
                              const std::vector<Rectangle>& results);
```

## Algorithm Implementation Details

### 🔍 **Contour Extraction**
```
Scanline Flood Fill Algorithm:
┌─────────────────────────────────────────┐
│  for each pixel (x,y) in image:        │
│    if pixel is unvisited and white:    │
│      start_new_contour()                │
│      flood_fill_4_connected(x,y)       │
│      extract_boundary_points()         │
│      store_contour()                   │
└─────────────────────────────────────────┘

Boundary Following:
┌─────────────────────────────────────────┐
│  starting from seed point:              │
│    follow_boundary_clockwise()         │
│    record_boundary_pixels()            │
│    detect_chain_code()                 │
│    simplify_using_douglas_peucker()    │
└─────────────────────────────────────────┘
```

### 📐 **Geometric Validation**
```cpp
// Corner angle calculation
double calculateCornerAngle(Point p1, Point p2, Point p3) {
    Vector v1 = normalize(p1 - p2);
    Vector v2 = normalize(p3 - p2);
    double cosAngle = dot(v1, v2);
    return std::acos(std::clamp(cosAngle, -1.0, 1.0));
}

// Parallel sides check
bool areParallelSides(const std::vector<Point>& corners) {
    Vector side1 = corners[1] - corners[0];
    Vector side3 = corners[3] - corners[2];
    Vector side2 = corners[2] - corners[1];
    Vector side4 = corners[0] - corners[3];
    
    return isParallel(side1, side3) && isParallel(side2, side4);
}
```

### 🧮 **Mathematical Foundations**
```
Hu Moments (Rotation Invariant):
φ₁ = η₂₀ + η₀₂
φ₂ = (η₂₀ - η₀₂)² + 4η₁₁²
φ₃ = (η₃₀ - 3η₁₂)² + (3η₂₁ - η₀₃)²

Where ηpq are normalized central moments:
ηpq = μpq / μ₀₀^((p+q)/2+1)

Rectangularity Measure:
R = Area_contour / Area_bounding_box

Circularity Test:
C = 4π × Area / Perimeter²
```

## Performance Characteristics

### 📊 **Benchmarking Results**
```
Image Size    Processing Time    Throughput      Memory Usage
──────────    ───────────────    ──────────      ────────────
100×100       354 ms            28 px/ms        ~50 KB
200×200       348 ms            114 px/ms       ~200 KB  
400×400       335 ms            476 px/ms       ~800 KB
800×800       338 ms            1,887 px/ms     ~3.2 MB
1600×1600     406 ms            6,289 px/ms     ~12.8 MB

Complex Scene (397 rectangles): 402 ms average
Multi-threading speedup: 2.3x on 4-core system
```

### ⚡ **Optimization Strategies**
```
Memory Management:
├─ Pre-allocated vectors with reserve()
├─ Stack-based temporary objects
├─ Minimal heap allocations during detection
└─ Efficient STL container usage

Algorithm Optimizations:
├─ Early termination conditions
├─ Spatial indexing for duplicate removal
├─ Vectorized operations using SIMD
├─ Cache-friendly memory access patterns
└─ Parallel processing with OpenMP

Data Structure Optimizations:
├─ Packed data structures
├─ Efficient point representation
├─ Optimized contour storage
└─ Fast lookup tables for trigonometry
```

## Build and Compilation

The source files are compiled using CMake with optimized build flags:

```cmake
# Optimization flags for Release build
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -mtune=native -flto -ffast-math")

# OpenMP support
find_package(OpenMP)
target_link_libraries(${PROJECT_NAME} OpenMP::OpenMP_CXX)
```

**Compiler Support:**
- GCC 10+ (recommended)
- Clang 12+
- MSVC 2019+ (Visual Studio 2019)

**Platform Support:**
- Linux (primary development platform)
- Windows (WSL and native)
- macOS (limited testing)