#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Update.h>
#include "config.h"
#include "logging.h"

// Global objects
WebServer server(HTTP_PORT);
Preferences preferences;
WiFiClientSecure client;

// Global state
DeviceState currentState = STATE_AP_MODE;
TeamsPresence currentPresence = PRESENCE_UNKNOWN;
unsigned long lastLedToggle = 0;
bool ledState = false;
unsigned long lastPresenceCheck = 0;
unsigned long tokenExpires = 0;

// LED Pattern state
LEDPattern callPattern = DEFAULT_CALL_PATTERN;
LEDPattern meetingPattern = DEFAULT_MEETING_PATTERN;
LEDPattern availablePattern = DEFAULT_AVAILABLE_PATTERN;
unsigned long doubleBlinksStartTime = 0;
bool doubleBlinksState = false;
int doubleBlinksCount = 0;

// Multiple LED configuration
LEDConfig leds[MAX_LEDS];
uint8_t ledCount = 0;
const uint8_t availableGPIOPins[] = AVAILABLE_GPIO_PINS;
const uint8_t availableGPIOCount = sizeof(availableGPIOPins) / sizeof(availableGPIOPins[0]);

// Configuration variables
String wifiSSID;
String wifiPassword;
String clientId;
String clientSecret;
String tenantId;
String userEmail;
String accessToken;
String refreshToken;

// Device Code Flow variables
String deviceCode;
String userCode;
String verificationUri;
unsigned long deviceCodeExpires = 0;
unsigned long lastDeviceCodePoll = 0;

// Function declarations
void setupLED();
void updateLED();
void setLEDState(int state);  // Helper function to control both LEDs (legacy)
void applyLEDPattern(LEDPattern pattern);  // Apply selected LED pattern (legacy)
void setupMultipleLEDs();  // Setup multiple LEDs
void updateMultipleLEDs(); // Update multiple LEDs
void setLEDState(uint8_t ledIndex, int state);  // Set state for specific LED
void applyLEDPattern(uint8_t ledIndex, LEDPattern pattern);  // Apply pattern to specific LED
void initDefaultLEDs();  // Initialize with default LED configuration
void setupWiFiAP();
void setupWiFiSTA();
void setupWebServer();
void handleRoot();
void handleConfig();
void handleSave();
void handleStatus();
void handleUpdate();
void handleLogin();
void handleCallback();
bool startDeviceCodeFlow();
bool pollDeviceCodeToken();
void checkTeamsPresence();
bool refreshAccessToken();
void loadConfiguration();
void saveConfiguration();

void setup() {
  Logger::begin(115200);
  LOG_INFO("Teams Red Light - Starting...");
  
  LOG_DEBUG("Setting up LED");
  setupLED();
  
  LOG_DEBUG("Initializing preferences");
  // Initialize preferences
  preferences.begin(PREF_NAMESPACE, false);
  
  LOG_DEBUG("Loading configuration");
  // Load saved configuration
  loadConfiguration();
  
  // Check if device code flow was in progress
  if (deviceCode.length() > 0 && deviceCodeExpires > millis()) {
    LOG_INFO("Resuming device code flow from previous session");
    currentState = STATE_DEVICE_CODE_PENDING;
    lastDeviceCodePoll = millis();
  }
  
  // Set up WiFi
  if (wifiSSID.length() > 0) {
    LOG_INFOF("WiFi credentials found, connecting to: %s", wifiSSID.c_str());
    setupWiFiSTA();
  } else {
    LOG_INFO("No WiFi credentials found, starting in AP mode");
    setupWiFiAP();
  }
  
  LOG_DEBUG("Setting up web server");
  // Set up web server
  setupWebServer();
  
  LOG_INFO("Setup complete");
}

void loop() {
  server.handleClient();  // Handle incoming HTTP requests
  
  updateLED();
  
  // Handle different states
  switch (currentState) {
    case STATE_AP_MODE:
      // Just blink LED and wait for configuration
      break;
      
    case STATE_CONNECTING_WIFI:
      if (WiFi.status() == WL_CONNECTED) {
        LOG_INFO("WiFi connected successfully!");
        LOG_INFOF("IP address: %s", WiFi.localIP().toString().c_str());
        LOG_INFOF("Signal strength: %d dBm", WiFi.RSSI());
        
        if (accessToken.length() > 0) {
          LOG_DEBUG("Access token found, transitioning to authenticated state");
          currentState = STATE_AUTHENTICATED;
        } else {
          LOG_DEBUG("No access token found, waiting for OAuth authentication");
          currentState = STATE_CONNECTING_OAUTH;
        }
      }
      break;
      
    case STATE_CONNECTING_OAUTH:
      LOG_DEBUG("Waiting for OAuth configuration...");
      // Wait for OAuth configuration
      break;
      
    case STATE_DEVICE_CODE_PENDING:
      // Poll for device code completion
      if (millis() - lastDeviceCodePoll > DEVICE_CODE_POLL_INTERVAL) {
        if (millis() > deviceCodeExpires) {
          LOG_WARN("Device code expired, returning to OAuth state");
          currentState = STATE_CONNECTING_OAUTH;
        } else if (pollDeviceCodeToken()) {
          LOG_INFO("Device code authentication successful!");
          currentState = STATE_AUTHENTICATED;
        }
        lastDeviceCodePoll = millis();
      }
      break;
      
    case STATE_AUTHENTICATED:
      LOG_INFO("Authentication successful, starting monitoring");
      currentState = STATE_MONITORING;
      break;
      
    case STATE_MONITORING:
      if (millis() - lastPresenceCheck > 30000) { // Check every 30 seconds
        LOG_DEBUG("Checking Teams presence");
        checkTeamsPresence();
        lastPresenceCheck = millis();
      }
      break;
      
    case STATE_ERROR:
      LOG_ERROR("Device in error state");
      // Handle error state
      break;
  }
  
  delay(100);
}

void initDefaultLEDs() {
  // Initialize with default LED (GPIO 2) for backward compatibility
  if (ledCount == 0) {
    ledCount = 1;
    leds[0].pin = LED_PIN;
    leds[0].callPattern = DEFAULT_CALL_PATTERN;
    leds[0].meetingPattern = DEFAULT_MEETING_PATTERN;
    leds[0].availablePattern = DEFAULT_AVAILABLE_PATTERN;
    leds[0].enabled = true;
    leds[0].lastToggle = 0;
    leds[0].state = false;
    leds[0].doubleBlinksStartTime = 0;
    leds[0].doubleBlinksState = false;
    leds[0].doubleBlinksCount = 0;
    
    LOG_INFO("Initialized with default LED configuration (GPIO 2)");
  }
}

void setupMultipleLEDs() {
  LOG_INFO("Setting up multiple LED configuration");
  
  // Ensure we have at least the default LED
  initDefaultLEDs();
  
  // Setup each enabled LED
  for (uint8_t i = 0; i < ledCount; i++) {
    if (leds[i].enabled) {
      LOG_DEBUGF("Configuring LED %d on GPIO pin %d", i, leds[i].pin);
      pinMode(leds[i].pin, OUTPUT);
      digitalWrite(leds[i].pin, LOW);
    }
  }
  
  // Setup onboard LED if it's different from configured LEDs
  bool onboardConfigured = false;
  for (uint8_t i = 0; i < ledCount; i++) {
    if (leds[i].enabled && leds[i].pin == LED_BUILTIN_PIN) {
      onboardConfigured = true;
      break;
    }
  }
  
  if (!onboardConfigured && LED_BUILTIN_PIN != LED_PIN) {
    LOG_DEBUGF("Configuring onboard LED on pin %d", LED_BUILTIN_PIN);
    pinMode(LED_BUILTIN_PIN, OUTPUT);
    digitalWrite(LED_BUILTIN_PIN, LOW);
  }
  
  LOG_INFOF("Multiple LED setup complete (%d LEDs configured)", ledCount);
}

void setupLED() {
  // Legacy function - now calls the new multi-LED setup
  setupMultipleLEDs();
}

void setLEDState(uint8_t ledIndex, int state) {
  if (ledIndex < ledCount && leds[ledIndex].enabled) {
    digitalWrite(leds[ledIndex].pin, state);
    leds[ledIndex].state = (state == HIGH);
  }
}

void setLEDState(int state) {
  // Legacy function - control all configured LEDs
  for (uint8_t i = 0; i < ledCount; i++) {
    if (leds[i].enabled) {
      setLEDState(i, state);
    }
  }
  
  // Control onboard LED if it's on a different pin
  bool onboardConfigured = false;
  for (uint8_t i = 0; i < ledCount; i++) {
    if (leds[i].enabled && leds[i].pin == LED_BUILTIN_PIN) {
      onboardConfigured = true;
      break;
    }
  }
  
  if (!onboardConfigured && LED_BUILTIN_PIN != LED_PIN) {
    digitalWrite(LED_BUILTIN_PIN, state);
  }
}

void applyLEDPattern(uint8_t ledIndex, LEDPattern pattern) {
  if (ledIndex >= ledCount || !leds[ledIndex].enabled) return;
  
  unsigned long currentTime = millis();
  LEDConfig* led = &leds[ledIndex];
  
  switch (pattern) {
    case PATTERN_OFF:
      setLEDState(ledIndex, LOW);
      break;
      
    case PATTERN_SOLID:
      setLEDState(ledIndex, HIGH);
      break;
      
    case PATTERN_DIM_SOLID:
      // For simple on/off LEDs, this is the same as solid
      // Could be enhanced for PWM in future
      setLEDState(ledIndex, HIGH);
      break;
      
    case PATTERN_SLOW_BLINK:
      if (currentTime - led->lastToggle > LED_PATTERN_SLOW_BLINK_INTERVAL) {
        led->state = !led->state;
        setLEDState(ledIndex, led->state ? HIGH : LOW);
        led->lastToggle = currentTime;
      }
      break;
      
    case PATTERN_MEDIUM_BLINK:
      if (currentTime - led->lastToggle > LED_PATTERN_MEDIUM_BLINK_INTERVAL) {
        led->state = !led->state;
        setLEDState(ledIndex, led->state ? HIGH : LOW);
        led->lastToggle = currentTime;
      }
      break;
      
    case PATTERN_FAST_BLINK:
      if (currentTime - led->lastToggle > LED_PATTERN_FAST_BLINK_INTERVAL) {
        led->state = !led->state;
        setLEDState(ledIndex, led->state ? HIGH : LOW);
        led->lastToggle = currentTime;
      }
      break;
      
    case PATTERN_DOUBLE_BLINK:
      // Double blink pattern: on-off-on-off-pause
      if (led->doubleBlinksStartTime == 0) {
        led->doubleBlinksStartTime = currentTime;
        led->doubleBlinksCount = 0;
        led->doubleBlinksState = true;
        setLEDState(ledIndex, HIGH);
      } else {
        unsigned long elapsed = currentTime - led->doubleBlinksStartTime;
        
        if (led->doubleBlinksCount < 4) { // 4 transitions = 2 blinks
          if (elapsed > LED_PATTERN_DOUBLE_BLINK_ON_TIME * (led->doubleBlinksCount + 1)) {
            led->doubleBlinksState = !led->doubleBlinksState;
            setLEDState(ledIndex, led->doubleBlinksState ? HIGH : LOW);
            led->doubleBlinksCount++;
          }
        } else if (elapsed > LED_PATTERN_DOUBLE_BLINK_INTERVAL) {
          // Reset for next cycle
          led->doubleBlinksStartTime = 0;
        }
      }
      break;
      
    default:
      setLEDState(ledIndex, LOW);
      break;
  }
}

