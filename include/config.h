#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define AP_SSID "Teams Red Light"
#define AP_PASSWORD "configure"
#define AP_TIMEOUT 300000  // 5 minutes

// LED Configuration
#define MAX_LEDS 8                       // Maximum number of configurable LEDs
#define LED_PIN 2                        // Default external LED pin (for backward compatibility)
#ifndef LED_BUILTIN
  #define LED_BUILTIN_PIN 2             // Onboard LED pin (fallback if not defined by board)
#else
  #define LED_BUILTIN_PIN LED_BUILTIN   // Use Arduino framework's built-in LED pin
#endif
#define LED_SLOW_BLINK_INTERVAL 1000    // 1 second - no network
#define LED_FAST_BLINK_INTERVAL 200     // 200ms - connecting to O365
#define LED_VERY_FAST_BLINK_INTERVAL 100 // 100ms - AP mode

// Available GPIO pins for LEDs on ESP32
#define AVAILABLE_GPIO_PINS {2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33}

// Network Configuration
#define HTTP_PORT 80

// Microsoft Graph API Configuration
#define GRAPH_API_HOST "graph.microsoft.com"
#define GRAPH_API_ENDPOINT "/v1.0/me/presence"
#define GRAPH_LOGIN_HOST "login.microsoftonline.com"

// Time Configuration
#define NTP_SERVER "pool.ntp.org"
#define NTP_TIMEZONE_OFFSET 0       // UTC by default
#define NTP_DAYLIGHT_OFFSET 0       // No daylight saving by default
#define TIME_UPDATE_INTERVAL 3600000 // Update time every hour (in milliseconds)

// Device Code Flow Configuration
#define DEVICE_CODE_SCOPE "https://graph.microsoft.com/Presence.Read offline_access"
#define DEVICE_CODE_POLL_INTERVAL 5000  // 5 seconds
#define DEVICE_CODE_TIMEOUT 900000      // 15 minutes

// Storage Keys
#define PREF_NAMESPACE "teamslight"
#define KEY_WIFI_SSID "wifi_ssid"
#define KEY_WIFI_PASS "wifi_pass"
#define KEY_CLIENT_ID "client_id"
#define KEY_CLIENT_SECRET "client_secret"
#define KEY_TENANT_ID "tenant_id"
#define KEY_USER_EMAIL "user_email"
#define KEY_ACCESS_TOKEN "access_token"
#define KEY_REFRESH_TOKEN "refresh_token"
#define KEY_TOKEN_EXPIRES "token_expires"

// Device Code Flow Storage Keys
#define KEY_DEVICE_CODE "device_code"
#define KEY_USER_CODE "user_code"
#define KEY_VERIFICATION_URI "verify_uri"
#define KEY_DEVICE_CODE_EXPIRES "dev_code_exp"

// Presence Logging Storage Keys
#define KEY_PRESENCE_LOG_COUNT "pres_log_count"
#define KEY_PRESENCE_LOG_PREFIX "pres_log_"
#define MAX_PRESENCE_LOGS 50  // Maximum number of presence logs to store

// Time Configuration Storage Keys
#define KEY_TIMEZONE_OFFSET "timezone_offset"
#define KEY_DAYLIGHT_OFFSET "daylight_offset"

// LED Pattern Storage Keys
#define KEY_MEETING_PATTERN "meeting_pattern"
#define KEY_NO_MEETING_PATTERN "no_meeting_pattern"
#define KEY_LED_COUNT "led_count"
#define KEY_LED_PIN_PREFIX "led_pin_"
#define KEY_LED_CALL_PATTERN_PREFIX "led_call_"
#define KEY_LED_MEETING_PATTERN_PREFIX "led_meet_"
#define KEY_LED_AVAILABLE_PATTERN_PREFIX "led_avail_"
#define KEY_LED_AWAY_PATTERN_PREFIX "led_away_"
#define KEY_LED_OFFLINE_PATTERN_PREFIX "led_offline_"

// Update Configuration
#define OTA_UPDATE_URL_KEY "ota_url"
#define DEFAULT_OTA_URL "https://github.com/fchapleau/teams-redlight/releases/latest/download/firmware.bin"

// LED Pattern Types
enum LEDPattern {
  PATTERN_OFF = 0,
  PATTERN_SOLID = 1,
  PATTERN_SLOW_BLINK = 2,       // 1000ms intervals
  PATTERN_MEDIUM_BLINK = 3,     // 500ms intervals  
  PATTERN_FAST_BLINK = 4,       // 200ms intervals
  PATTERN_DOUBLE_BLINK = 5,     // Double blink every 1000ms
  PATTERN_DIM_SOLID = 6         // Always on but dimmed (for PWM-capable pins)
};

// Default LED patterns
#define DEFAULT_CALL_PATTERN PATTERN_FAST_BLINK
#define DEFAULT_MEETING_PATTERN PATTERN_SOLID
#define DEFAULT_AVAILABLE_PATTERN PATTERN_OFF
#define DEFAULT_AWAY_PATTERN PATTERN_OFF
#define DEFAULT_OFFLINE_PATTERN PATTERN_OFF

// LED Pattern Intervals
#define LED_PATTERN_SLOW_BLINK_INTERVAL 1000     // 1 second
#define LED_PATTERN_MEDIUM_BLINK_INTERVAL 500    // 500ms
#define LED_PATTERN_FAST_BLINK_INTERVAL 200      // 200ms
#define LED_PATTERN_DOUBLE_BLINK_INTERVAL 1000   // 1 second cycle
#define LED_PATTERN_DOUBLE_BLINK_ON_TIME 100     // 100ms on time for double blink

// Device States
enum DeviceState {
  STATE_AP_MODE,
  STATE_CONNECTING_WIFI,
  STATE_CONNECTING_OAUTH,
  STATE_DEVICE_CODE_PENDING,  // New state for device code flow
  STATE_AUTHENTICATED,
  STATE_MONITORING,
  STATE_ERROR
};

// Teams Presence States
enum TeamsPresence {
  PRESENCE_UNKNOWN,
  PRESENCE_AVAILABLE,
  PRESENCE_BUSY,
  PRESENCE_IN_MEETING,
  PRESENCE_AWAY,
  PRESENCE_OFFLINE
};

// LED Configuration Structure
struct LEDConfig {
  uint8_t pin;
  LEDPattern callPattern;
  LEDPattern meetingPattern;
  LEDPattern availablePattern;
  LEDPattern awayPattern;
  LEDPattern offlinePattern;
  bool enabled;
  // Pattern state variables
  unsigned long lastToggle;
  bool state;
  unsigned long doubleBlinksStartTime;
  bool doubleBlinksState;
  int doubleBlinksCount;
};

// Presence Log Entry Structure
struct PresenceLogEntry {
  time_t timestamp;
  TeamsPresence presence;
  char presenceString[20]; // Human readable presence
};

#endif // CONFIG_H
