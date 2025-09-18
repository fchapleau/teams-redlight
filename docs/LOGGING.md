# Logging Documentation

## Overview

The Teams Red Light project now includes comprehensive logging functionality that provides detailed information about the device's operation through the serial port. This helps with troubleshooting and monitoring the device's status.

## Log Levels

The logging system supports multiple log levels:

- **DEBUG (0)**: Detailed information for debugging purposes
- **INFO (1)**: General informational messages (default)
- **WARN (2)**: Warning messages for potential issues
- **ERROR (3)**: Error messages for actual problems
- **NONE (4)**: Disable all logging

## Configuration

### Build-Time Configuration

You can set the log level at compile time using PlatformIO environments:

- `esp32dev`: INFO level (default)
- `esp32dev_debug`: DEBUG level (most verbose)
- `esp32dev_production`: WARN level (minimal logging)

### Runtime Configuration

The log level can also be changed at runtime using:

```cpp
Logger::setLevel(LOG_LEVEL_DEBUG);
```

## Usage Examples

### Basic Logging

```cpp
Logger::info("Device started successfully");
Logger::warn("WiFi signal weak");
Logger::error("Failed to connect to API");
```

### Formatted Logging

```cpp
Logger::infof("Connected to WiFi: %s", ssid.c_str());
Logger::errorf("HTTP request failed with code: %d", httpCode);
```

### Component-Based Logging

```cpp
Logger::info("WiFi", "Connection established");
Logger::error("OAuth", "Authentication failed");
```

### Convenience Macros

```cpp
LOG_INFO("This is an info message");
LOG_DEBUGF("User ID: %s", userId.c_str());
```

## Log Output Format

Each log entry includes:

1. **Timestamp**: `[HH:MM:SS.mmm]` - Hours, minutes, seconds, milliseconds since boot
2. **Log Level**: Color-coded level indicator
3. **Component** (optional): Function or module name
4. **Message**: The actual log message

Example output:
```
[00:01:23.456] [INFO]  [setup] Teams Red Light - Starting...
[00:01:23.500] [DEBUG] [setupLED] Configuring LED on pin 2
[00:01:23.600] [INFO]  [setupWiFiSTA] Connecting to WiFi network: MyNetwork
[00:01:25.123] [INFO]  [setupWiFiSTA] WiFi connection successful!
[00:01:25.124] [INFO]  [setupWiFiSTA] IP address: 192.168.1.100
```

## Logged Operations

The system logs detailed information about:

### WiFi Operations
- Access Point setup and configuration
- Station mode connection attempts
- Connection success/failure with network details
- Signal strength and network information

### OAuth Authentication
- Login redirect generation
- Authorization code exchange
- Token refresh operations
- Authentication errors and success

### Teams Presence Monitoring
- API request attempts
- Presence state changes
- Token expiration and refresh
- API errors and responses

### Device Configuration
- Configuration loading from flash
- Configuration changes and saves
- Parameter validation
- Device restarts

### LED Status
- LED pattern changes based on device state
- Presence-based LED behavior
- State transitions

## Troubleshooting with Logs

### Common Issues and Log Patterns

1. **WiFi Connection Problems**
   ```
   [ERROR] [setupWiFiSTA] Failed to connect to WiFi after 30 seconds. Status: 6
   [ERROR] [setupWiFiSTA] Network not found - check SSID
   ```

2. **OAuth Authentication Issues**
   ```
   [ERROR] [handleCallback] OAuth authentication error: invalid_client - Client authentication failed
   [WARN]  [checkTeamsPresence] Teams API returned 401 Unauthorized - token may be expired
   ```

3. **API Communication Problems**
   ```
   [ERROR] [checkTeamsPresence] Teams presence API failed: HTTP 503
   [INFO]  [refreshAccessToken] Token refresh successful!
   ```

## Serial Monitor Setup

To view logs:

1. Connect the ESP32 via USB
2. Open a serial monitor at 115200 baud
3. Reset the device to see startup logs

### PlatformIO Serial Monitor
```bash
pio device monitor --baud 115200
```

### Arduino IDE Serial Monitor
- Tools → Serial Monitor
- Set baud rate to 115200

## Color Output

When using a terminal that supports ANSI colors, log levels are color-coded:
- DEBUG: Cyan
- INFO: Green  
- WARN: Yellow
- ERROR: Red

This makes it easy to quickly identify different types of messages in the log output.