void applyLEDPattern(LEDPattern pattern) {
  // Legacy function - apply pattern to all LEDs (for backward compatibility)
  static unsigned long lastPatternToggle = 0;
  static bool patternState = false;
  unsigned long currentTime = millis();
  
  switch (pattern) {
    case PATTERN_OFF:
      setLEDState(LOW);
      break;
      
    case PATTERN_SOLID:
      setLEDState(HIGH);
      break;
      
    case PATTERN_DIM_SOLID:
      // For simple on/off LEDs, this is the same as solid
      // Could be enhanced for PWM in future
      setLEDState(HIGH);
      break;
      
    case PATTERN_SLOW_BLINK:
      if (currentTime - lastPatternToggle > LED_PATTERN_SLOW_BLINK_INTERVAL) {
        patternState = !patternState;
        setLEDState(patternState ? HIGH : LOW);
        lastPatternToggle = currentTime;
      }
      break;
      
    case PATTERN_MEDIUM_BLINK:
      if (currentTime - lastPatternToggle > LED_PATTERN_MEDIUM_BLINK_INTERVAL) {
        patternState = !patternState;
        setLEDState(patternState ? HIGH : LOW);
        lastPatternToggle = currentTime;
      }
      break;
      
    case PATTERN_FAST_BLINK:
      if (currentTime - lastPatternToggle > LED_PATTERN_FAST_BLINK_INTERVAL) {
        patternState = !patternState;
        setLEDState(patternState ? HIGH : LOW);
        lastPatternToggle = currentTime;
      }
      break;
      
    case PATTERN_DOUBLE_BLINK:
      // Double blink pattern: on-off-on-off-pause
      if (doubleBlinksStartTime == 0) {
        doubleBlinksStartTime = currentTime;
        doubleBlinksCount = 0;
        doubleBlinksState = true;
        setLEDState(HIGH);
      } else {
        unsigned long elapsed = currentTime - doubleBlinksStartTime;
        
        if (doubleBlinksCount < 4) { // 4 transitions = 2 blinks
          if (elapsed > LED_PATTERN_DOUBLE_BLINK_ON_TIME * (doubleBlinksCount + 1)) {
            doubleBlinksState = !doubleBlinksState;
            setLEDState(doubleBlinksState ? HIGH : LOW);
            doubleBlinksCount++;
          }
        } else if (elapsed > LED_PATTERN_DOUBLE_BLINK_INTERVAL) {
          // Reset for next cycle
          doubleBlinksStartTime = 0;
        }
      }
      break;
      
    default:
      setLEDState(LOW);
      break;
  }
}

void updateMultipleLEDs() {
  // Update each LED with its appropriate pattern based on current state
  for (uint8_t i = 0; i < ledCount; i++) {
    if (!leds[i].enabled) continue;
    
    switch (currentState) {
      case STATE_AUTHENTICATED:
      case STATE_MONITORING:
        // LED behavior based on Teams presence using individual patterns
        switch (currentPresence) {
          case PRESENCE_BUSY:
            applyLEDPattern(i, leds[i].callPattern);
            break;
          case PRESENCE_IN_MEETING:
            applyLEDPattern(i, leds[i].meetingPattern);
            break;
          default:
            applyLEDPattern(i, leds[i].availablePattern);
            break;
        }
        break;
      
      default:
        // For system states (AP mode, connecting, etc), use the legacy behavior
        // but apply to all LEDs synchronously
        break;
    }
  }
}

void updateLED() {
  unsigned long interval;
  static DeviceState lastLoggedState = STATE_ERROR; // Initialize to invalid state
  
  switch (currentState) {
    case STATE_AP_MODE:
      interval = LED_VERY_FAST_BLINK_INTERVAL;
      if (lastLoggedState != STATE_AP_MODE) {
        LOG_DEBUG("LED: Very fast blink (AP mode)");
        lastLoggedState = STATE_AP_MODE;
      }
      break;
    case STATE_CONNECTING_WIFI:
      interval = LED_SLOW_BLINK_INTERVAL;
      if (lastLoggedState != STATE_CONNECTING_WIFI) {
        LOG_DEBUG("LED: Slow blink (connecting to WiFi)");
        lastLoggedState = STATE_CONNECTING_WIFI;
      }
      break;
    case STATE_CONNECTING_OAUTH:
      interval = LED_FAST_BLINK_INTERVAL;
      if (lastLoggedState != STATE_CONNECTING_OAUTH) {
        LOG_DEBUG("LED: Fast blink (connecting to OAuth)");
        lastLoggedState = STATE_CONNECTING_OAUTH;
      }
      break;
    case STATE_AUTHENTICATED:
    case STATE_MONITORING:
      // Use the new multi-LED system for Teams presence
      updateMultipleLEDs();
      if (lastLoggedState != STATE_MONITORING) {
        LOG_INFOF("LED: Using individual patterns for %d LEDs", ledCount);
        lastLoggedState = STATE_MONITORING;
      }
      return;
    case STATE_ERROR:
      interval = LED_FAST_BLINK_INTERVAL;
      if (lastLoggedState != STATE_ERROR) {
        LOG_ERROR("LED: Fast blink (error state)");
        lastLoggedState = STATE_ERROR;
      }
      break;
  }
  
  // For system states, use synchronized blinking on all LEDs
  if (millis() - lastLedToggle > interval) {
    ledState = !ledState;
    setLEDState(ledState ? HIGH : LOW);
    lastLedToggle = millis();
  }
}

void setupWiFiAP() {
  LOG_INFO("Setting up WiFi Access Point...");
  currentState = STATE_AP_MODE;
  
  WiFi.mode(WIFI_AP);
  LOG_DEBUGF("WiFi mode set to AP: %s", AP_SSID);
  
  // Configure AP with standard ESP32 default IP configuration
  // This ensures DHCP server works properly for connecting clients
  IPAddress local_IP(192, 168, 4, 1);      // Gateway IP (ESP32 default)
  IPAddress gateway(192, 168, 4, 1);       // Gateway IP  
  IPAddress subnet(255, 255, 255, 0);      // Subnet mask for /24
  
  LOG_DEBUGF("Configuring AP IP: %s", local_IP.toString().c_str());
  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
    LOG_ERROR("Failed to configure AP IP settings");
  }
  
  if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    LOG_ERROR("Failed to start Access Point");
    currentState = STATE_ERROR;
    return;
  }
  
  IPAddress IP = WiFi.softAPIP();
  LOG_INFOF("Access Point started successfully");
  LOG_INFOF("SSID: %s", AP_SSID);
  LOG_INFOF("Password: %s", AP_PASSWORD);
  LOG_INFOF("IP address: %s", IP.toString().c_str());
  LOG_INFO("Connect to this network and navigate to http://" + IP.toString());
}

void setupWiFiSTA() {
  LOG_INFOF("Connecting to WiFi network: %s", wifiSSID.c_str());
  currentState = STATE_CONNECTING_WIFI;
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  
  LOG_DEBUG("Waiting for WiFi connection...");
  // Wait for connection with timeout
  unsigned long startTime = millis();
  int dotCount = 0;
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
    delay(500);
    if (++dotCount % 10 == 0) {
      LOG_DEBUGF("Still connecting... (%d seconds)", (millis() - startTime) / 1000);
    }
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    LOG_ERRORF("Failed to connect to WiFi after 30 seconds. Status: %d", WiFi.status());
    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL:
        LOG_ERROR("Network not found - check SSID");
        break;
      case WL_CONNECT_FAILED:
        LOG_ERROR("Connection failed - check password");
        break;
      case WL_CONNECTION_LOST:
        LOG_ERROR("Connection lost");
        break;
      default:
        LOG_ERRORF("Unknown WiFi error: %d", WiFi.status());
    }
    LOG_INFO("Falling back to AP mode");
    setupWiFiAP();
  } else {
    LOG_INFO("WiFi connection successful!");
    LOG_INFOF("Connected to: %s", WiFi.SSID().c_str());
    LOG_INFOF("IP address: %s", WiFi.localIP().toString().c_str());
    LOG_INFOF("Gateway: %s", WiFi.gatewayIP().toString().c_str());
    LOG_INFOF("DNS: %s", WiFi.dnsIP().toString().c_str());
    LOG_INFOF("Signal strength: %d dBm", WiFi.RSSI());
  }
}

