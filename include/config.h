#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define AP_SSID "Teams Red Light"
#define AP_PASSWORD "configure"
#define AP_TIMEOUT 300000  // 5 minutes

// LED Configuration
#define LED_PIN 2                        // External LED pin
#ifndef LED_BUILTIN
  #define LED_BUILTIN_PIN 2             // Onboard LED pin (fallback if not defined by board)
#else
  #define LED_BUILTIN_PIN LED_BUILTIN   // Use Arduino framework's built-in LED pin
#endif
#define LED_SLOW_BLINK_INTERVAL 1000    // 1 second - no network
#define LED_FAST_BLINK_INTERVAL 200     // 200ms - connecting to O365
#define LED_VERY_FAST_BLINK_INTERVAL 100 // 100ms - AP mode

// Network Configuration
#define HTTP_PORT 80

// Microsoft Graph API Configuration
#define GRAPH_API_HOST "graph.microsoft.com"
#define GRAPH_API_ENDPOINT "/v1.0/me/presence"
#define GRAPH_LOGIN_HOST "login.microsoftonline.com"

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

// LED Pattern Storage Keys
#define KEY_MEETING_PATTERN "meeting_pattern"
#define KEY_NO_MEETING_PATTERN "no_meeting_pattern"

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
#define DEFAULT_MEETING_PATTERN PATTERN_SOLID
#define DEFAULT_NO_MEETING_PATTERN PATTERN_OFF

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

#endif // CONFIG_H
