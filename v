#!/bin/bash

echo "Rectangle Detection - Visual Test Script"
echo "========================================"

# Check if ImageMagick is available
if ! command -v convert &> /dev/null; then
    echo "Error: ImageMagick not found. Please install it with:"
    echo "sudo apt update && sudo apt install imagemagick"
    exit 1
fi

# Check if build directory exists
if [[ ! -d "build" ]]; then
    echo "Build directory not found. Running initial build..."
    ./b
fi

# Create Output directory if it doesn't exist
mkdir -p Output

cd build 2>/dev/null || { echo "Error: build directory not found. Run cmake first."; exit 1; }

# Check if VisualTest executable exists and is newer than source files
VISUAL_TEST_EXE="../Output/VisualTest"
NEEDS_BUILD=false

if [[ ! -f "$VISUAL_TEST_EXE" ]]; then
    echo "VisualTest executable not found. Building..."
    NEEDS_BUILD=true
else
    # Check if any source files are newer than the executable
    if [[ ../Source/VisualTest.cpp -nt "$VISUAL_TEST_EXE" ]] || \
       [[ ../Source/ImageProcessor.cpp -nt "$VISUAL_TEST_EXE" ]] || \
       [[ ../Source/RectangleDetector.cpp -nt "$VISUAL_TEST_EXE" ]] || \
       [[ ../Include/ShapeDetector/ImageProcessor.hpp -nt "$VISUAL_TEST_EXE" ]] || \
       [[ ../Include/ShapeDetector/RectangleDetector.hpp -nt "$VISUAL_TEST_EXE" ]] || \
       [[ ../CMakeLists.txt -nt "$VISUAL_TEST_EXE" ]]; then
        echo "Source files have been modified. Rebuilding..."
        NEEDS_BUILD=true
    fi
fi

# Build only if necessary
if [[ "$NEEDS_BUILD" == "true" ]]; then
    echo "Building VisualTest..."
    if ! make VisualTest > /dev/null 2>&1; then
        echo "Error: Build failed"
        exit 1
    fi
    echo "Build completed."
else
    echo "VisualTest is up to date. Skipping build."
fi

echo "Running visual test application..."
$VISUAL_TEST_EXE

# Check if output files exist
if [[ ! -f "visual_test_mixed_shapes.png" ]]; then
    echo "Error: Visual test output files not found"
    exit 1
fi

echo ""
echo "PNG files generated directly by visual test..."

echo ""
echo "Files created:"
echo "  - visual_test_circles_only.png      (circles only - 0 rectangles detected)"
echo "  - visual_test_triangles_only.png    (triangles only - 0 rectangles detected)"
echo "  - visual_test_rectangles_only.png   (rectangles only - all detected)"
echo "  - visual_test_mixed_shapes.png      (mixed shapes - only rectangles detected)"
echo "  - visual_test_rotated_rectangles.png (rotated rectangles at various angles - all detected)"
echo "  - visual_test_complex_scene.png     (complex scene - only rectangles detected)"

echo ""
echo "Image Information:"
echo "=================="
echo "Mixed shapes test image:"
identify visual_test_mixed_shapes.png

echo ""
echo "Display Options:"
echo "================"

# Check if we're in WSL
if grep -qi microsoft /proc/version 2>/dev/null; then
    echo "WSL detected. Choose viewing option:"
    echo ""
    echo "1. Open PNG files with Windows default viewer:"
    echo "   explorer.exe visual_test_mixed_shapes.png"
    echo "   explorer.exe visual_test_rectangles_only.png"
    echo ""
    echo "2. If you have X11 forwarding set up:"
    echo "   display visual_test_mixed_shapes.png &"
    echo ""
    echo "3. View all test results:"
    echo "   explorer.exe visual_test_*.png"
    
    echo ""
    echo "Opening mixed shapes test with Windows explorer..."
    explorer.exe visual_test_mixed_shapes.png 2>/dev/null &
    
    echo "Opening rotated rectangles test with Windows explorer..."
    explorer.exe visual_test_rotated_rectangles.png 2>/dev/null &
    
else
    echo "Linux system detected."
    echo "1. Use display command (ImageMagick):"
    echo "   display visual_test_mixed_shapes.png &"
    echo ""
    echo "2. Use other image viewers:"
    echo "   eog visual_test_*.png &    # GNOME - view all"
    echo "   feh visual_test_*.png &    # Lightweight - view all"
    echo "   gimp visual_test_mixed_shapes.png &   # Advanced editing"
    
    # Try to open with available viewers
    if command -v display &> /dev/null; then
        echo ""
        echo "Opening mixed shapes test with ImageMagick display..."
        display visual_test_mixed_shapes.png &
        echo "Opening rotated rectangles test with ImageMagick display..."
        display visual_test_rotated_rectangles.png &
    elif command -v eog &> /dev/null; then
        echo ""
        echo "Opening visual tests with Eye of GNOME..."
        eog visual_test_*.png &
    elif command -v feh &> /dev/null; then
        echo ""
        echo "Opening visual tests with feh..."
        feh visual_test_*.png &
    fi
fi

echo ""
echo "Script completed successfully!"