void setupWebServer() {
  LOG_DEBUG("Configuring web server routes");
  
  // Serve static files
  server.on("/", [](){
    LOG_DEBUG("Serving root page");
    handleRoot();
  });
  
  server.on("/config", [](){
    LOG_DEBUG("Serving config page");
    handleConfig();
  });
  
  server.on("/save", HTTP_POST, [](){
    LOG_INFO("Processing configuration save request");
    handleSave();
  });
  
  server.on("/status", [](){
    LOG_DEBUG("Serving status API request");
    handleStatus();
  });
  
  server.on("/update", HTTP_POST, [](){
    LOG_INFO("Processing firmware update request");
    handleUpdate();
  });
  
  server.on("/login", [](){
    LOG_INFO("Processing OAuth login request");
    handleLogin();
  });
  
  server.on("/callback", [](){
    LOG_INFO("OAuth callback accessed - redirecting to device code flow");
    server.send(200, "text/html", R"(
<!DOCTYPE html>
<html>
<head>
    <title>Authentication Method Changed</title>
    <meta http-equiv="refresh" content="3;url=/">
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
        .message { background-color: #d1ecf1; color: #0c5460; padding: 20px; border-radius: 5px; display: inline-block; }
    </style>
</head>
<body>
    <div class="message">
        <h2>ℹ️ Authentication Method Updated</h2>
        <p>This device now uses Device Code Flow for improved security.</p>
        <p>No redirect URLs required! You will be redirected to the home page.</p>
    </div>
</body>
</html>
    )");
  });
  
  server.on("/restart", HTTP_POST, [](){
    LOG_WARN("Device restart requested via web interface");
    server.send(200, "text/plain", "Restarting...");
    delay(1000);
    ESP.restart();
  });
  
  server.begin();
  LOG_INFOF("Web server started on port %d", HTTP_PORT);
  
  if (currentState == STATE_AP_MODE) {
    LOG_INFO("Access configuration at: http://192.168.4.1");
  } else {
    LOG_INFOF("Access configuration at: http://%s", WiFi.localIP().toString().c_str());
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html lang=\"en\">";
  html += "<head>";
  html += "<meta charset=\"UTF-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<title>Teams Red Light</title>";
  html += "<style>";
  html += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; line-height: 1.6; color: #333; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; padding: 1rem; display: flex; align-items: center; justify-content: center; }";
  html += ".container { background: white; border-radius: 12px; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1); width: 100%; max-width: 500px; overflow: hidden; }";
  html += ".header { background: linear-gradient(135deg, #d73502, #b12d02); color: white; padding: 2rem; text-align: center; }";
  html += ".header h1 { font-size: 1.8rem; font-weight: 600; margin-bottom: 0.5rem; }";
  html += ".content { padding: 2rem; }";
  html += ".status-card { background: #f8f9fa; border-radius: 8px; padding: 1.5rem; margin-bottom: 2rem; text-align: center; border-left: 4px solid #6c757d; transition: all 0.3s ease; }";
  html += ".status-card.connected { border-left-color: #28a745; background: #d4edda; color: #155724; }";
  html += ".status-card.disconnected { border-left-color: #dc3545; background: #f8d7da; color: #721c24; }";
  html += ".status-card.configuring { border-left-color: #ffc107; background: #fff3cd; color: #856404; }";
  html += ".status-icon { font-size: 2rem; margin-bottom: 0.5rem; display: block; }";
  html += ".status-text { font-weight: 600; font-size: 1.1rem; margin-bottom: 0.5rem; }";
  html += ".status-detail { font-size: 0.9rem; opacity: 0.8; }";
  html += ".actions { display: grid; gap: 0.75rem; margin-top: 1.5rem; }";
  html += ".btn { background: #d73502; color: white; border: none; padding: 0.75rem 1rem; border-radius: 6px; font-size: 1rem; cursor: pointer; transition: all 0.2s ease; text-decoration: none; display: inline-block; text-align: center; }";
  html += ".btn:hover { background: #b12d02; transform: translateY(-1px); }";
  html += ".btn-secondary { background: #6c757d; } .btn-secondary:hover { background: #5a6268; }";
  html += ".btn-danger { background: #dc3545; } .btn-danger:hover { background: #c82333; }";
  html += ".device-info { background: #f8f9fa; border-radius: 6px; padding: 1rem; margin-top: 1rem; font-size: 0.9rem; color: #6c757d; }";
  html += ".device-info div { margin-bottom: 0.25rem; }";
  html += "@media (max-width: 768px) { body { padding: 0.5rem; } .header { padding: 1.5rem; } .content { padding: 1.5rem; } }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class=\"container\">";
  html += "<div class=\"header\">";
  html += "<h1>&#x1F534; Teams Red Light</h1>";
  html += "<p>Device Control Panel</p>";
  html += "</div>";
  html += "<div class=\"content\">";
  html += "<div id=\"status\" class=\"status-card configuring\">";
  html += "<span class=\"status-icon\">&#x23F3;</span>";
  html += "<div class=\"status-text\">Loading Status...</div>";
  html += "<div class=\"status-detail\">Please wait while we check the device status</div>";
  html += "</div>";
  html += "<div class=\"actions\">";
  html += "<button class=\"btn\" onclick=\"window.location.href='/config'\">&#x2699;&#xFE0F; Configure Device</button>";
  html += "<button class=\"btn btn-secondary\" onclick=\"checkStatus()\">&#x1F504; Refresh Status</button>";
  html += "<button class=\"btn btn-danger\" onclick=\"restartDevice()\">&#x1F50C; Restart Device</button>";
  html += "</div>";
  html += "<div class=\"device-info\" id=\"deviceInfo\" style=\"display: none;\">";
  html += "<div><strong>Device Information:</strong></div>";
  html += "<div id=\"ipAddress\"></div>";
  html += "<div id=\"uptime\"></div>";
  html += "<div id=\"wifiStatus\"></div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  html += "<script>";
  html += "function checkStatus() {";
  html += "fetch('/status').then(response => response.json()).then(data => {";
  html += "const statusDiv = document.getElementById('status');";
  html += "const deviceInfo = document.getElementById('deviceInfo');";
  html += "if (data.state === 'monitoring') {";
  html += "statusDiv.className = 'status-card connected';";
  html += "statusDiv.innerHTML = '<span class=\"status-icon\">&#x2705;</span><div class=\"status-text\">Connected & Monitoring</div><div class=\"status-detail\">Teams presence: ' + (data.presence || 'Unknown') + '</div>';";
  html += "} else if (data.state === 'ap_mode') {";
  html += "statusDiv.className = 'status-card configuring';";
  html += "statusDiv.innerHTML = '<span class=\"status-icon\">&#x2699;&#xFE0F;</span><div class=\"status-text\">Configuration Mode</div><div class=\"status-detail\">Please configure WiFi and Teams settings</div>';";
  html += "} else if (data.state === 'connecting_wifi') {";
  html += "statusDiv.className = 'status-card configuring';";
  html += "statusDiv.innerHTML = '<span class=\"status-icon\">&#x1F4F6;</span><div class=\"status-text\">Connecting to WiFi</div><div class=\"status-detail\">Establishing network connection...</div>';";
  html += "} else if (data.state === 'connecting_oauth' || data.state === 'device_code_pending') {";
  html += "statusDiv.className = 'status-card configuring';";
  html += "statusDiv.innerHTML = '<span class=\"status-icon\">&#x1F510;</span><div class=\"status-text\">Waiting for Authentication</div><div class=\"status-detail\">Complete Microsoft Teams authentication</div>';";
  html += "} else {";
  html += "statusDiv.className = 'status-card disconnected';";
  html += "statusDiv.innerHTML = '<span class=\"status-icon\">&#x274C;</span><div class=\"status-text\">Disconnected</div><div class=\"status-detail\">' + (data.message || 'Not connected') + '</div>';";
  html += "}";
  html += "if (data.ip_address || data.uptime || data.wifi_connected !== undefined) {";
  html += "deviceInfo.style.display = 'block';";
  html += "document.getElementById('ipAddress').textContent = data.ip_address ? 'IP Address: ' + data.ip_address : '';";
  html += "document.getElementById('uptime').textContent = data.uptime ? 'Uptime: ' + Math.floor(data.uptime / 60) + ' minutes' : '';";
  html += "document.getElementById('wifiStatus').textContent = data.wifi_connected !== undefined ? 'WiFi: ' + (data.wifi_connected ? 'Connected' : 'Disconnected') : '';";
  html += "}";
  html += "}).catch(() => {";
  html += "const statusDiv = document.getElementById('status');";
  html += "statusDiv.className = 'status-card disconnected';";
  html += "statusDiv.innerHTML = '<span class=\"status-icon\">&#x26A0;&#xFE0F;</span><div class=\"status-text\">Connection Error</div><div class=\"status-detail\">Unable to get device status</div>';";
  html += "});";
  html += "}";
  html += "function restartDevice() {";
  html += "if (confirm('Are you sure you want to restart the device? This will temporarily interrupt monitoring.')) {";
  html += "fetch('/restart', { method: 'POST' }).then(() => {";
  html += "const statusDiv = document.getElementById('status');";
  html += "statusDiv.className = 'status-card configuring';";
  html += "statusDiv.innerHTML = '<span class=\"status-icon\">&#x1F504;</span><div class=\"status-text\">Restarting Device</div><div class=\"status-detail\">Please wait 30 seconds...</div>';";
  html += "setTimeout(() => location.reload(), 30000);";
  html += "}).catch(() => { alert('Failed to restart device. Please try again.'); });";
  html += "}";
  html += "}";
  html += "checkStatus(); setInterval(checkStatus, 10000);";
  html += "</script>";
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html);
}

void handleConfig() {
  String html = "<!DOCTYPE html>";
  html += "<html lang=\"en\">";
  html += "<head>";
  html += "<meta charset=\"UTF-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<title>Teams Red Light - Configuration</title>";
  html += "<style>";
  html += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  html += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; line-height: 1.6; color: #333; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; padding: 1rem; }";
  html += ".container { background: white; border-radius: 12px; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1); width: 100%; max-width: 700px; margin: 0 auto; overflow: hidden; }";
  html += ".header { background: linear-gradient(135deg, #d73502, #b12d02); color: white; padding: 2rem; text-align: center; }";
  html += ".header h1 { font-size: 1.8rem; font-weight: 600; margin-bottom: 0.5rem; }";
  html += ".content { padding: 2rem; }";
  html += ".section { background: #f8f9fa; border-radius: 8px; padding: 1.5rem; margin-bottom: 1.5rem; border-left: 4px solid #d73502; }";
  html += ".section h3 { color: #d73502; margin-bottom: 1rem; font-size: 1.2rem; }";
  html += ".form-group { margin-bottom: 1rem; }";
  html += "label { display: block; margin-bottom: 0.5rem; font-weight: 600; color: #333; }";
  html += "input[type=\"text\"], input[type=\"password\"], input[type=\"email\"], select { width: 100%; padding: 0.75rem; border: 2px solid #e9ecef; border-radius: 6px; font-size: 1rem; transition: border-color 0.2s ease; }";
  html += "input:focus, select:focus { outline: none; border-color: #d73502; }";
  html += ".help { font-size: 0.875rem; color: #6c757d; margin-top: 0.25rem; }";
  html += ".led-config { border: 1px solid #dee2e6; border-radius: 6px; padding: 1rem; margin: 1rem 0; background: #fff; }";
  html += ".led-config h4 { color: #495057; margin-bottom: 1rem; font-size: 1rem; border-bottom: 1px solid #dee2e6; padding-bottom: 0.5rem; }";
  html += ".actions { display: grid; grid-template-columns: 1fr 1fr; gap: 1rem; margin-top: 2rem; }";
  html += ".btn { background: #d73502; color: white; border: none; padding: 0.75rem 1.5rem; border-radius: 6px; font-size: 1rem; cursor: pointer; transition: all 0.2s ease; text-decoration: none; text-align: center; display: inline-block; }";
  html += ".btn:hover { background: #b12d02; transform: translateY(-1px); }";
  html += ".btn-secondary { background: #6c757d; color: white; } .btn-secondary:hover { background: #5a6268; }";
  html += ".btn-auth { background: #0078d4; margin-top: 1rem; width: 100%; } .btn-auth:hover { background: #106ebe; }";
  html += ".info-box { background: #e8f4fd; border: 1px solid #bee5eb; border-radius: 6px; padding: 1rem; margin-top: 1rem; }";
  html += ".info-box h4 { color: #0c5460; margin-bottom: 0.5rem; }";
  html += ".info-box ol { margin-left: 1.5rem; color: #0c5460; }";
  html += ".info-box li { margin-bottom: 0.5rem; }";
  html += "@media (max-width: 768px) { body { padding: 0.5rem; } .header { padding: 1.5rem; } .content { padding: 1.5rem; } .actions { grid-template-columns: 1fr; } }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class=\"container\">";
  html += "<div class=\"header\">";
  html += "<h1>&#x2699;&#xFE0F; Device Configuration</h1>";
  html += "<p>Configure your Teams Red Light device</p>";
  html += "</div>";
  html += "<div class=\"content\">";
  html += "<form action=\"/save\" method=\"POST\">";
  html += "<div class=\"section\">";
  html += "<h3>&#x1F4F6; WiFi Connection</h3>";
  html += "<div class=\"form-group\">";
  html += "<label for=\"wifi_ssid\">Network Name (SSID)</label>";
  html += "<input type=\"text\" id=\"wifi_ssid\" name=\"wifi_ssid\" value=\"" + wifiSSID + "\" required placeholder=\"Enter your WiFi network name\">";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label for=\"wifi_password\">WiFi Password</label>";
  html += "<input type=\"password\" id=\"wifi_password\" name=\"wifi_password\" value=\"\" placeholder=\"Enter WiFi password\">";
  html += "<div class=\"help\">&#x1F4A1; Leave blank to keep current password</div>";
  html += "</div>";
  html += "</div>";
  html += "<div class=\"section\">";
  html += "<h3>&#x1F510; Microsoft Teams Integration</h3>";
  html += "<div class=\"form-group\">";
  html += "<label for=\"user_email\">Your Email Address</label>";
  html += "<input type=\"email\" id=\"user_email\" name=\"user_email\" value=\"" + userEmail + "\" required placeholder=\"your.name@company.com\">";
  html += "<div class=\"help\">&#x1F4E7; The email address for your Teams account</div>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label for=\"tenant_id\">Tenant ID (Optional)</label>";
  html += "<input type=\"text\" id=\"tenant_id\" name=\"tenant_id\" value=\"" + tenantId + "\" placeholder=\"common\">";
  html += "<div class=\"help\">&#x1F3E2; Your Office 365 tenant ID (use 'common' for personal accounts)</div>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label for=\"client_id\">Application Client ID</label>";
  html += "<input type=\"text\" id=\"client_id\" name=\"client_id\" value=\"" + clientId + "\" required placeholder=\"12345678-1234-1234-1234-123456789012\">";
  html += "<div class=\"help\">&#x1F194; Azure AD Application Client ID</div>";
  html += "</div>";
  html += "<div class=\"form-group\">";
  html += "<label for=\"client_secret\">Application Client Secret</label>";
  html += "<input type=\"password\" id=\"client_secret\" name=\"client_secret\" value=\"\" placeholder=\"Enter client secret\">";
  html += "<div class=\"help\">&#x1F511; Azure AD Application Client Secret (leave blank to keep current)</div>";
  html += "</div>";
  html += "</div>";
  html += "<div class=\"section\">";
  html += "<h3>&#x1F504; Firmware Updates</h3>";
  html += "<div class=\"form-group\">";
  html += "<label for=\"ota_url\">Update URL</label>";
  html += "<input type=\"text\" id=\"ota_url\" name=\"ota_url\" value=\"" + preferences.getString(OTA_UPDATE_URL_KEY, DEFAULT_OTA_URL) + "\" placeholder=\"https://github.com/...\">";
  html += "<div class=\"help\">&#x1F310; URL for over-the-air firmware updates</div>";
  html += "</div>";
  html += "</div>";
  html += "<div class=\"section\">";
  html += "<h3>&#x1F4A1; LED Pattern Settings</h3>";
  html += "<div class=\"form-group\">";
  html += "<label for=\"led_count\">Number of LEDs</label>";
  html += "<select id=\"led_count\" name=\"led_count\" onchange=\"updateLEDFields()\">";
  for (uint8_t i = 1; i <= MAX_LEDS; i++) {
    html += "<option value=\"" + String(i) + "\"" + String(ledCount == i ? " selected" : "") + ">" + String(i) + " LED" + (i > 1 ? "s" : "") + "</option>";
  }
  html += "</select>";
  html += "<div class=\"help\">&#x1F4A1; Select the number of LEDs to configure</div>";
  html += "</div>";
  
  html += "<div id=\"led_configurations\">";
  for (uint8_t i = 0; i < MAX_LEDS; i++) {
    html += "<div class=\"led-config\" id=\"led_config_" + String(i) + "\" style=\"" + String(i >= ledCount ? "display:none;" : "") + "\">";
    html += "<h4>LED " + String(i + 1) + " Configuration</h4>";
    
    html += "<div class=\"form-group\">";
    html += "<label for=\"led_pin_" + String(i) + "\">GPIO Pin</label>";
    html += "<select id=\"led_pin_" + String(i) + "\" name=\"led_pin_" + String(i) + "\">";
    for (uint8_t j = 0; j < availableGPIOCount; j++) {
      uint8_t pin = availableGPIOPins[j];
      uint8_t selectedPin = (i < ledCount) ? leds[i].pin : LED_PIN;
      html += "<option value=\"" + String(pin) + "\"" + String(selectedPin == pin ? " selected" : "") + ">GPIO " + String(pin) + "</option>";
    }
    html += "</select>";
    html += "<div class=\"help\">&#x1F50C; Select GPIO pin for this LED</div>";
    html += "</div>";
    
    html += "<div class=\"form-group\">";
    html += "<label for=\"led_call_" + String(i) + "\">Call Pattern</label>";
    html += "<select id=\"led_call_" + String(i) + "\" name=\"led_call_" + String(i) + "\">";
    LEDPattern callPat = (i < ledCount) ? leds[i].callPattern : DEFAULT_CALL_PATTERN;
    html += "<option value=\"0\"" + String(callPat == 0 ? " selected" : "") + ">Off</option>";
    html += "<option value=\"1\"" + String(callPat == 1 ? " selected" : "") + ">Solid</option>";
    html += "<option value=\"2\"" + String(callPat == 2 ? " selected" : "") + ">Slow Blink (1s)</option>";
    html += "<option value=\"3\"" + String(callPat == 3 ? " selected" : "") + ">Medium Blink (0.5s)</option>";
    html += "<option value=\"4\"" + String(callPat == 4 ? " selected" : "") + ">Fast Blink (0.2s) (Default)</option>";
    html += "<option value=\"5\"" + String(callPat == 5 ? " selected" : "") + ">Double Blink</option>";
    html += "<option value=\"6\"" + String(callPat == 6 ? " selected" : "") + ">Dim Solid</option>";
    html += "</select>";
    html += "<div class=\"help\">&#x1F4F5; Pattern during calls (busy status)</div>";
    html += "</div>";
    
    html += "<div class=\"form-group\">";
    html += "<label for=\"led_meeting_" + String(i) + "\">Meeting Pattern</label>";
    html += "<select id=\"led_meeting_" + String(i) + "\" name=\"led_meeting_" + String(i) + "\">";
    LEDPattern meetPat = (i < ledCount) ? leds[i].meetingPattern : DEFAULT_MEETING_PATTERN;
    html += "<option value=\"0\"" + String(meetPat == 0 ? " selected" : "") + ">Off</option>";
    html += "<option value=\"1\"" + String(meetPat == 1 ? " selected" : "") + ">Solid (Default)</option>";
    html += "<option value=\"2\"" + String(meetPat == 2 ? " selected" : "") + ">Slow Blink (1s)</option>";
    html += "<option value=\"3\"" + String(meetPat == 3 ? " selected" : "") + ">Medium Blink (0.5s)</option>";
    html += "<option value=\"4\"" + String(meetPat == 4 ? " selected" : "") + ">Fast Blink (0.2s)</option>";
    html += "<option value=\"5\"" + String(meetPat == 5 ? " selected" : "") + ">Double Blink</option>";
    html += "<option value=\"6\"" + String(meetPat == 6 ? " selected" : "") + ">Dim Solid</option>";
    html += "</select>";
    html += "<div class=\"help\">&#x1F4C5; Pattern during meetings</div>";
    html += "</div>";
    
    html += "<div class=\"form-group\">";
    html += "<label for=\"led_available_" + String(i) + "\">Available Pattern</label>";
    html += "<select id=\"led_available_" + String(i) + "\" name=\"led_available_" + String(i) + "\">";
    LEDPattern availPat = (i < ledCount) ? leds[i].availablePattern : DEFAULT_AVAILABLE_PATTERN;
    html += "<option value=\"0\"" + String(availPat == 0 ? " selected" : "") + ">Off (Default)</option>";
    html += "<option value=\"1\"" + String(availPat == 1 ? " selected" : "") + ">Solid</option>";
    html += "<option value=\"2\"" + String(availPat == 2 ? " selected" : "") + ">Slow Blink (1s)</option>";
    html += "<option value=\"3\"" + String(availPat == 3 ? " selected" : "") + ">Medium Blink (0.5s)</option>";
    html += "<option value=\"4\"" + String(availPat == 4 ? " selected" : "") + ">Fast Blink (0.2s)</option>";
    html += "<option value=\"5\"" + String(availPat == 5 ? " selected" : "") + ">Double Blink</option>";
    html += "<option value=\"6\"" + String(availPat == 6 ? " selected" : "") + ">Dim Solid</option>";
    html += "</select>";
    html += "<div class=\"help\">&#x1F7E2; Pattern when available</div>";
    html += "</div>";
    html += "</div>";
  }
  html += "</div>";
  
  html += "<script>";
  html += "function updateLEDFields() {";
  html += "  var count = parseInt(document.getElementById('led_count').value);";
  html += "  for (var i = 0; i < " + String(MAX_LEDS) + "; i++) {";
  html += "    var element = document.getElementById('led_config_' + i);";
  html += "    if (element) {";
  html += "      element.style.display = i < count ? 'block' : 'none';";
  html += "    }";
  html += "  }";
  html += "}";
  html += "</script>";
  html += "</div>";
  html += "<div class=\"actions\">";
  html += "<button type=\"submit\" class=\"btn\">&#x1F4BE; Save Configuration</button>";
  html += "<button type=\"button\" class=\"btn btn-secondary\" onclick=\"window.location.href='/'\">&#x2190; Back to Home</button>";
  html += "</div>";
  html += "</form>";
  html += "<div class=\"section\">";
  html += "<h3>&#x1F510; Microsoft Authentication</h3>";
  html += "<p>After saving your configuration, authenticate with Microsoft to enable Teams presence monitoring.</p>";
  html += "<button type=\"button\" class=\"btn btn-auth\" onclick=\"window.location.href='/login'\">&#x1F680; Authenticate with Microsoft</button>";
  html += "<div class=\"info-box\">";
  html += "<h4>&#x2705; Secure Device Code Flow</h4>";
  html += "<p>This device uses Microsoft's secure Device Code Flow - no redirect URLs or SSL certificates needed!</p>";
  html += "</div>";
  html += "</div>";
  html += "<div class=\"section\">";
  html += "<h3>&#x1F4CB; Setup Guide</h3>";
  html += "<div class=\"info-box\">";
  html += "<h4>Azure AD Application Setup:</h4>";
  html += "<ol>";
  html += "<li>Go to <strong>Azure Portal</strong> &#x2192; Azure Active Directory &#x2192; App registrations</li>";
  html += "<li>Click <strong>\"New registration\"</strong></li>";
  html += "<li>Enter name: <strong>\"Teams Red Light\"</strong></li>";
  html += "<li>Leave redirect URI <strong>blank</strong> (not needed!)</li>";
  html += "<li>Add API permission: <strong>Microsoft Graph &#x2192; Presence.Read</strong></li>";
  html += "<li>Create a <strong>client secret</strong></li>";
  html += "<li>Copy the <strong>Client ID</strong> and <strong>Client Secret</strong> above</li>";
  html += "</ol>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html);
}


void handleSave() {
  LOG_INFO("Processing configuration save request");
  bool configChanged = false;
  
  // Save WiFi configuration
  if (server.hasArg("wifi_ssid")) {
    String newSSID = server.arg("wifi_ssid");
    if (newSSID != wifiSSID) {
      LOG_INFOF("WiFi SSID changed from '%s' to '%s'", wifiSSID.c_str(), newSSID.c_str());
      wifiSSID = newSSID;
      preferences.putString(KEY_WIFI_SSID, wifiSSID);
      configChanged = true;
    }
  }
  
  if (server.hasArg("wifi_password") && server.arg("wifi_password").length() > 0) {
    wifiPassword = server.arg("wifi_password");
    preferences.putString(KEY_WIFI_PASS, wifiPassword);
    LOG_INFO("WiFi password updated");
    configChanged = true;
  }
  
  // Save OAuth configuration
  if (server.hasArg("user_email")) {
    String newEmail = server.arg("user_email");
    if (newEmail != userEmail) {
      LOG_INFOF("User email changed from '%s' to '%s'", userEmail.c_str(), newEmail.c_str());
      userEmail = newEmail;
      preferences.putString(KEY_USER_EMAIL, userEmail);
      configChanged = true;
    }
  }
  
  if (server.hasArg("tenant_id")) {
    String newTenantId = server.arg("tenant_id");
    if (newTenantId.length() == 0) newTenantId = "common";
    if (newTenantId != tenantId) {
      LOG_INFOF("Tenant ID changed from '%s' to '%s'", tenantId.c_str(), newTenantId.c_str());
      tenantId = newTenantId;
      preferences.putString(KEY_TENANT_ID, tenantId);
      configChanged = true;
    }
  }
  
  if (server.hasArg("client_id")) {
    String newClientId = server.arg("client_id");
    if (newClientId != clientId) {
      LOG_INFOF("Client ID changed: %s", newClientId.length() > 0 ? "set" : "cleared");
      clientId = newClientId;
      preferences.putString(KEY_CLIENT_ID, clientId);
      configChanged = true;
    }
  }
  
  if (server.hasArg("client_secret") && server.arg("client_secret").length() > 0) {
    clientSecret = server.arg("client_secret");
    preferences.putString(KEY_CLIENT_SECRET, clientSecret);
    LOG_INFO("Client secret updated");
    configChanged = true;
  }
  
  if (server.hasArg("ota_url")) {
    String otaUrl = server.arg("ota_url");
    String currentOtaUrl = preferences.getString(OTA_UPDATE_URL_KEY, DEFAULT_OTA_URL);
    if (otaUrl != currentOtaUrl) {
      LOG_INFOF("OTA URL changed to: %s", otaUrl.c_str());
      preferences.putString(OTA_UPDATE_URL_KEY, otaUrl);
      configChanged = true;
    }
  }
  
  // Save LED pattern configuration
  if (server.hasArg("led_count")) {
    uint8_t newLedCount = server.arg("led_count").toInt();
    if (newLedCount > MAX_LEDS) newLedCount = MAX_LEDS;
    if (newLedCount < 1) newLedCount = 1;
    
    if (newLedCount != ledCount) {
      LOG_INFOF("LED count changed from %d to %d", ledCount, newLedCount);
      ledCount = newLedCount;
      configChanged = true;
    }
    
    // Save individual LED configurations
    for (uint8_t i = 0; i < ledCount; i++) {
      String pinArg = "led_pin_" + String(i);
      String callArg = "led_call_" + String(i);
      String meetArg = "led_meeting_" + String(i);
      String availArg = "led_available_" + String(i);
      
      if (server.hasArg(pinArg)) {
        uint8_t newPin = server.arg(pinArg).toInt();
        if (i >= MAX_LEDS) continue; // Safety check
        
        if (i < MAX_LEDS && (i >= ledCount || leds[i].pin != newPin)) {
          if (i < ledCount) {
            LOG_INFOF("LED %d pin changed from %d to %d", i, leds[i].pin, newPin);
          }
          leds[i].pin = newPin;
          leds[i].enabled = true;
          configChanged = true;
        }
      }
      
      if (server.hasArg(callArg)) {
        LEDPattern newPattern = (LEDPattern)server.arg(callArg).toInt();
        if (i < MAX_LEDS && (i >= ledCount || leds[i].callPattern != newPattern)) {
          if (i < ledCount) {
            LOG_INFOF("LED %d call pattern changed from %d to %d", i, leds[i].callPattern, newPattern);
          }
          leds[i].callPattern = newPattern;
          configChanged = true;
        }
      }
      
      if (server.hasArg(meetArg)) {
        LEDPattern newPattern = (LEDPattern)server.arg(meetArg).toInt();
        if (i < MAX_LEDS && (i >= ledCount || leds[i].meetingPattern != newPattern)) {
          if (i < ledCount) {
            LOG_INFOF("LED %d meeting pattern changed from %d to %d", i, leds[i].meetingPattern, newPattern);
          }
          leds[i].meetingPattern = newPattern;
          configChanged = true;
        }
      }
      
      if (server.hasArg(availArg)) {
        LEDPattern newPattern = (LEDPattern)server.arg(availArg).toInt();
        if (i < MAX_LEDS && (i >= ledCount || leds[i].availablePattern != newPattern)) {
          if (i < ledCount) {
            LOG_INFOF("LED %d available pattern changed from %d to %d", i, leds[i].availablePattern, newPattern);
          }
          leds[i].availablePattern = newPattern;
          configChanged = true;
        }
      }
      
      // Initialize LED state variables
      if (i < MAX_LEDS) {
        leds[i].lastToggle = 0;
        leds[i].state = false;
        leds[i].doubleBlinksStartTime = 0;
        leds[i].doubleBlinksState = false;
        leds[i].doubleBlinksCount = 0;
      }
    }
    
    // Update legacy patterns for backward compatibility (use first LED)
    if (ledCount > 0) {
      callPattern = leds[0].callPattern;
      meetingPattern = leds[0].meetingPattern;
      availablePattern = leds[0].availablePattern;
    }
  } else {
    // Legacy LED pattern handling for backward compatibility
    if (server.hasArg("meeting_pattern")) {
      LEDPattern newMeetingPattern = (LEDPattern)server.arg("meeting_pattern").toInt();
      if (newMeetingPattern != meetingPattern) {
        LOG_INFOF("Meeting LED pattern changed from %d to %d", meetingPattern, newMeetingPattern);
        meetingPattern = newMeetingPattern;
        preferences.putUInt(KEY_MEETING_PATTERN, meetingPattern);
        configChanged = true;
      }
    }
    
    if (server.hasArg("no_meeting_pattern")) {
      LEDPattern newNoMeetingPattern = (LEDPattern)server.arg("no_meeting_pattern").toInt();
      if (newNoMeetingPattern != availablePattern) {
        LOG_INFOF("No meeting LED pattern changed from %d to %d", availablePattern, newNoMeetingPattern);
        availablePattern = newNoMeetingPattern;
        preferences.putUInt(KEY_NO_MEETING_PATTERN, availablePattern);
        configChanged = true;
      }
    }
  }
  
  if (configChanged) {
    LOG_INFO("Configuration changes saved to flash memory");
  } else {
    LOG_INFO("No configuration changes detected");
  }
  
  server.send(200, "text/html", R"(
<!DOCTYPE html>
<html>
<head>
    <title>Configuration Saved</title>
    <meta http-equiv="refresh" content="3;url=/">
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
        .message { background-color: #d4edda; color: #155724; padding: 20px; border-radius: 5px; display: inline-block; }
    </style>
</head>
<body>
    <div class="message">
        <h2>✅ Configuration Saved!</h2>
        <p>The device will restart and connect to the new network...</p>
        <p>You will be redirected to the home page in 3 seconds.</p>
    </div>
</body>
</html>
  )");
  
  // Restart the device after a short delay
  LOG_WARN("Restarting device to apply new configuration...");
  delay(1000);
  ESP.restart();
}

const char* getPatternName(LEDPattern pattern) {
  switch (pattern) {
    case PATTERN_OFF: return "Off";
    case PATTERN_SOLID: return "Solid";
    case PATTERN_SLOW_BLINK: return "Slow Blink";
    case PATTERN_MEDIUM_BLINK: return "Medium Blink";
    case PATTERN_FAST_BLINK: return "Fast Blink";
    case PATTERN_DOUBLE_BLINK: return "Double Blink";
    case PATTERN_DIM_SOLID: return "Dim Solid";
    default: return "Unknown";
  }
}

void handleStatus() {
  DynamicJsonDocument doc(1024);
  
  switch (currentState) {
    case STATE_AP_MODE:
      doc["state"] = "ap_mode";
      doc["message"] = "Configuration mode - Please configure WiFi";
      break;
    case STATE_CONNECTING_WIFI:
      doc["state"] = "connecting_wifi";
      doc["message"] = "Connecting to WiFi";
      break;
    case STATE_CONNECTING_OAUTH:
      doc["state"] = "connecting_oauth";
      doc["message"] = "Waiting for OAuth authentication";
      break;
    case STATE_DEVICE_CODE_PENDING:
      doc["state"] = "device_code_pending";
      doc["message"] = "Waiting for device code authentication";
      if (userCode.length() > 0) {
        doc["user_code"] = userCode;
        doc["verification_uri"] = verificationUri;
        long timeRemaining = (long)(deviceCodeExpires - millis()) / 1000;
        if (timeRemaining > 0) {
          doc["expires_in"] = timeRemaining;
        } else {
          doc["expired"] = true;
        }
      }
      break;
    case STATE_AUTHENTICATED:
      doc["state"] = "authenticated";
      doc["message"] = "Authenticated, starting monitoring";
      break;
    case STATE_MONITORING:
      doc["state"] = "monitoring";
      doc["message"] = "Monitoring Teams presence";
      switch (currentPresence) {
        case PRESENCE_AVAILABLE:
          doc["presence"] = "Available";
          break;
        case PRESENCE_BUSY:
          doc["presence"] = "Busy";
          break;
        case PRESENCE_IN_MEETING:
          doc["presence"] = "In Meeting";
          break;
        case PRESENCE_AWAY:
          doc["presence"] = "Away";
          break;
        case PRESENCE_OFFLINE:
          doc["presence"] = "Offline";
          break;
        default:
          doc["presence"] = "Unknown";
          break;
      }
      break;
    case STATE_ERROR:
      doc["state"] = "error";
      doc["message"] = "Error occurred";
      break;
  }
  
  doc["wifi_connected"] = WiFi.status() == WL_CONNECTED;
  if (WiFi.status() == WL_CONNECTED) {
    doc["ip_address"] = WiFi.localIP().toString();
  }
  doc["has_token"] = accessToken.length() > 0;
  doc["uptime"] = millis() / 1000;
  
  // Add LED status information
  doc["led_count"] = ledCount;
  JsonArray ledArray = doc.createNestedArray("leds");
  for (uint8_t i = 0; i < ledCount; i++) {
    JsonObject led = ledArray.createNestedObject();
    led["id"] = i;
    led["pin"] = leds[i].pin;
    led["enabled"] = leds[i].enabled;
    led["current_state"] = leds[i].state;
    
    // Add pattern names for readability
    led["call_pattern"] = getPatternName(leds[i].callPattern);
    led["meeting_pattern"] = getPatternName(leds[i].meetingPattern);
    led["available_pattern"] = getPatternName(leds[i].availablePattern);
  }
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleUpdate() {
  LOG_INFO("Firmware update request received");
  // OTA Update functionality - simplified for now
  LOG_WARN("OTA Update not implemented in this version");
  server.send(200, "text/plain", "OTA Update not implemented in this version");
}

void handleLogin() {
  LOG_INFO("Device code authentication request received");
  
  if (clientId.length() == 0 || tenantId.length() == 0) {
    LOG_ERROR("Authentication failed - missing client ID or tenant ID");
    server.send(400, "text/plain", "Client ID and Tenant ID must be configured first");
    return;
  }
  
  // Start device code flow
  if (startDeviceCodeFlow()) {
    LOG_INFO("Device code flow started successfully");
    
    // Display the user code and verification URL to the user
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Teams Red Light - Device Authentication</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta http-equiv="refresh" content="10;url=/status">
    <style>
        body { 
            font-family: Arial, sans-serif; 
            text-align: center; 
            margin: 20px; 
            background-color: #f5f5f5;
        }
        .container { 
            max-width: 600px; 
            margin: 0 auto; 
            background: white; 
            padding: 30px; 
            border-radius: 10px; 
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        .user-code { 
            font-size: 2em; 
            font-weight: bold; 
            color: #0078d4; 
            margin: 20px 0; 
            padding: 15px; 
            background: #f0f8ff; 
            border-radius: 5px;
            letter-spacing: 3px;
        }
        .instructions {
            margin: 20px 0;
            line-height: 1.6;
        }
        .verification-url {
            background: #e8f4f8;
            padding: 10px;
            border-radius: 5px;
            margin: 15px 0;
            word-break: break-all;
        }
        .status {
            margin-top: 20px;
            padding: 10px;
            background: #fff3cd;
            border-radius: 5px;
            color: #856404;
        }
        .button {
            background: #0078d4;
            color: white;
            padding: 10px 20px;
            text-decoration: none;
            border-radius: 5px;
            margin: 10px;
            display: inline-block;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔴 Teams Red Light Authentication</h1>
        
        <div class="instructions">
            <h2>Step 1: Visit the Microsoft login page</h2>
            <div class="verification-url">
                <strong>Go to:</strong> <a href=")" + verificationUri + R"(" target="_blank">)" + verificationUri + R"(</a>
            </div>
            
            <h2>Step 2: Enter this code</h2>
            <div class="user-code">)" + userCode + R"(</div>
            
            <h2>Step 3: Sign in with your Teams account</h2>
            <p>After entering the code, sign in with your Microsoft Teams/Office 365 account and authorize the application.</p>
        </div>
        
        <div class="status">
            <strong>Waiting for authentication...</strong><br>
            This page will refresh automatically. You can also <a href="/status">check status</a> manually.
        </div>
        
        <div style="margin-top: 30px;">
            <a href=")" + verificationUri + R"(" target="_blank" class="button">Open Microsoft Login</a>
            <a href="/status" class="button">Check Status</a>
        </div>
    </div>
</body>
</html>
    )";
    
    server.send(200, "text/html", html);
  } else {
    LOG_ERROR("Failed to start device code flow");
    server.send(500, "text/plain", "Failed to start authentication process. Please try again.");
  }
}

void handleCallback() {
  LOG_INFO("OAuth callback received");
  
  if (server.hasArg("code")) {
    String code = server.arg("code");
    LOG_INFOF("Authorization code received (length: %d)", code.length());
    
    // Exchange code for tokens
    HTTPClient http;
    String tokenUrl = "https://login.microsoftonline.com/" + tenantId + "/oauth2/v2.0/token";
    LOG_DEBUGF("Making token exchange request to: %s", tokenUrl.c_str());
    
    http.begin(tokenUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    String deviceIP = WiFi.localIP().toString();
    String redirectUri = "http://" + deviceIP + "/callback";
    
    String postData = "client_id=" + clientId;
    postData += "&client_secret=" + clientSecret;
    postData += "&code=" + code;
    postData += "&grant_type=authorization_code";
    postData += "&redirect_uri=" + redirectUri;
    
    LOG_DEBUG("Sending token exchange request...");
    int httpCode = http.POST(postData);
    LOG_INFOF("Token exchange response: HTTP %d", httpCode);
    
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      LOG_DEBUGF("Token response payload length: %d", payload.length());
      
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, payload);
      
      if (error) {
        LOG_ERRORF("Failed to parse token response JSON: %s", error.c_str());
        server.send(400, "text/plain", "Authentication failed: Invalid JSON response");
      } else if (doc.containsKey("access_token")) {
        accessToken = doc["access_token"].as<String>();
        refreshToken = doc["refresh_token"].as<String>();
        unsigned long expiresIn = doc["expires_in"].as<unsigned long>();
        tokenExpires = millis() + (expiresIn * 1000);
        
        LOG_INFO("OAuth authentication successful!");
        LOG_INFOF("Access token length: %d", accessToken.length());
        LOG_INFOF("Refresh token length: %d", refreshToken.length());
        LOG_INFOF("Token expires in: %lu seconds", expiresIn);
        
        preferences.putString(KEY_ACCESS_TOKEN, accessToken);
        preferences.putString(KEY_REFRESH_TOKEN, refreshToken);
        preferences.putULong64(KEY_TOKEN_EXPIRES, tokenExpires);
        
        currentState = STATE_AUTHENTICATED;
        LOG_INFO("Device state changed to AUTHENTICATED");
        
        server.send(200, "text/html", R"(
<!DOCTYPE html>
<html>
<head>
    <title>Authentication Successful</title>
    <meta http-equiv="refresh" content="3;url=/">
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
        .message { background-color: #d4edda; color: #155724; padding: 20px; border-radius: 5px; display: inline-block; }
    </style>
</head>
<body>
    <div class="message">
        <h2>✅ Authentication Successful!</h2>
        <p>Teams Red Light is now monitoring your presence.</p>
        <p>You will be redirected to the home page in 3 seconds.</p>
    </div>
</body>
</html>
        )");
      } else {
        LOG_ERROR("Authentication failed: No access token in response");
        if (doc.containsKey("error")) {
          String error = doc["error"];
          String errorDesc = doc.containsKey("error_description") ? doc["error_description"].as<String>() : "";
          LOG_ERRORF("OAuth error: %s - %s", error.c_str(), errorDesc.c_str());
        }
        server.send(400, "text/plain", "Authentication failed: No access token received");
      }
    } else {
      LOG_ERRORF("Token exchange failed with HTTP %d", httpCode);
      String response = http.getString();
      if (response.length() > 0) {
        LOG_DEBUGF("Error response: %s", response.c_str());
      }
      server.send(400, "text/plain", "Authentication failed: HTTP " + String(httpCode));
    }
    
    http.end();
  } else if (server.hasArg("error")) {
    String error = server.arg("error");
    String errorDesc = server.hasArg("error_description") ? server.arg("error_description") : "";
    LOG_ERRORF("OAuth authentication error: %s - %s", error.c_str(), errorDesc.c_str());
    server.send(400, "text/plain", "Authentication failed: " + error);
  } else {
    LOG_ERROR("OAuth callback received without authorization code or error");
    server.send(400, "text/plain", "Authentication failed: No authorization code received");
  }
}

bool startDeviceCodeFlow() {
  LOG_INFO("Starting device code flow");
  
  HTTPClient http;
  String deviceCodeUrl = "https://login.microsoftonline.com/" + tenantId + "/oauth2/v2.0/devicecode";
  
  http.begin(deviceCodeUrl);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  String postData = "client_id=" + clientId;
  postData += "&scope=" + String(DEVICE_CODE_SCOPE);
  
  LOG_DEBUGF("Device code request URL: %s", deviceCodeUrl.c_str());
  LOG_DEBUG("Sending device code request...");
  
  int httpCode = http.POST(postData);
  LOG_INFOF("Device code response: HTTP %d", httpCode);
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    LOG_DEBUGF("Device code response payload length: %d", payload.length());
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      LOG_ERRORF("Failed to parse device code response JSON: %s", error.c_str());
      http.end();
      return false;
    }
    
    deviceCode = doc["device_code"].as<String>();
    userCode = doc["user_code"].as<String>();
    verificationUri = doc["verification_uri"].as<String>();
    unsigned long expiresIn = doc["expires_in"].as<unsigned long>();
    deviceCodeExpires = millis() + (expiresIn * 1000);
    
    LOG_INFO("Device code flow initiated successfully");
    LOG_INFOF("User code: %s", userCode.c_str());
    LOG_INFOF("Verification URI: %s", verificationUri.c_str());
    LOG_INFOF("Device code expires in: %lu seconds", expiresIn);
    
    // Save device code data
    preferences.putString(KEY_DEVICE_CODE, deviceCode);
    preferences.putString(KEY_USER_CODE, userCode);
    preferences.putString(KEY_VERIFICATION_URI, verificationUri);
    preferences.putULong64(KEY_DEVICE_CODE_EXPIRES, deviceCodeExpires);
    
    currentState = STATE_DEVICE_CODE_PENDING;
    lastDeviceCodePoll = millis();
    
    http.end();
    return true;
  } else {
    LOG_ERRORF("Device code request failed with HTTP %d", httpCode);
    String response = http.getString();
    if (response.length() > 0) {
      LOG_DEBUGF("Error response: %s", response.c_str());
    }
    http.end();
    return false;
  }
}

bool pollDeviceCodeToken() {
  if (deviceCode.length() == 0) {
    LOG_ERROR("No device code available for polling");
    return false;
  }
  
  LOG_DEBUG("Polling for device code token");
  
  HTTPClient http;
  String tokenUrl = "https://login.microsoftonline.com/" + tenantId + "/oauth2/v2.0/token";
  
  http.begin(tokenUrl);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  String postData = "grant_type=urn:ietf:params:oauth:grant-type:device_code";
  postData += "&client_id=" + clientId;
  postData += "&client_secret=" + clientSecret;
  postData += "&device_code=" + deviceCode;
  
  int httpCode = http.POST(postData);
  LOG_DEBUGF("Token poll response: HTTP %d", httpCode);
  
  if (httpCode == HTTP_CODE_OK || httpCode == 400 || httpCode == 401) {
    String payload = http.getString();
    LOG_DEBUGF("Token response payload length: %d", payload.length());
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      LOG_ERRORF("Failed to parse token response JSON: %s", error.c_str());
      http.end();
      return false;
    }
    
    if (doc.containsKey("access_token")) {
      accessToken = doc["access_token"].as<String>();
      refreshToken = doc["refresh_token"].as<String>();
      unsigned long expiresIn = doc["expires_in"].as<unsigned long>();
      tokenExpires = millis() + (expiresIn * 1000);
      
      LOG_INFO("Device code authentication successful!");
      LOG_INFOF("Access token length: %d", accessToken.length());
      LOG_INFOF("Refresh token length: %d", refreshToken.length());
      LOG_INFOF("Token expires in: %lu seconds", expiresIn);
      
      preferences.putString(KEY_ACCESS_TOKEN, accessToken);
      preferences.putString(KEY_REFRESH_TOKEN, refreshToken);
      preferences.putULong64(KEY_TOKEN_EXPIRES, tokenExpires);
      
      // Clear device code data
      preferences.remove(KEY_DEVICE_CODE);
      preferences.remove(KEY_USER_CODE);
      preferences.remove(KEY_VERIFICATION_URI);
      preferences.remove(KEY_DEVICE_CODE_EXPIRES);
      deviceCode = "";
      userCode = "";
      verificationUri = "";
      deviceCodeExpires = 0;
      
      http.end();
      return true;
    } else if (doc.containsKey("error")) {
      String error = doc["error"].as<String>();
      
      if (error == "authorization_pending") {
        LOG_DEBUG("Authorization still pending, will continue polling");
      } else if (error == "slow_down") {
        LOG_DEBUG("Rate limited, slowing down polling");
        // Add extra delay for next poll
        lastDeviceCodePoll += 5000;
      } else if (error == "authorization_declined") {
        LOG_WARN("User declined authorization");
        currentState = STATE_CONNECTING_OAUTH;
        http.end();
        return false;
      } else if (error == "expired_token") {
        LOG_WARN("Device code expired");
        currentState = STATE_CONNECTING_OAUTH;
        http.end();
        return false;
      } else {
        LOG_ERRORF("OAuth error: %s", error.c_str());
        if (doc.containsKey("error_description")) {
          LOG_ERRORF("Error description: %s", doc["error_description"].as<String>().c_str());
        }
      }
    }
    
    http.end();
    return false;
  } else {
    LOG_ERRORF("Token poll failed with HTTP %d", httpCode);
    http.end();
    return false;
  }
}

void checkTeamsPresence() {
  if (accessToken.length() == 0) {
    LOG_WARN("Cannot check Teams presence - no access token available");
    return;
  }
  
  // Check if token needs refresh
  if (millis() > tokenExpires - 300000) { // Refresh 5 minutes before expiry
    LOG_INFO("Access token expiring soon, attempting refresh...");
    if (!refreshAccessToken()) {
      LOG_ERROR("Token refresh failed, switching to OAuth state");
      currentState = STATE_CONNECTING_OAUTH;
      return;
    }
  }
  
  LOG_DEBUG("Making Teams presence API request");
  HTTPClient http;
  http.begin("https://graph.microsoft.com/v1.0/me/presence");
  http.addHeader("Authorization", "Bearer " + accessToken);
  http.addHeader("User-Agent", "TeamsRedLight/1.0");
  
  int httpCode = http.GET();
  LOG_DEBUGF("Presence API response: HTTP %d", httpCode);
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    LOG_DEBUGF("Presence API response payload length: %d", payload.length());
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      LOG_ERRORF("Failed to parse presence JSON: %s", error.c_str());
      http.end();
      return;
    }
    
    String availability = doc["availability"].as<String>();
    String activity = doc["activity"].as<String>();
    
    LOG_DEBUGF("Teams presence - Availability: %s, Activity: %s", availability.c_str(), activity.c_str());
    
    TeamsPresence newPresence;
    
    // Map Teams presence to our enum
    if (activity == "InAMeeting" || activity == "InACall" || activity == "InAConferenceCall") {
      newPresence = PRESENCE_IN_MEETING;
    } else if (availability == "Busy" || availability == "DoNotDisturb") {
      newPresence = PRESENCE_BUSY;
    } else if (availability == "Available") {
      newPresence = PRESENCE_AVAILABLE;
    } else if (availability == "Away" || availability == "BeRightBack") {
      newPresence = PRESENCE_AWAY;
    } else if (availability == "Offline") {
      newPresence = PRESENCE_OFFLINE;
    } else {
      newPresence = PRESENCE_UNKNOWN;
      LOG_WARNF("Unknown presence state - Availability: %s, Activity: %s", availability.c_str(), activity.c_str());
    }
    
    // Only log if presence changed
    if (newPresence != currentPresence) {
      const char* presenceStr = "";
      switch (newPresence) {
        case PRESENCE_AVAILABLE: presenceStr = "Available"; break;
        case PRESENCE_BUSY: presenceStr = "Busy"; break;
        case PRESENCE_IN_MEETING: presenceStr = "In Meeting"; break;
        case PRESENCE_AWAY: presenceStr = "Away"; break;
        case PRESENCE_OFFLINE: presenceStr = "Offline"; break;
        default: presenceStr = "Unknown"; break;
      }
      
      LOG_INFOF("Teams presence changed: %s (was %s)", presenceStr, 
               currentPresence == PRESENCE_UNKNOWN ? "Unknown" : "different");
      currentPresence = newPresence;
    } else {
      LOG_DEBUG("Teams presence unchanged");
    }
    
  } else if (httpCode == HTTP_CODE_UNAUTHORIZED) {
    LOG_WARN("Teams API returned 401 Unauthorized - token may be expired");
    LOG_INFO("Attempting to refresh access token...");
    if (!refreshAccessToken()) {
      LOG_ERROR("Token refresh failed after 401 response");
      currentState = STATE_CONNECTING_OAUTH;
    } else {
      LOG_INFO("Token refreshed successfully, will retry next cycle");
    }
  } else {
    LOG_ERRORF("Teams presence API failed: HTTP %d", httpCode);
    String response = http.getString();
    if (response.length() > 0 && response.length() < 200) {
      LOG_DEBUGF("Error response: %s", response.c_str());
    }
  }
  
  http.end();
}

