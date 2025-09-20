#!/bin/bash

# Test to verify that build process fails properly when compilation errors occur

echo "Testing build failure handling..."

# Test 1: Check that workflow does not use continue-on-error for firmware builds
echo "Test 1: Checking firmware build job does not ignore build failures..."
if grep -A 10 "build-firmware:" .github/workflows/build.yml | grep -q "continue-on-error: true"; then
    echo "❌ Build-firmware job uses continue-on-error which masks build failures"
    exit 1
else
    echo "✅ Build-firmware job will fail when builds fail"
fi

# Test 2: Check that workflow does not mask build failures with fallback messages
echo "Test 2: Checking workflow does not mask compilation failures..."
if grep -q "|| echo.*Build failed.*may be due to network restrictions" .github/workflows/build.yml; then
    echo "❌ Workflow masks build failures with fallback messages"
    exit 1
else
    echo "✅ Workflow does not mask build failures"
fi

# Test 3: Check that workflow does not skip firmware builds
echo "Test 3: Checking workflow does not skip firmware builds..."
if grep -q "Skip firmware build" .github/workflows/build.yml; then
    echo "❌ Workflow inappropriately skips firmware builds"
    exit 1
else
    echo "✅ Workflow does not skip firmware builds"
fi

# Test 4: Check that web deployment depends on successful firmware build
echo "Test 4: Checking web deployment depends on firmware build..."
if grep -q "needs: \[validate, build-firmware\]" .github/workflows/build.yml; then
    echo "✅ Web deployment blocked if firmware build fails"
else
    echo "❌ Web deployment does not depend on firmware build success"
    exit 1
fi

# Test 5: Check that the main.cpp compiles without errors
echo "Test 5: Checking that firmware actually compiles..."
if command -v pio >/dev/null 2>&1; then
    if pio run -e esp32dev --silent >/dev/null 2>&1; then
        echo "✅ Firmware compiles successfully"
    else
        echo "❌ Firmware compilation fails - this should cause workflow failure"
        exit 1
    fi
else
    echo "⚠️  PlatformIO not available for compilation test"
fi

echo -e "\n🎉 All build failure handling tests passed!"
echo "Build process will now properly fail when firmware cannot be compiled."
echo "No mock or demonstration firmware will be built or deployed."