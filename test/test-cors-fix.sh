#!/bin/bash

# Test to verify CORS fix implementation in web interface

echo "Testing CORS fix implementation..."

# Test 1: Check that web interface includes GitHub Pages firmware fetching
echo "Test 1: Checking GitHub Pages firmware fetching logic..."
if grep -q "fetch('./firmware/teams-redlight-firmware.bin')" web/index.html; then
    echo "✅ GitHub Pages firmware fetching implemented"
else
    echo "❌ GitHub Pages firmware fetching not found"
    exit 1
fi

# Test 2: Check that CI/CD downloads release assets for GitHub Pages
echo "Test 2: Checking CI/CD downloads release assets..."
if grep -q "Download latest release assets" .github/workflows/build.yml; then
    echo "✅ CI/CD downloads release assets for GitHub Pages"
else
    echo "❌ CI/CD release asset download not found"
    exit 1
fi

# Test 3: Check that workflow copies firmware to Pages deployment
echo "Test 3: Checking workflow firmware copy logic..."
if grep -q "Copy firmware binaries to serve from same origin" .github/workflows/build.yml; then
    echo "✅ Workflow copies firmware to avoid CORS"
else
    echo "❌ Workflow firmware copy not found"
    exit 1
fi

# Test 4: Check that workflow depends on build-firmware job
echo "Test 4: Checking workflow dependencies..."
if grep -q "needs: \[validate, build-firmware\]" .github/workflows/build.yml; then
    echo "✅ Web deployment depends on firmware build"
else
    echo "❌ Web deployment dependency not found"
    exit 1
fi

echo -e "\n🎉 All CORS fix tests passed!"
echo "The firmware download CORS issue should be resolved."