# 🔴 Teams Red Light

An ESP32-based device that shows your Microsoft Teams presence status with a red LED. The LED lights up when you're in a meeting or busy, helping others know when not to disturb you.

## Features

- **Real-time Teams presence monitoring** using Microsoft Graph API
- **Web-based configuration** - no need for coding or complex setup
- **WiFi connectivity** with easy setup through captive portal
- **Persistent storage** - remembers settings after power cycles
- **OTA firmware updates** - update wirelessly
- **Web-based flashing** - flash ESP32 directly from your browser
- **Customizable LED patterns**:
  - **System patterns**: Very fast blink (AP mode), slow blink (WiFi), fast blink (API)
  - **Meeting patterns**: 7 selectable patterns for when in meetings or busy
  - **Available patterns**: 7 selectable patterns for when available (optional)
  - **Pattern types**: Off, Solid, Slow/Medium/Fast blink, Double blink, Dim solid

## Quick Start

### 1. Flash the Firmware

Visit the [**Web Flasher**](https://fchapleau.github.io/teams-redlight/) to flash your ESP32 directly from your browser using WebSerial.

**Requirements:**
- Chrome or Edge browser (WebSerial support required)
- ESP32 development board
- USB cable

### 2. Hardware Setup

#### Wiring Diagram

```
ESP32 Development Board
┌─────────────────────┐
│                     │
│  GPIO 2 ●───────────┼─── 220Ω Resistor ─── LED (Anode +)
│                     │                            │
│     GND ●───────────┼────────────────────────────┘ (Cathode -)
│                     │
│   3.3V ●           │ (Optional: External power)
│                     │
│     EN ●           │ (Reset button)
│   BOOT ●           │ (Flash button)
│                     │
└─────────────────────┘
```

#### Component List

- ESP32 Development Board (ESP32-DevKitC, WROOM, or similar)
- Red LED (3mm or 5mm)
- 220Ω resistor (or appropriate value for your LED)
- Jumper wires or breadboard
- USB cable for programming and power

#### Connection Steps

1. **Connect the LED:**
   - LED Anode (longer leg, +) → 220Ω Resistor → GPIO 2
   - LED Cathode (shorter leg, -) → GND

2. **Power the ESP32:**
   - Connect via USB cable to computer or USB power supply

### 3. Initial Configuration

1. **Power on the ESP32** - The LED will blink very fast indicating configuration mode
2. **Connect to WiFi network** named "Teams Red Light" (password: "configure")
3. **Open browser** and go to http://192.168.4.1 (HTTPS no longer required for setup!)
4. **Configure your settings:**
   - WiFi network credentials
   - Microsoft Teams/Office 365 settings
   - Azure AD application credentials
   - LED pattern preferences (optional)

### 4. Microsoft Azure AD Setup

Before configuring the device, you need to register an application in Azure AD:

1. **Go to** [Azure Portal](https://portal.azure.com)
2. **Navigate to** Azure Active Directory → App registrations
3. **Click** "New registration"
4. **Enter** application details:
   - Name: "Teams Red Light"
   - Supported account types: Choose based on your organization
   - Redirect URI: **Not required!** (leave empty or use placeholder like `https://example.com`)

5. **Configure API permissions:**
   - Add permission: Microsoft Graph → Delegated permissions
   - Select: `Presence.Read`
   - Grant admin consent (if required)

6. **Create client secret:**
   - Go to "Certificates & secrets"
   - New client secret
   - Copy the secret value (you'll need this for ESP32 configuration)

7. **Note down:**
   - Application (client) ID
   - Directory (tenant) ID
   - Client secret value

## Configuration

### Web Interface

The device provides a comprehensive web interface for configuration:

- **WiFi Settings**: Configure network connection
- **Teams Settings**: Set up Microsoft Graph API access
- **LED Pattern Settings**: Customize LED behavior for different states
- **Device Status**: Monitor connection and presence status
- **Firmware Updates**: Update device firmware remotely

### Configuration Options

| Setting | Description | Required |
|---------|-------------|----------|
| WiFi SSID | Your WiFi network name | Yes |
| WiFi Password | Your WiFi network password | Yes |
| Email Address | Your Teams/Office 365 email | Yes |
| Client ID | Azure AD Application ID | Yes |
| Client Secret | Azure AD Application Secret | Yes |
| Tenant ID | Azure AD Tenant ID (can be "common") | No |
| Meeting LED Pattern | LED behavior during meetings/busy | No |
| Available LED Pattern | LED behavior when available | No |
| OTA URL | Firmware update URL | No |

## LED Status Indicators

### System Status Patterns (Fixed)

| Pattern | Meaning |
|---------|---------|
| Very fast blink (100ms) | Configuration mode - connect to "Teams Red Light" WiFi |
| Slow blink (1000ms) | Connecting to WiFi network |
| Fast blink (200ms) | Connecting to Microsoft Graph API |

### Teams Presence Patterns (Configurable)

**Meeting/Busy Status** - Choose from these patterns when in a meeting or busy:
- **Solid** (default) - LED stays on continuously
- **Slow Blink (1s)** - LED blinks every second
- **Medium Blink (0.5s)** - LED blinks twice per second
- **Fast Blink (0.2s)** - LED blinks 5 times per second
- **Double Blink** - Two quick blinks followed by a pause
- **Dim Solid** - LED stays on at reduced brightness
- **Off** - LED turns off

**Available Status** - Choose from these patterns when available/away/offline:
- **Off** (default) - LED turns off
- **Solid** - LED stays on continuously
- **Slow Blink (1s)** - LED blinks every second
- **Medium Blink (0.5s)** - LED blinks twice per second
- **Fast Blink (0.2s)** - LED blinks 5 times per second
- **Double Blink** - Two quick blinks followed by a pause
- **Dim Solid** - LED stays on at reduced brightness

> **Note:** 
> - Both the external LED (on GPIO 2) and the ESP32's onboard LED will show the same status patterns simultaneously
> - LED patterns for Teams presence can be customized in the web configuration interface
> - System status patterns (configuration, WiFi, API connection) cannot be changed

## Troubleshooting

### Common Issues

**LED blinks very fast continuously**
- Device is in configuration mode
- Connect to "Teams Red Light" WiFi and configure settings

**LED blinks slowly continuously**
- Cannot connect to WiFi
- Check WiFi credentials in configuration

**LED blinks fast continuously**
- Cannot authenticate with Microsoft Graph
- Check Azure AD configuration and credentials
- Re-authenticate through web interface

**LED doesn't light up during meetings**
- Check Teams presence status in web interface
- Verify API permissions in Azure AD
- Check token expiration and refresh

### Reset to Factory Settings

1. Hold the BOOT button on ESP32
2. Press and release the EN/RST button
3. Release the BOOT button
4. Device will restart in configuration mode

### Debug Mode

Flash the debug firmware for detailed logging:
1. Download `teams-redlight-firmware-debug.bin` from releases
2. Use web flasher to install debug version
3. Connect to serial monitor at 115200 baud for detailed logs

## Development

### Building from Source

**Requirements:**
- PlatformIO
- Python 3.7+

**Build steps:**
```bash
# Clone repository
git clone https://github.com/fchapleau/teams-redlight.git
cd teams-redlight

# Install PlatformIO
pip install platformio

# Build firmware
pio run -e esp32dev

# Upload to device
pio run -e esp32dev -t upload

# Run tests
pio test
```

### Project Structure

```
├── src/                 # ESP32 firmware source code
│   ├── main.cpp        # Main application code
│   └── config.h        # Configuration constants
├── web/                # Web flasher interface
│   └── index.html      # WebSerial-based flasher
├── test/               # Unit tests
├── docs/               # Additional documentation
├── .github/workflows/  # CI/CD pipeline
└── platformio.ini      # PlatformIO configuration
```

### Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

## Security Considerations

- **Client secrets** are stored securely in ESP32 flash memory
- **Access tokens** are refreshed automatically
- **Network traffic** uses HTTPS for Microsoft Graph API
- **Device Code Flow** eliminates need for SSL certificates on ESP32
- **Secure authentication** via Microsoft's servers (no local SSL required)

### Authentication Security (Device Code Flow)

The device uses Microsoft's Device Code Flow for secure authentication:

- **No SSL certificates required** on the ESP32 device
- **No redirect URIs needed** - eliminates SSL complexity
- **Secure by design**: Authentication happens on Microsoft's HTTPS servers
- **IoT-optimized**: Designed specifically for resource-constrained devices
- **User-friendly**: Enter code on any internet-connected device

**Authentication Process:**
1. Device displays a user code and verification URL
2. User visits verification URL on phone/computer
3. User enters the device code and signs in with Microsoft
4. Device automatically receives secure access tokens
5. No local SSL certificates or complex network configuration needed

**Benefits over traditional OAuth:**
- ✅ No SSL certificate management
- ✅ No redirect URI configuration 
- ✅ No network firewall issues
- ✅ Works from any device with internet access
- ✅ More secure (authentication on Microsoft's servers)

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) file for details.

## Support

- **Issues**: Report bugs and feature requests on [GitHub Issues](https://github.com/fchapleau/teams-redlight/issues)
- **Discussions**: Community support on [GitHub Discussions](https://github.com/fchapleau/teams-redlight/discussions)
- **Wiki**: Additional documentation on [GitHub Wiki](https://github.com/fchapleau/teams-redlight/wiki)

## Acknowledgments

- Microsoft Graph API for Teams presence data
- ESP32 community for excellent documentation and libraries
- PlatformIO for cross-platform development tools
