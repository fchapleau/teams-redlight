#!/bin/bash

# Test to verify the specific CORS API fix implementation

echo "Testing CORS API fix implementation..."

# Test 1: Check that web interface uses browser_download_url instead of api url
echo "Test 1: Checking browser_download_url usage..."
if grep -q "firmwareAsset.browser_download_url" web/index.html; then
    echo "✅ Uses browser_download_url for direct download"
else
    echo "❌ browser_download_url not found"
    exit 1
fi

# Test 2: Check that API URL with headers is not used
echo "Test 2: Checking that API URL with headers is not used..."
if grep -q "firmwareAsset.url" web/index.html; then
    echo "❌ Still using problematic API URL"
    exit 1
else
    echo "✅ API URL usage removed"
fi

# Test 3: Check that Accept header is not used (not needed for direct download)
echo "Test 3: Checking that Accept header is removed..."
if grep -q "Accept.*octet-stream" web/index.html; then
    echo "❌ Still using Accept header for direct download"
    exit 1
else
    echo "✅ Accept header properly removed"
fi

# Test 4: Check updated log messages
echo "Test 4: Checking updated log messages..."
if grep -q "from GitHub releases" web/index.html && ! grep -q "from GitHub API" web/index.html; then
    echo "✅ Log messages updated correctly"
else
    echo "❌ Log messages not updated correctly"
    exit 1
fi

echo -e "\n🎉 All CORS API fix tests passed!"
echo "The firmware download should now work without CORS errors."