bool refreshAccessToken() {
  if (refreshToken.length() == 0) {
    LOG_ERROR("Cannot refresh token - no refresh token available");
    return false;
  }
  
  LOG_INFO("Refreshing OAuth access token...");
  
  HTTPClient http;
  String tokenUrl = "https://login.microsoftonline.com/" + tenantId + "/oauth2/v2.0/token";
  LOG_DEBUGF("Token refresh URL: %s", tokenUrl.c_str());
  
  http.begin(tokenUrl);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  String postData = "client_id=" + clientId;
  postData += "&client_secret=" + clientSecret;
  postData += "&refresh_token=" + refreshToken;
  postData += "&grant_type=refresh_token";
  
  LOG_DEBUG("Sending token refresh request...");
  int httpCode = http.POST(postData);
  LOG_INFOF("Token refresh response: HTTP %d", httpCode);
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    LOG_DEBUGF("Token refresh response length: %d", payload.length());
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      LOG_ERRORF("Failed to parse token refresh JSON: %s", error.c_str());
      http.end();
      return false;
    }
    
    if (doc.containsKey("access_token")) {
      accessToken = doc["access_token"].as<String>();
      if (doc.containsKey("refresh_token")) {
        refreshToken = doc["refresh_token"].as<String>();
        LOG_DEBUG("New refresh token received");
      }
      unsigned long expiresIn = doc["expires_in"].as<unsigned long>();
      tokenExpires = millis() + (expiresIn * 1000);
      
      LOG_INFO("Token refresh successful!");
      LOG_INFOF("New access token length: %d", accessToken.length());
      LOG_INFOF("Token expires in: %lu seconds", expiresIn);
      
      preferences.putString(KEY_ACCESS_TOKEN, accessToken);
      preferences.putString(KEY_REFRESH_TOKEN, refreshToken);
      preferences.putULong64(KEY_TOKEN_EXPIRES, tokenExpires);
      
      http.end();
      return true;
    } else {
      LOG_ERROR("Token refresh failed - no access token in response");
      if (doc.containsKey("error")) {
        String error = doc["error"];
        String errorDesc = doc.containsKey("error_description") ? doc["error_description"].as<String>() : "";
        LOG_ERRORF("Refresh token error: %s - %s", error.c_str(), errorDesc.c_str());
      }
    }
  } else {
    LOG_ERRORF("Token refresh failed with HTTP %d", httpCode);
    String response = http.getString();
    if (response.length() > 0 && response.length() < 200) {
      LOG_DEBUGF("Error response: %s", response.c_str());
    }
  }
  
  http.end();
  return false;
}

