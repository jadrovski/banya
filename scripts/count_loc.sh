#!/bin/bash
# Count C++ Lines of Code in src/ directory

echo "=== C++ Lines of Code ==="
echo ""

# Find all .cpp and .h files and count lines
find src -name "*.cpp" -o -name "*.h" | sort | xargs wc -l

echo ""
echo "=== Summary ==="

# Count .cpp files
cpp_count=$(find src -name "*.cpp" | wc -l)
cpp_lines=$(find src -name "*.cpp" -exec cat {} + | wc -l)

# Count .h files
h_count=$(find src -name "*.h" | wc -l)
h_lines=$(find src -name "*.h" -exec cat {} + | wc -l)

# Total
total_files=$((cpp_count + h_count))
total_lines=$((cpp_lines + h_lines))

echo "C++ files (.cpp):  $cpp_count files, $cpp_lines lines"
echo "Header files (.h): $h_count files, $h_lines lines"
echo "Total:             $total_files files, $total_lines lines"
