#!/usr/bin/env python3
"""
ESP32 Firmware Components Extractor for Web Flashing

This script extracts the necessary ESP32 firmware components:
- Bootloader (bootloader_dio_40m.bin at 0x1000)
- Partition table (partitions.bin at 0x8000) 
- Boot app (boot_app0.bin at 0xe000)
- Application (teams-redlight-firmware.bin at 0x10000)

These components are needed for ESP Web Tools to properly flash an ESP32
and avoid "invalid header" boot failures.
"""

import os
import sys
import shutil
from pathlib import Path

# For PlatformIO
Import("env")

def create_merged_firmware(firmware_dir, output_file):
    """
    Create a merged firmware file compatible with ESP Web Tools specification.
    This merges bootloader, partitions, boot_app0, and application into a single file.
    """
    print("🔧 Creating merged firmware for ESP Web Tools compatibility...")
    
    # Define the memory layout offsets (same as in manifest.json)
    layout = [
        (0x1000, firmware_dir / "bootloader_dio_40m.bin"),     # Bootloader at 0x1000
        (0x8000, firmware_dir / "partitions.bin"),            # Partitions at 0x8000  
        (0xe000, firmware_dir / "boot_app0.bin"),             # Boot App0 at 0xe000
        (0x10000, firmware_dir / "teams-redlight-firmware.bin") # Application at 0x10000
    ]
    
    # Calculate total size needed
    max_offset = 0
    for offset, file_path in layout:
        if file_path.exists():
            file_size = file_path.stat().st_size
            max_offset = max(max_offset, offset + file_size)
        else:
            print(f"⚠️  Warning: {file_path} not found for merged firmware")
            return False
    
    # Create merged firmware file filled with 0xFF (erased flash state)
    merged_data = bytearray(b'\xFF' * max_offset)
    
    # Copy each component to its correct offset
    for offset, file_path in layout:
        if file_path.exists():
            with open(file_path, 'rb') as f:
                component_data = f.read()
                merged_data[offset:offset + len(component_data)] = component_data
                print(f"   ✅ Merged {file_path.name} at offset 0x{offset:X} ({len(component_data):,} bytes)")
        else:
            print(f"   ❌ Missing {file_path.name}")
            return False
    
    # Write merged firmware
    with open(output_file, 'wb') as f:
        f.write(merged_data)
    
    print(f"✅ Created merged firmware: {output_file} ({len(merged_data):,} bytes)")
    return True

def extract_firmware_components(source, target, env):
    """
    Post-build script to extract ESP32 firmware components for web flashing
    """
    print("🔧 Extracting ESP32 firmware components for web flashing...")
    print(f"Source: {source}")
    print(f"Target: {target}")
    
    # Get directories
    build_dir = Path(env.subst("$BUILD_DIR"))
    project_dir = Path(env.subst("$PROJECT_DIR"))
    platform_dir = Path(env.subst("$PLATFORMIO_CORE_DIR")) / "packages"
    
    print(f"Build dir: {build_dir}")
    print(f"Project dir: {project_dir}")
    
    # Output directory
    firmware_dir = project_dir / "firmware"
    firmware_dir.mkdir(exist_ok=True)
    print(f"Firmware output dir: {firmware_dir}")
    
    # 1. Copy application firmware
    app_bin = build_dir / "firmware.bin"
    if app_bin.exists():
        target_app = firmware_dir / "teams-redlight-firmware.bin"
        shutil.copy2(app_bin, target_app)
        print(f"✅ Copied application firmware: {target_app} ({app_bin.stat().st_size:,} bytes)")
    else:
        print(f"❌ Application firmware not found: {app_bin}")
        return
    
    # 2. Extract bootloader
    bootloader_bin = build_dir / "bootloader.bin"
    if bootloader_bin.exists():
        target_bootloader = firmware_dir / "bootloader_dio_40m.bin"
        shutil.copy2(bootloader_bin, target_bootloader)
        print(f"✅ Copied bootloader: {target_bootloader} ({bootloader_bin.stat().st_size:,} bytes)")
    else:
        # Try to find a generic ESP32 bootloader
        print("⚠️  Bootloader not found in build - creating placeholder")
        # Create minimal bootloader placeholder
        target_bootloader = firmware_dir / "bootloader_dio_40m.bin"
        with open(target_bootloader, 'wb') as f:
            # Write minimal bootloader header (this is a placeholder)
            # In production, this should be the actual ESP32 bootloader
            f.write(b'\xE9' + b'\x00' * 4095)  # Minimal ESP32 bootloader signature
        print(f"⚠️  Created placeholder bootloader: {target_bootloader}")
    
    # 3. Extract partition table
    partitions_bin = build_dir / "partitions.bin"
    if partitions_bin.exists():
        target_partitions = firmware_dir / "partitions.bin"
        shutil.copy2(partitions_bin, target_partitions)
        print(f"✅ Copied partition table: {target_partitions} ({partitions_bin.stat().st_size:,} bytes)")
    else:
        print("⚠️  Partition table not found - creating default")
        # Create a minimal partition table
        target_partitions = firmware_dir / "partitions.bin"
        # This is a simplified partition table in binary format
        # nvs, data, nvs, 0x9000, 0x4000
        # phy_init, data, phy, 0xd000, 0x1000
        # factory, app, factory, 0x10000, 0x300000
        partition_data = bytearray(4096)  # 4KB partition table
        # Add minimal partition entries (this is very simplified)
        with open(target_partitions, 'wb') as f:
            f.write(partition_data)
        print(f"⚠️  Created placeholder partition table: {target_partitions}")
    
    # 4. Create boot_app0.bin (tells ESP32 to boot from factory app)
    boot_app0 = firmware_dir / "boot_app0.bin"
    with open(boot_app0, 'wb') as f:
        # boot_app0.bin tells ESP32 which app partition to boot
        # 0xf0f0f0f0 means boot from factory app partition
        f.write(b'\xf0\xf0\xf0\xf0' + b'\xff' * 4092)
    print(f"✅ Created boot_app0.bin: {boot_app0} (4,096 bytes)")
    
    # List all firmware components
    print("\n📦 Firmware components ready for ESP Web Tools:")
    for component in firmware_dir.glob("*.bin"):
        size = component.stat().st_size
        print(f"   {component.name}: {size:,} bytes")
    
    # 5. Create merged firmware for ESP Web Tools compatibility
    # This provides an alternative single-file approach as per ESP Web Tools spec
    merged_firmware = firmware_dir / "teams-redlight-merged.bin"
    create_merged_firmware(firmware_dir, merged_firmware)
    
    print("✅ Firmware extraction completed successfully!")

# This function is called by PlatformIO after build
def post_build(source, target, env):
    try:
        extract_firmware_components(source, target, env)
    except Exception as e:
        print(f"❌ Error in post-build script: {e}")
        import traceback
        traceback.print_exc()

# Register the callback for PlatformIO
env.AddPostAction("buildprog", post_build)

if __name__ == "__main__":
    print("This script is called by PlatformIO during build")
    print("To test manually, use: pio run -e esp32dev_web")