void loadConfiguration() {
  LOG_INFO("Loading configuration from flash memory");
  
  wifiSSID = preferences.getString(KEY_WIFI_SSID, "");
  wifiPassword = preferences.getString(KEY_WIFI_PASS, "");
  clientId = preferences.getString(KEY_CLIENT_ID, "");
  clientSecret = preferences.getString(KEY_CLIENT_SECRET, "");
  tenantId = preferences.getString(KEY_TENANT_ID, "common");
  userEmail = preferences.getString(KEY_USER_EMAIL, "");
  accessToken = preferences.getString(KEY_ACCESS_TOKEN, "");
  refreshToken = preferences.getString(KEY_REFRESH_TOKEN, "");
  tokenExpires = preferences.getULong64(KEY_TOKEN_EXPIRES, 0);
  
  // Load device code flow data if present
  deviceCode = preferences.getString(KEY_DEVICE_CODE, "");
  userCode = preferences.getString(KEY_USER_CODE, "");
  verificationUri = preferences.getString(KEY_VERIFICATION_URI, "");
  deviceCodeExpires = preferences.getULong64(KEY_DEVICE_CODE_EXPIRES, 0);
  
  // Load LED pattern preferences (legacy for backward compatibility)
  callPattern = (LEDPattern)preferences.getUInt("call_pattern", DEFAULT_CALL_PATTERN);
  meetingPattern = (LEDPattern)preferences.getUInt(KEY_MEETING_PATTERN, DEFAULT_MEETING_PATTERN);
  availablePattern = (LEDPattern)preferences.getUInt(KEY_NO_MEETING_PATTERN, DEFAULT_AVAILABLE_PATTERN);
  
  // Load multiple LED configuration
  ledCount = preferences.getUInt(KEY_LED_COUNT, 0);
  if (ledCount > MAX_LEDS) ledCount = MAX_LEDS;
  
  if (ledCount == 0) {
    // Initialize with default LED for backward compatibility
    initDefaultLEDs();
  } else {
    // Load configured LEDs
    for (uint8_t i = 0; i < ledCount; i++) {
      String pinKey = String(KEY_LED_PIN_PREFIX) + String(i);
      String callKey = String(KEY_LED_CALL_PATTERN_PREFIX) + String(i);
      String meetKey = String(KEY_LED_MEETING_PATTERN_PREFIX) + String(i);
      String availKey = String(KEY_LED_AVAILABLE_PATTERN_PREFIX) + String(i);
      
      leds[i].pin = preferences.getUInt(pinKey.c_str(), LED_PIN);
      leds[i].callPattern = (LEDPattern)preferences.getUInt(callKey.c_str(), DEFAULT_CALL_PATTERN);
      leds[i].meetingPattern = (LEDPattern)preferences.getUInt(meetKey.c_str(), DEFAULT_MEETING_PATTERN);
      leds[i].availablePattern = (LEDPattern)preferences.getUInt(availKey.c_str(), DEFAULT_AVAILABLE_PATTERN);
      leds[i].enabled = true;
      leds[i].lastToggle = 0;
      leds[i].state = false;
      leds[i].doubleBlinksStartTime = 0;
      leds[i].doubleBlinksState = false;
      leds[i].doubleBlinksCount = 0;
    }
  }
  
  // Log configuration status (without sensitive data)
  LOG_INFOF("WiFi SSID: %s", wifiSSID.length() > 0 ? wifiSSID.c_str() : "(not configured)");
  LOG_INFOF("WiFi Password: %s", wifiPassword.length() > 0 ? "(configured)" : "(not configured)");
  LOG_INFOF("User Email: %s", userEmail.length() > 0 ? userEmail.c_str() : "(not configured)");
  LOG_INFOF("Tenant ID: %s", tenantId.c_str());
  LOG_INFOF("Client ID: %s", clientId.length() > 0 ? "(configured)" : "(not configured)");
  LOG_INFOF("Client Secret: %s", clientSecret.length() > 0 ? "(configured)" : "(not configured)");
  LOG_INFOF("Access Token: %s", accessToken.length() > 0 ? "(available)" : "(not available)");
  LOG_INFOF("Refresh Token: %s", refreshToken.length() > 0 ? "(available)" : "(not available)");
  LOG_INFOF("Call LED Pattern: %d", callPattern);
  LOG_INFOF("Meeting LED Pattern: %d", meetingPattern);
  LOG_INFOF("Available LED Pattern: %d", availablePattern);
  LOG_INFOF("LED Count: %d", ledCount);
  for (uint8_t i = 0; i < ledCount; i++) {
    LOG_INFOF("LED %d: GPIO %d, Call: %d, Meeting: %d, Available: %d", 
              i, leds[i].pin, leds[i].callPattern, leds[i].meetingPattern, leds[i].availablePattern);
  }
  
  if (tokenExpires > 0) {
    long timeToExpiry = (long)(tokenExpires - millis()) / 1000;
    if (timeToExpiry > 0) {
      LOG_INFOF("Token expires in: %ld seconds", timeToExpiry);
    } else {
      LOG_WARN("Access token has already expired");
    }
  }
  
  LOG_INFO("Configuration loading complete");
}

