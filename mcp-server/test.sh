#!/bin/bash

# Test AGUADA MCP Server

echo "==================================="
echo "  AGUADA MCP Server - Test Suite"
echo "==================================="
echo ""

cd "$(dirname "$0")/.."

echo "1. Testing compilation..."
npm run build
if [ $? -eq 0 ]; then
    echo "   ✓ Build successful"
else
    echo "   ✗ Build failed"
    exit 1
fi

echo ""
echo "2. Testing server startup..."
timeout 2 npm start > /dev/null 2>&1
if [ $? -eq 124 ]; then
    echo "   ✓ Server started (timeout as expected)"
else
    echo "   ✗ Server failed to start"
    exit 1
fi

echo ""
echo "3. Verifying files..."
FILES=(
    "dist/index.js"
    "package.json"
    "tsconfig.json"
    "README.md"
)

for file in "${FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "   ✓ $file exists"
    else
        echo "   ✗ $file missing"
        exit 1
    fi
done

echo ""
echo "==================================="
echo "  All tests passed! ✓"
echo "==================================="
echo ""
echo "Next steps:"
echo "1. Reload VS Code window (Ctrl+Shift+P > Reload Window)"
echo "2. MCP server will be available as 'aguada'"
echo "3. Use tools: get_telemetry, get_reservoir_status, etc."
echo ""
