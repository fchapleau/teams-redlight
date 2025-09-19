#!/bin/bash

# Test to verify the ESP32 boot failure fix implementation

echo "Testing ESP32 boot failure fix..."

# Test 1: Check that manifest.json includes all required ESP32 components
echo "Test 1: Checking ESP Web Tools manifest includes complete firmware components..."
if grep -q 'bootloader_dio_40m.bin' web/manifest.json; then
    echo "✅ Manifest includes bootloader component"
else
    echo "❌ Manifest missing bootloader component"
    exit 1
fi

if grep -q 'partitions.bin' web/manifest.json; then
    echo "✅ Manifest includes partition table component"
else
    echo "❌ Manifest missing partition table component"
    exit 1
fi

if grep -q 'boot_app0.bin' web/manifest.json; then
    echo "✅ Manifest includes boot_app0 component"
else
    echo "❌ Manifest missing boot_app0 component"
    exit 1
fi

if grep -q 'teams-redlight-firmware.bin' web/manifest.json; then
    echo "✅ Manifest includes application firmware"
else
    echo "❌ Manifest missing application firmware"
    exit 1
fi

# Test 2: Check that offsets are correct for ESP32 memory layout
echo "Test 2: Checking ESP32 memory layout offsets..."
if grep -q '"offset": 4096' web/manifest.json; then
    echo "✅ Bootloader at correct offset (0x1000)"
else
    echo "❌ Bootloader offset incorrect"
    exit 1
fi

if grep -q '"offset": 32768' web/manifest.json; then
    echo "✅ Partition table at correct offset (0x8000)"
else
    echo "❌ Partition table offset incorrect"
    exit 1
fi

if grep -q '"offset": 57344' web/manifest.json; then
    echo "✅ Boot app0 at correct offset (0xe000)"
else
    echo "❌ Boot app0 offset incorrect"
    exit 1
fi

if grep -q '"offset": 65536' web/manifest.json; then
    echo "✅ Application at correct offset (0x10000)"
else
    echo "❌ Application offset incorrect"
    exit 1
fi

# Test 3: Check that firmware extraction script exists
echo "Test 3: Checking firmware extraction script..."
if [ -f "scripts/merge_firmware.py" ]; then
    echo "✅ Firmware extraction script exists"
else
    echo "❌ Firmware extraction script missing"
    exit 1
fi

# Test 4: Check that PlatformIO configuration includes web build
echo "Test 4: Checking PlatformIO web build configuration..."
if grep -q "esp32dev_web" platformio.ini; then
    echo "✅ PlatformIO web build environment configured"
else
    echo "❌ PlatformIO web build environment missing"
    exit 1
fi

if grep -q "post:scripts/merge_firmware.py" platformio.ini; then
    echo "✅ PlatformIO post-build script configured"
else
    echo "❌ PlatformIO post-build script not configured"
    exit 1
fi

# Test 5: Check that CI/CD workflow includes web build
echo "Test 5: Checking CI/CD workflow includes web build..."
if grep -q "pio run -e esp32dev_web" .github/workflows/build.yml; then
    echo "✅ CI/CD includes web build"
else
    echo "❌ CI/CD missing web build"
    exit 1
fi

# Test 6: Check that CI/CD downloads all firmware components
echo "Test 6: Checking CI/CD downloads all firmware components..."
if grep -q "bootloader_dio_40m" .github/workflows/build.yml; then
    echo "✅ CI/CD downloads bootloader"
else
    echo "❌ CI/CD doesn't download bootloader"
    exit 1
fi

if grep -q "partitions\.bin" .github/workflows/build.yml; then
    echo "✅ CI/CD downloads partition table"
else
    echo "❌ CI/CD doesn't download partition table"
    exit 1
fi

if grep -q "boot_app0\.bin" .github/workflows/build.yml; then
    echo "✅ CI/CD downloads boot_app0"
else
    echo "❌ CI/CD doesn't download boot_app0"
    exit 1
fi

# Test 7: Check that web interface includes boot failure guidance
echo "Test 7: Checking web interface includes boot failure guidance..."
if grep -q "invalid header.*0xffffffff" web/index.html; then
    echo "✅ Web interface mentions specific boot failure symptoms"
else
    echo "❌ Web interface missing boot failure guidance"
    exit 1
fi

if grep -q "incomplete firmware flashing" web/index.html; then
    echo "✅ Web interface explains incomplete firmware issue"
else
    echo "❌ Web interface missing incomplete firmware explanation"
    exit 1
fi

# Test 8: Check firmware version has been updated
echo "Test 8: Checking firmware version updated..."
if grep -q '"version": "1.0.8"' web/manifest.json; then
    echo "✅ Firmware version updated to 1.0.8"
else
    echo "❌ Firmware version not updated"
    exit 1
fi

echo -e "\n🎉 All ESP32 boot failure fix tests passed!"
echo "The fix should resolve 'invalid header: 0xffffffff' boot failures by:"
echo "- Including complete ESP32 firmware components (bootloader + partition table + app)"
echo "- Using correct memory layout offsets"
echo "- Providing user guidance for boot issues"
echo "- Updating the build system to extract all components"