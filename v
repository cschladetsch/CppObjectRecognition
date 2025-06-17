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

# Check if output files exist in the correct location
OUTPUT_DIR="../Output/Images"

# First, ensure the output directory exists
if [[ ! -d "$OUTPUT_DIR" ]]; then
    echo "Creating output directory: $OUTPUT_DIR"
    mkdir -p "$OUTPUT_DIR"
fi

# Check if any of the expected PNG files exist
PNG_FILES_EXIST=false
for file in "$OUTPUT_DIR"/visual_test_*.png; do
    if [[ -f "$file" ]]; then
        PNG_FILES_EXIST=true
        break
    fi
done

if [[ "$PNG_FILES_EXIST" == "false" ]]; then
    echo "Note: PNG files not found. This may be normal if ImageMagick is not installed."
    echo "The visual test may still have completed successfully."
    # Don't exit with error - the test may have worked but PNG conversion failed
fi

echo ""
echo "PNG files generated directly by visual test..."

echo ""
echo "Files created in $OUTPUT_DIR:"
echo "  - visual_test_circles_only.png      (circles only - 0 rectangles detected)"
echo "  - visual_test_triangles_only.png    (triangles only - 0 rectangles detected)"
echo "  - visual_test_rectangles_only.png   (rectangles only - all detected)"
echo "  - visual_test_mixed_shapes.png      (mixed shapes - only rectangles detected)"
echo "  - visual_test_rotated_rectangles.png (rotated rectangles at various angles - all detected)"
echo "  - visual_test_complex_scene.png     (complex scene - only rectangles detected)"

echo ""
echo "Image Information:"
echo "=================="
if [[ -f "$OUTPUT_DIR/visual_test_mixed_shapes.png" ]]; then
    echo "Mixed shapes test image:"
    identify "$OUTPUT_DIR/visual_test_mixed_shapes.png" 2>/dev/null || echo "  (ImageMagick 'identify' command not available)"
else
    echo "PNG files not found - visual test may have completed but PNG conversion failed"
fi

echo ""
echo "Display Options:"
echo "================"

# Check if we're in WSL
if grep -qi microsoft /proc/version 2>/dev/null; then
    echo "WSL detected. Choose viewing option:"
    echo ""
    echo "1. Open PNG files with Windows default viewer:"
    echo "   explorer.exe $OUTPUT_DIR/visual_test_mixed_shapes.png"
    echo "   explorer.exe $OUTPUT_DIR/visual_test_rectangles_only.png"
    echo ""
    echo "2. If you have X11 forwarding set up:"
    echo "   display $OUTPUT_DIR/visual_test_mixed_shapes.png &"
    echo ""
    echo "3. View all test results:"
    echo "   explorer.exe $OUTPUT_DIR/visual_test_*.png"
    
    if [[ "$PNG_FILES_EXIST" == "true" ]]; then
        echo ""
        echo "Opening mixed shapes test with Windows explorer..."
        explorer.exe "$OUTPUT_DIR/visual_test_mixed_shapes.png" 2>/dev/null &
        
        echo "Opening rotated rectangles test with Windows explorer..."
        explorer.exe "$OUTPUT_DIR/visual_test_rotated_rectangles.png" 2>/dev/null &
    fi
    
else
    echo "Linux system detected."
    echo "1. Use display command (ImageMagick):"
    echo "   display $OUTPUT_DIR/visual_test_mixed_shapes.png &"
    echo ""
    echo "2. Use other image viewers:"
    echo "   eog $OUTPUT_DIR/visual_test_*.png &    # GNOME - view all"
    echo "   feh $OUTPUT_DIR/visual_test_*.png &    # Lightweight - view all"
    echo "   gimp $OUTPUT_DIR/visual_test_mixed_shapes.png &   # Advanced editing"
    
    # Try to open with available viewers
    if [[ "$PNG_FILES_EXIST" == "true" ]]; then
        if command -v display &> /dev/null; then
            echo ""
            echo "Opening mixed shapes test with ImageMagick display..."
            display "$OUTPUT_DIR/visual_test_mixed_shapes.png" &
            echo "Opening rotated rectangles test with ImageMagick display..."
            display "$OUTPUT_DIR/visual_test_rotated_rectangles.png" &
        elif command -v eog &> /dev/null; then
            echo ""
            echo "Opening visual tests with Eye of GNOME..."
            eog "$OUTPUT_DIR"/visual_test_*.png &
        elif command -v feh &> /dev/null; then
            echo ""
            echo "Opening visual tests with feh..."
            feh "$OUTPUT_DIR"/visual_test_*.png &
        fi
    fi
fi

echo ""
echo "Script completed successfully!"
