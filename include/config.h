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
#define HTTPS_PORT 443
#define ENABLE_HTTPS true   // HTTPS support enabled using ESPAsyncWebServer with SSL

// Microsoft Graph API Configuration
#define GRAPH_API_HOST "graph.microsoft.com"
#define GRAPH_API_ENDPOINT "/v1.0/me/presence"
#define GRAPH_LOGIN_HOST "login.microsoftonline.com"

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

// SSL Certificate Storage Keys
#define KEY_SSL_CERT "ssl_cert"
#define KEY_SSL_KEY "ssl_key"
#define KEY_SSL_ENABLED "ssl_enabled"

// Update Configuration
#define OTA_UPDATE_URL_KEY "ota_url"
#define DEFAULT_OTA_URL "https://github.com/fchapleau/teams-redlight/releases/latest/download/firmware.bin"

// Device States
enum DeviceState {
  STATE_AP_MODE,
  STATE_CONNECTING_WIFI,
  STATE_CONNECTING_OAUTH,
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