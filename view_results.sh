#!/bin/bash

echo "Rectangle Detection - Image Viewer Script"
echo "=========================================="

# Check if ImageMagick is available
if ! command -v convert &> /dev/null; then
    echo "Error: ImageMagick not found. Please install it with:"
    echo "sudo apt update && sudo apt install imagemagick"
    exit 1
fi

# Build and run the application first
echo "Building and running rectangle detection..."
cd build 2>/dev/null || { echo "Error: build directory not found. Run cmake first."; exit 1; }

if ! make > /dev/null 2>&1; then
    echo "Error: Build failed"
    exit 1
fi

echo "Running rectangle detection application..."
./CppRectangleRecognition

# Check if output file exists
if [[ ! -f "output.ppm" ]]; then
    echo "Error: PPM output file not found"
    exit 1
fi

echo ""
echo "Converting PPM file to PNG for better viewing..."

# Convert PPM to PNG for better compatibility
convert output.ppm output.png

echo "Files created:"
echo "  - output.ppm  (color image with original in grey and detected rectangles in red)"
echo "  - output.png  (converted for viewing)"

echo ""
echo "Image Information:"
echo "=================="
echo "Output image:"
identify output.ppm

echo ""
echo "Display Options:"
echo "================"

# Check if we're in WSL
if grep -qi microsoft /proc/version 2>/dev/null; then
    echo "WSL detected. Choose viewing option:"
    echo ""
    echo "1. Open PNG file with Windows default viewer:"
    echo "   explorer.exe output.png"
    echo ""
    echo "2. If you have X11 forwarding set up:"
    echo "   display output.ppm &"
    echo ""
    echo "3. View as ASCII art (rough approximation):"
    
    # Create simple ASCII representation
    echo ""
    echo "Output image (ASCII approximation):"
    convert output.ppm -resize 60x40 txt:- | grep -v '^#' | awk '{if($3=="black") printf "#"; else printf " "; if(NR%60==0) printf "\n"}'
    
    echo ""
    echo "Opening PNG file with Windows explorer..."
    explorer.exe output.png 2>/dev/null &
    
else
    echo "Linux system detected."
    echo "1. Use display command (ImageMagick):"
    echo "   display output.ppm &"
    echo ""
    echo "2. Use other image viewers:"
    echo "   eog output.png &    # GNOME"
    echo "   feh output.png &    # Lightweight"
    echo "   gimp output.png &   # Advanced editing"
    
    # Try to open with available viewers
    if command -v display &> /dev/null; then
        echo ""
        echo "Opening with ImageMagick display..."
        display output.ppm &
    elif command -v eog &> /dev/null; then
        echo ""
        echo "Opening with Eye of GNOME..."
        eog output.png &
    elif command -v feh &> /dev/null; then
        echo ""
        echo "Opening with feh..."
        feh output.png &
    fi
fi

echo ""
echo "Script completed successfully!"