#!/bin/bash

# Simple validation tests for the Teams Red Light project

echo "Running Teams Red Light validation tests..."

# Test 1: Check required files exist
echo "Test 1: Checking required files..."
required_files=(
    "src/main.cpp"
    "include/config.h"
    "platformio.ini"
    "README.md"
    "web/index.html"
)

for file in "${required_files[@]}"; do
    if [[ -f "$file" ]]; then
        echo "✅ $file exists"
    else
        echo "❌ $file missing"
        exit 1
    fi
done

# Test 2: Check configuration constants
echo -e "\nTest 2: Checking configuration constants..."
if grep -q "LED_PIN" include/config.h; then
    echo "✅ LED_PIN defined"
else
    echo "❌ LED_PIN not found"
    exit 1
fi

if grep -q "AP_SSID" include/config.h; then
    echo "✅ AP_SSID defined"
else
    echo "❌ AP_SSID not found"
    exit 1
fi

# Test 3: Check main includes
echo -e "\nTest 3: Checking main.cpp includes..."
if grep -q "#include.*WiFi.h" src/main.cpp; then
    echo "✅ WiFi.h included"
else
    echo "❌ WiFi.h not included"
    exit 1
fi

if grep -q "#include.*ArduinoJson.h" src/main.cpp; then
    echo "✅ ArduinoJson.h included"
else
    echo "❌ ArduinoJson.h not included"
    exit 1
fi

# Test 4: Check web interface
echo -e "\nTest 4: Checking web interface..."
if grep -q "Teams Red Light" web/index.html; then
    echo "✅ Web interface has correct title"
else
    echo "❌ Web interface title incorrect"
    exit 1
fi

if grep -q "Web Serial\|WebSerial\|serial" web/index.html; then
    echo "✅ WebSerial support mentioned"
else
    echo "❌ WebSerial not found"
    exit 1
fi

# Test 5: Check documentation
echo -e "\nTest 5: Checking documentation..."
if grep -q "GPIO 2" docs/WIRING.md; then
    echo "✅ Wiring documentation mentions GPIO 2"
else
    echo "❌ GPIO 2 not mentioned in wiring docs"
    exit 1
fi

if grep -q "Azure AD" docs/AZURE_SETUP.md; then
    echo "✅ Azure AD setup documentation exists"
else
    echo "❌ Azure AD documentation missing"
    exit 1
fi

# Test 6: Check build configuration
echo -e "\nTest 6: Checking build configuration..."
if grep -q "esp32dev" platformio.ini; then
    echo "✅ ESP32 target configured"
else
    echo "❌ ESP32 target not found"
    exit 1
fi

if grep -q "platform.*espressif32" platformio.ini; then
    echo "✅ ESP32 platform configured"
else
    echo "❌ ESP32 platform not found"
    exit 1
fi

echo -e "\n🎉 All validation tests passed!"
echo "The Teams Red Light project structure is complete and ready for use."