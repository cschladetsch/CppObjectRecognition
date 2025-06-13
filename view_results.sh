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

# Check if output files exist
if [[ ! -f "test_input.pbm" ]] || [[ ! -f "test_output.pbm" ]]; then
    echo "Error: PBM files not found"
    exit 1
fi

echo ""
echo "Converting PBM files to PNG for better viewing..."

# Convert PBM to PNG for better compatibility
convert test_input.pbm test_input.png
convert test_output.pbm test_output.png

echo "Files created:"
echo "  - test_input.pbm  (original PBM binary)"
echo "  - test_input.png  (converted for viewing)"
echo "  - test_output.pbm (result PBM binary)" 
echo "  - test_output.png (converted for viewing)"

echo ""
echo "Image Information:"
echo "=================="
echo "Input image:"
identify test_input.pbm
echo ""
echo "Output image:"
identify test_output.pbm

echo ""
echo "Display Options:"
echo "================"

# Check if we're in WSL
if grep -qi microsoft /proc/version 2>/dev/null; then
    echo "WSL detected. Choose viewing option:"
    echo ""
    echo "1. Open PNG files with Windows default viewer:"
    echo "   explorer.exe test_input.png"
    echo "   explorer.exe test_output.png"
    echo ""
    echo "2. If you have X11 forwarding set up:"
    echo "   display test_input.pbm &"
    echo "   display test_output.pbm &"
    echo ""
    echo "3. View as ASCII art (rough approximation):"
    
    # Create simple ASCII representation
    echo ""
    echo "Input image (ASCII approximation):"
    convert test_input.pbm -resize 60x40 txt:- | grep -v '^#' | awk '{if($3=="black") printf "#"; else printf " "; if(NR%60==0) printf "\n"}'
    
    echo ""
    echo "Output image (ASCII approximation):"
    convert test_output.pbm -resize 60x40 txt:- | grep -v '^#' | awk '{if($3=="black") printf "#"; else printf " "; if(NR%60==0) printf "\n"}'
    
    echo ""
    echo "Opening PNG files with Windows explorer..."
    explorer.exe test_input.png 2>/dev/null &
    sleep 1
    explorer.exe test_output.png 2>/dev/null &
    
else
    echo "Linux system detected."
    echo "1. Use display command (ImageMagick):"
    echo "   display test_input.pbm &"
    echo "   display test_output.pbm &"
    echo ""
    echo "2. Use other image viewers:"
    echo "   eog test_input.png &    # GNOME"
    echo "   feh test_input.png &    # Lightweight"
    echo "   gimp test_input.png &   # Advanced editing"
    
    # Try to open with available viewers
    if command -v display &> /dev/null; then
        echo ""
        echo "Opening with ImageMagick display..."
        display test_input.pbm &
        display test_output.pbm &
    elif command -v eog &> /dev/null; then
        echo ""
        echo "Opening with Eye of GNOME..."
        eog test_input.png &
        eog test_output.png &
    elif command -v feh &> /dev/null; then
        echo ""
        echo "Opening with feh..."
        feh test_input.png test_output.png &
    fi
fi

echo ""
echo "Script completed successfully!"