void saveConfiguration() {
  LOG_INFO("Saving configuration to flash memory");
  
  preferences.putString(KEY_WIFI_SSID, wifiSSID);
  preferences.putString(KEY_WIFI_PASS, wifiPassword);
  preferences.putString(KEY_CLIENT_ID, clientId);
  preferences.putString(KEY_CLIENT_SECRET, clientSecret);
  preferences.putString(KEY_TENANT_ID, tenantId);
  preferences.putString(KEY_USER_EMAIL, userEmail);
  preferences.putString(KEY_ACCESS_TOKEN, accessToken);
  preferences.putString(KEY_REFRESH_TOKEN, refreshToken);
  preferences.putULong64(KEY_TOKEN_EXPIRES, tokenExpires);
  
  // Save LED pattern preferences (legacy for backward compatibility)
  preferences.putUInt("call_pattern", callPattern);
  preferences.putUInt(KEY_MEETING_PATTERN, meetingPattern);
  preferences.putUInt(KEY_NO_MEETING_PATTERN, availablePattern);
  
  // Save multiple LED configuration
  preferences.putUInt(KEY_LED_COUNT, ledCount);
  for (uint8_t i = 0; i < ledCount; i++) {
    String pinKey = String(KEY_LED_PIN_PREFIX) + String(i);
    String callKey = String(KEY_LED_CALL_PATTERN_PREFIX) + String(i);
    String meetKey = String(KEY_LED_MEETING_PATTERN_PREFIX) + String(i);
    String availKey = String(KEY_LED_AVAILABLE_PATTERN_PREFIX) + String(i);
    
    preferences.putUInt(pinKey.c_str(), leds[i].pin);
    preferences.putUInt(callKey.c_str(), leds[i].callPattern);
    preferences.putUInt(meetKey.c_str(), leds[i].meetingPattern);
    preferences.putUInt(availKey.c_str(), leds[i].availablePattern);
  }
  
  LOG_INFO("Configuration saved successfully");
}

void restartESP() {
  LOG_WARN("Device restart requested");
  ESP.restart();
}

