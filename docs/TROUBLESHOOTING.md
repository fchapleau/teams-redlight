# Troubleshooting Guide

This guide helps resolve common issues with the Teams Red Light project, including development environment setup and CI/CD pipeline problems.

## Development Environment Issues

### PlatformIO Registry Access Issues

**Problem:** Cannot download PlatformIO packages due to DNS blocks or network restrictions
```
HTTPClientError: 
api.registry.platformio.org
```

**Solutions:**

#### Option 1: Local Development
1. **Use PlatformIO IDE:** Download and install PlatformIO IDE locally
2. **Manual package installation:** Download packages manually from GitHub
3. **Offline development:** Work with cached packages

#### Option 2: Network Configuration
If you have admin access to your network:

1. **Add to allowlist:**
   - `api.registry.platformio.org`
   - `dl.platformio.org` 
   - `registry.platformio.org`
   - `collector.platformio.org`

2. **Corporate proxy:** Configure PlatformIO to use corporate proxy:
   ```bash
   pio settings set enable_proxy true
   pio settings set proxy_server http://proxy.company.com:8080
   ```

#### Option 3: Alternative Development Approach
1. **Arduino IDE:** Use Arduino IDE with ESP32 board support
2. **ESP-IDF:** Use Espressif's native development framework
3. **Docker container:** Use a pre-configured development container

### GitHub Actions / CI Pipeline Issues

**Problem:** GitHub Actions failing due to network restrictions

**Solution:** The updated workflow now includes:
- Network connectivity testing
- Graceful fallback to mock builds
- Separated validation from firmware compilation
- Continue-on-error for network-dependent steps

#### Workflow Components:

1. **Validation Job** - Always runs:
   - Project structure validation
   - Code syntax checking
   - Documentation validation
   
2. **Firmware Build Job** - Network dependent:
   - Tests PlatformIO registry access
   - Falls back to mock firmware if blocked
   - Uses `continue-on-error` to prevent pipeline failure

3. **Web Deployment** - Always runs:
   - Deploys web flasher interface
   - Works with or without firmware builds

**Problem:** GitHub Pages deployment fails with "Permission denied" error

**Solution:** Ensure the workflow has required permissions:
```yaml
permissions:
  contents: write
  pages: write  
  id-token: write
```

**Problem:** GitHub Pages deployment fails with "Branch not allowed to deploy" error

**Solution:** Environment protection rules restrict deployments to main branch only. The workflow includes a conditional to only deploy from main branch:
```yaml
deploy-web:
  if: github.ref == 'refs/heads/main' || github.event_name == 'release' || github.event_name == 'workflow_dispatch'
```

This ensures that:
- Pull requests can run validation and build tests
- Only main branch, releases, and manual triggers deploy to GitHub Pages
- Feature branches don't attempt GitHub Pages deployment

## Runtime Issues

### ESP32 Connection Problems

**LED blinks very fast (100ms interval)**
- Device is in configuration mode
- Connect to "Teams Red Light" WiFi network
- Navigate to http://192.168.4.1
- Configure WiFi credentials

**LED blinks slowly (1000ms interval)**
- Cannot connect to configured WiFi
- Check WiFi credentials in configuration
- Verify WiFi network is accessible
- Check signal strength

**LED blinks fast (200ms interval)**
- Cannot connect to Microsoft Graph API
- Check internet connectivity
- Verify Azure AD configuration
- Check firewall settings

**LED doesn't respond to Teams status**
- Check OAuth token validity
- Verify Microsoft Graph API permissions
- Check device logs via serial monitor
- Re-authenticate through web interface

### Network Configuration

**Cannot access configuration portal**
1. Ensure ESP32 is in AP mode (very fast LED blinking)
2. Connect to "Teams Red Light" network (password: "configure")
3. Try these addresses:
   - http://192.168.4.1
   - http://192.168.4.1:80
4. Disable mobile data on phone if using mobile hotspot

**WiFi connection fails**
1. Check SSID and password are correct
2. Verify WiFi network supports 2.4GHz (ESP32 limitation)
3. Check for special characters in password
4. Try connecting to a different network for testing

### Microsoft Graph API Issues

**Authentication fails**
1. Verify Azure AD application configuration:
   - Correct Client ID and Secret
   - Proper redirect URI: `http://[ESP32-IP]/callback`
   - Required permissions: `Presence.Read`
2. Check tenant ID (use "common" for personal accounts)
3. Ensure admin consent is granted if required

**Presence not updating**
1. Check token expiration and refresh
2. Verify Teams presence is actually changing
3. Check API rate limits
4. Monitor serial output for API responses

## Hardware Issues

### LED Problems

**LED doesn't light up**
- Check LED polarity (anode to resistor, cathode to GND)
- Verify GPIO 2 connection
- Test LED with multimeter
- Check resistor value (220Ω recommended)

