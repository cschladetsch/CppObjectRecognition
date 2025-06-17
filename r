#!/bin/bash

./b || exit 1

# Show both input and output files if they exist
echo ""
echo "Generated files:"
if [[ -f "test_input.pbm" ]]; then
    echo "- test_input.pbm ($(stat -f%z test_input.pbm 2>/dev/null || stat -c%s test_input.pbm) bytes)"
    if command -v identify &> /dev/null; then
        identify test_input.pbm
    fi
fi

if [[ -f "test_output.pbm" ]]; then
    echo "- test_output.pbm ($(stat -f%z test_output.pbm 2>/dev/null || stat -c%s test_output.pbm) bytes)"
    if command -v identify &> /dev/null; then
        identify test_output.pbm
    fi
fi

# Convert to PNG and show with Windows explorer if in WSL
if grep -qi microsoft /proc/version 2>/dev/null; then
    if [[ -f "test_input.pbm" ]] && [[ -f "test_output.pbm" ]]; then
        echo ""
        echo "Converting to PNG and opening with Windows explorer..."
        convert test_input.pbm test_input.png 2>/dev/null
        convert test_output.pbm test_output.png 2>/dev/null
        
        if [[ -f "test_input.png" ]] && [[ -f "test_output.png" ]]; then
            explorer.exe test_input.png 2>/dev/null &
            sleep 1
            explorer.exe test_output.png 2>/dev/null &
            echo "- Opened both images in Windows viewer"
        fi
    fi
fi
