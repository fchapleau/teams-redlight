# ESP32 Boot Failure Fix (Issue #43)

## Problem Description

After successfully flashing the Teams Red Light firmware, some users encountered boot failures with these symptoms:

```
rst:0x10 (RTCWDT_RTC_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
invalid header: 0xffffffff
invalid header: 0xffffffff
invalid header: 0xffffffff
invalid header: 0xffffffff
invalid header: 0xffffffff
invalid header: 0xffffffff
invalid header: 0xffffffff
invalid header: 0xffffffff
ets Jul 29 2019 12:21:46
```

## Root Cause

The issue was caused by incomplete firmware flashing. The ESP Web Tools manifest was only flashing the application firmware at offset 0x10000, but ESP32 devices require:

1. **Bootloader** at offset 0x1000
2. **Partition Table** at offset 0x8000  
3. **Boot App0** at offset 0xe000
4. **Application** at offset 0x10000

Without the bootloader and partition table, the ESP32 cannot boot and shows "invalid header: 0xffffffff" errors.

## Solution Implemented

### 1. Updated ESP Web Tools Manifest

The manifest now includes all required ESP32 firmware components:

```json
{
  "builds": [
    {
      "chipFamily": "ESP32",
      "parts": [
        { "path": "firmware/bootloader_dio_40m.bin", "offset": 4096 },
        { "path": "firmware/partitions.bin", "offset": 32768 },
        { "path": "firmware/boot_app0.bin", "offset": 57344 },
        { "path": "firmware/teams-redlight-firmware.bin", "offset": 65536 }
      ]
    }
  ]
}
```

### 2. Updated Build System

- Added `esp32dev_web` build environment in `platformio.ini`
- Created `scripts/merge_firmware.py` to extract all firmware components
- Updated CI/CD workflow to build and distribute complete firmware

### 3. Enhanced User Interface

- Added guidance about boot failures in the web flasher
- Provided specific troubleshooting steps for "invalid header" errors
- Updated firmware version to 1.0.6

## For Users Experiencing This Issue

### If you're currently experiencing boot failures:

1. **Flash again with the latest firmware** (v1.0.6+) from https://fchapleau.github.io/teams-redlight/
2. The new firmware includes all required ESP32 components
3. If still failing, try holding the BOOT button during the entire flash process
4. Ensure you're using a USB cable that supports data transfer (not just charging)

### Prevention

The latest firmware (v1.0.6+) automatically includes all required components, so this issue should not occur with new installations.

## Technical Details

### ESP32 Memory Layout

| Offset | Size | Component | Description |
|--------|------|-----------|-------------|
| 0x1000 | 28KB | Bootloader | Second stage bootloader |
| 0x8000 | 4KB  | Partition Table | Flash partition definitions |
| 0xe000 | 4KB  | Boot App0 | Boot application selector |
| 0x10000| ~3MB | Application | Main firmware |

### Files Changed

- `web/manifest.json` - Updated to include all firmware components
- `platformio.ini` - Added web build environment  
- `scripts/merge_firmware.py` - Extract firmware components
- `.github/workflows/build.yml` - Updated CI/CD for complete firmware
- `web/index.html` - Added boot failure guidance
- `test/test-esp32-boot-fix.sh` - New test for this fix

## Testing

Run the test to verify the fix:

```bash
./test/test-esp32-boot-fix.sh
```

This verifies:
- Manifest includes all firmware components
- Correct memory layout offsets
- Build system configuration
- CI/CD pipeline updates
- User interface guidance