**LED too dim/bright**
- Adjust resistor value:
  - Higher resistance = dimmer LED
  - Lower resistance = brighter LED (don't go below 150Ω)

**LED stays on constantly**
- Check for short circuit
- Verify firmware is running correctly
- Monitor serial output for status

### Power Issues

**ESP32 doesn't start**
- Check USB cable and power supply
- Verify ESP32 board connections
- Try different USB port/computer
- Check for damaged board

**Frequent resets**
- Insufficient power supply
- Loose connections
- Software crashes (check serial monitor)

## Development Workflow Issues

### Building Firmware Locally

If GitHub Actions can't build firmware due to network restrictions:

1. **Clone repository locally:**
   ```bash
   git clone https://github.com/fchapleau/teams-redlight.git
   cd teams-redlight
   ```

2. **Install PlatformIO CLI:**
   ```bash
   pip install platformio
   ```

3. **Build firmware:**
   ```bash
   pio run -e esp32dev
   ```

4. **Upload to ESP32:**
   ```bash
   pio run -e esp32dev -t upload
   ```

### Alternative Build Methods

#### Using Arduino IDE

1. Install ESP32 board support in Arduino IDE
2. Copy code from `src/main.cpp` to Arduino IDE
3. Install required libraries:
   - ArduinoJson
   - WiFi (built-in)
   - WebServer (built-in)
   - HTTPClient (built-in)
   - Preferences (built-in)

#### Using ESP-IDF

1. Install ESP-IDF development framework
2. Convert Arduino code to ESP-IDF components
3. Use ESP-IDF build system

### Testing Without Hardware

**Web Interface Testing:**
1. Open `web/index.html` in browser
2. Test WebSerial interface (requires Chrome/Edge)
3. Validate UI responsiveness

**Code Validation:**
1. Run syntax checkers locally
2. Use the validation script: `./test/validate.sh`
3. Check includes and function definitions

## Web Interface Issues

### CORS Policy Errors

**Problem:** Firmware download fails with CORS policy error when accessing GitHub Pages
```
Access to fetch at 'https://release-assets.githubusercontent.com/...' from origin 'https://fchapleau.github.io' has been blocked by CORS policy
```

**Solution:** This issue has been resolved in the latest version by:
1. **Primary method:** Serving firmware files from GitHub Pages (same origin)
2. **Fallback method:** Using `browser_download_url` instead of API URLs for direct downloads

**If you still experience CORS errors:**
1. **Clear browser cache** and reload the page
2. **Check firmware availability** - ensure the latest release includes firmware files
3. **Verify deployment** - check that GitHub Pages deployment completed successfully
4. **Try alternative browser** - use Chrome or Edge for best WebSerial support

### WebSerial API Not Supported

**Problem:** Browser displays "Web Serial API Not Supported" message

**Solution:**
- Use Chrome 89+ or Edge 89+ (WebSerial requirement)
- Firefox and Safari are not currently supported
- Ensure site is served over HTTPS (required for WebSerial)

### Firmware Download Timeout

**Problem:** Firmware download takes too long or times out

**Solutions:**
1. **Check network connection** - ensure stable internet connectivity
2. **Try smaller firmware** - debug builds are larger than production builds
3. **Use local firmware** - upload custom firmware file instead of downloading

## Corporate Environment Considerations

### Firewall Rules

Request IT department to allow:
- `*.platformio.org` for development tools
- `*.microsoft.com` for Graph API access
- `login.microsoftonline.com` for authentication
- `graph.microsoft.com` for presence data

### Proxy Configuration

**For PlatformIO:**
```bash
export HTTP_PROXY=http://proxy.company.com:8080
export HTTPS_PROXY=http://proxy.company.com:8080
```

**For ESP32 (in firmware):**
```cpp
// Add to main.cpp if needed
WiFiClientSecure client;
client.setProxy("proxy.company.com", 8080);
```

### Security Policies

**Certificate validation:**
- Corporate networks may require custom CA certificates
- Add to ESP32 firmware if HTTPS connections fail

**Network segmentation:**
- Ensure ESP32 can reach internet from corporate WiFi
- May need guest network or IoT VLAN

## Getting Help

### Debug Information to Collect

When reporting issues, include:

1. **Hardware information:**
   - ESP32 board model
   - LED type and resistor value
   - Power supply details

2. **Software information:**
   - Firmware version
   - Arduino IDE or PlatformIO version
   - Operating system

3. **Network information:**
   - WiFi router model
   - Network configuration
   - Firewall/proxy settings

4. **Error logs:**
   - Serial monitor output
   - Browser console errors
   - GitHub Actions logs

### Support Channels

- **GitHub Issues:** Report bugs and feature requests
- **GitHub Discussions:** Community support and questions
- **Serial Monitor:** Real-time debugging information
- **Documentation:** Check README.md and docs/ folder

### Diagnostic Commands

**Check ESP32 status:**
```bash
# Open serial monitor
pio device monitor --baud 115200
```

**Test network connectivity:**
```bash
# From ESP32 serial monitor
ping 8.8.8.8
```

**Check GitHub Actions:**
- View workflow runs in repository Actions tab
- Download logs from failed runs
- Check for DNS-related errors