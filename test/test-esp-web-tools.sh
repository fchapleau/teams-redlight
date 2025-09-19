#!/bin/bash

# Test to verify ESP Web Tools compatibility and CI/CD integration

echo "Testing ESP Web Tools compatibility..."

# Test 1: Check that manifest.json exists in web directory
echo "Test 1: Checking manifest.json exists..."
if [ -f "web/manifest.json" ]; then
    echo "✅ web/manifest.json exists"
else
    echo "❌ web/manifest.json missing"
    exit 1
fi

# Test 2: Check manifest.json format and content
echo "Test 2: Checking manifest.json format..."
if grep -q '"name":.*"Teams Red Light"' web/manifest.json; then
    echo "✅ Manifest has correct name"
else
    echo "❌ Manifest name incorrect"
    exit 1
fi

if grep -q '"chipFamily":.*"ESP32"' web/manifest.json; then
    echo "✅ Manifest specifies ESP32 chip family"
else
    echo "❌ Manifest missing ESP32 chip family"
    exit 1
fi

if grep -q '"path":.*"firmware/teams-redlight-firmware.bin"' web/manifest.json; then
    echo "✅ Manifest references correct firmware path"
else
    echo "❌ Manifest firmware path incorrect"
    exit 1
fi

# Test 3: Check that web interface references manifest correctly
echo "Test 3: Checking web interface ESP Web Tools integration..."
if grep -q 'manifest="./manifest.json"' web/index.html; then
    echo "✅ Web interface references manifest correctly"
else
    echo "❌ Web interface manifest reference missing or incorrect"
    exit 1
fi

if grep -q 'esp-web-install-button' web/index.html; then
    echo "✅ ESP Web Tools button component present"
else
    echo "❌ ESP Web Tools button component missing"
    exit 1
fi

# Test 4: Check CI/CD copies manifest to deployment
echo "Test 4: Checking CI/CD copies manifest.json for deployment..."
if grep -q "cp web/manifest.json _site/manifest.json" .github/workflows/build.yml; then
    echo "✅ CI/CD copies manifest.json to deployment"
else
    echo "❌ CI/CD does not copy manifest.json"
    exit 1
fi

# Test 5: Check CI/CD validates ESP Web Tools setup
echo "Test 5: Checking CI/CD validates ESP Web Tools configuration..."
if grep -q "Validating ESP Web Tools manifest configuration" .github/workflows/build.yml; then
    echo "✅ CI/CD validates ESP Web Tools configuration"
else
    echo "❌ CI/CD does not validate ESP Web Tools configuration"
    exit 1
fi

# Test 6: Check firmware is copied to the path expected by manifest
echo "Test 6: Checking CI/CD copies firmware to expected path..."
if grep -q "cp firmware/teams-redlight-firmware.bin _site/firmware/" .github/workflows/build.yml; then
    echo "✅ CI/CD copies firmware to path expected by manifest"
else
    echo "❌ CI/CD firmware copy path mismatch"
    exit 1
fi

echo -e "\n🎉 All ESP Web Tools tests passed!"
echo "ESP Web Tools should work correctly with the deployed web interface."
echo "Users can flash firmware directly from https://fchapleau.github.io/teams-redlight/"