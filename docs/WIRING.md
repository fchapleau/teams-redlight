# Wiring Guide

## Basic Wiring Diagram

```
ESP32 Pin Layout (Top View)
                        
    ┌─────────────────────────┐
    │ EN                  D23 │
    │ VP                  D22 │
    │ VN                  TX0 │
    │ D34                 RX0 │
    │ D35                 D21 │
    │ D32                 D19 │
    │ D33                 D18 │
    │ D25                 D5  │
    │ D26                 TX2 │
    │ D27                 RX2 │
    │ D14                 D4  │
    │ D12                 D2  │◄─── Connect LED here
    │ D13                 D15 │
    │ GND                 GND │◄─── Connect LED cathode here
    │ VIN                 3V3 │
    └─────────────────────────┘
                     USB
```

## LED Connection

### Standard LED (3mm/5mm)

**Components needed:**
- 1x Red LED
- 1x 220Ω resistor (red-red-brown-gold)

**Connection:**
```
GPIO 2 ──[220Ω]──┤>├── GND
              Red LED
```

**Step by step:**
1. Insert LED into breadboard
2. Connect longer leg (anode) to one end of 220Ω resistor
3. Connect other end of resistor to GPIO 2 on ESP32
4. Connect shorter leg (cathode) to GND on ESP32

### High-Power LED

For brighter indication, you can use a high-power LED with a transistor:

**Components needed:**
- 1x High-power red LED (1W or 3W)
- 1x NPN transistor (2N2222 or similar)
- 1x 1kΩ resistor (base resistor)
- 1x Current-limiting resistor for LED (calculate based on LED specs)

**Connection:**
```
GPIO 2 ──[1kΩ]─── Base
                     │
                  ┌──▼──┐
                  │ NPN │
    LED_VCC ──────│  C  │
                  │  E  │───── GND
                  └─────┘
                     │
    LED+ ──[R_LED]───┘
    LED- ──────────────── GND
```

## Power Options

### USB Power (Recommended for development)
- Connect ESP32 to computer or USB power adapter
- Provides 5V input, regulated to 3.3V internally
- Easy for programming and debugging

### External 3.3V Power
- Connect regulated 3.3V to 3V3 pin
- Connect ground to GND pin
- More efficient than 5V input

### Battery Power
- Use 3.7V LiPo battery with voltage regulator
- Connect battery through power switch for easy on/off
- Consider deep sleep mode for battery conservation

## Enclosure Suggestions

### 3D Printed Case
- Design files available in `/docs/3d-models/`
- Provides protection and professional appearance
- Include cutouts for LED, USB port, and reset button

### Simple Project Box
- Use plastic project box from electronics store
- Drill holes for LED and USB cable
- Add labels for professional appearance

### Desk Mount
- Mount ESP32 under desk or monitor
- Position LED in visible location
- Use long wires to separate ESP32 from LED

## Safety Considerations

⚠️ **Important Safety Notes:**

1. **Check polarity** - LEDs will not work if connected backwards
2. **Use appropriate resistors** - Too low resistance can damage LED or ESP32
3. **Avoid shorts** - Double-check connections before powering on
4. **Power limits** - ESP32 GPIO pins can only source 40mA maximum
5. **Static protection** - Handle ESP32 with anti-static precautions

## Testing Your Wiring

1. **Power on ESP32** - Should see power LED on board
2. **Check LED blinks** - Should blink very fast in configuration mode
3. **No smoke or heat** - If you smell smoke or feel heat, disconnect immediately
4. **Multimeter check** - Verify voltage at LED is around 2-3V when on

## Common Wiring Mistakes

❌ **LED backwards** - LED won't light up
✅ **Solution:** Swap LED leads

❌ **No resistor** - LED may burn out or ESP32 may be damaged
✅ **Solution:** Always use current-limiting resistor

❌ **Wrong GPIO pin** - LED won't respond to Teams status
✅ **Solution:** Use GPIO 2 as specified in firmware

❌ **Loose connections** - Intermittent operation
✅ **Solution:** Ensure all connections are secure

## Advanced Configurations

### Multiple LEDs
Connect additional LEDs to other GPIO pins and modify firmware:
- Green LED for available status
- Yellow LED for away status
- Blue LED for offline status

### RGB LED
Use RGB LED for different colors based on status:
- Red: In meeting/busy
- Green: Available
- Yellow: Away
- Blue: Offline

### LED Strip
Control WS2812B LED strip for more dramatic effect:
- Multiple LEDs light up during meetings
- Color patterns for different statuses
- Breathing effect for transitions