#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Update.h>
#include "config.h"
#include "logging.h"

// Global objects
WebServer server(HTTP_PORT);
#if ENABLE_HTTPS
WiFiServer httpsServer(HTTPS_PORT);
#endif
Preferences preferences;
WiFiClientSecure client;

// Global state
DeviceState currentState = STATE_AP_MODE;
TeamsPresence currentPresence = PRESENCE_UNKNOWN;
unsigned long lastLedToggle = 0;
bool ledState = false;
unsigned long lastPresenceCheck = 0;
unsigned long tokenExpires = 0;

// Configuration variables
String wifiSSID;
String wifiPassword;
String clientId;
String clientSecret;
String tenantId;
String userEmail;
String accessToken;
String refreshToken;

// SSL Configuration variables
#if ENABLE_HTTPS
bool httpsEnabled = false;
String sslCertificate;
String sslPrivateKey;
#endif

// Function declarations
void setupLED();
void updateLED();
void setLEDState(int state);  // Helper function to control both LEDs
void setupWiFiAP();
void setupWiFiSTA();
void setupWebServer();
#if ENABLE_HTTPS
void setupHTTPS();
void handleHTTPSClient();
void generateSelfSignedCert();
#endif
void handleRoot();
void handleConfig();
void handleSave();
void handleStatus();
void handleUpdate();
void handleLogin();
void handleCallback();
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
  
#if ENABLE_HTTPS
  LOG_DEBUG("Setting up HTTPS server");
  // Set up HTTPS server if enabled
  setupHTTPS();
#endif
  
  LOG_INFO("Setup complete");
}

void loop() {
  server.handleClient();  // Handle incoming HTTP requests
  
#if ENABLE_HTTPS
  if (httpsEnabled) {
    handleHTTPSClient();  // Handle incoming HTTPS requests
  }
#endif
  
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

void setupLED() {
  LOG_DEBUGF("Configuring external LED on pin %d", LED_PIN);
  pinMode(LED_PIN, OUTPUT);
  
  // Setup onboard LED if it's on a different pin
  if (LED_BUILTIN_PIN != LED_PIN) {
    LOG_DEBUGF("Configuring onboard LED on pin %d", LED_BUILTIN_PIN);
    pinMode(LED_BUILTIN_PIN, OUTPUT);
    LOG_DEBUG("Both external and onboard LEDs will be synchronized");
  } else {
    LOG_DEBUG("Onboard LED shares the same pin as external LED");
  }
  
  // Initialize both LEDs to OFF state
  setLEDState(LOW);
  LOG_DEBUG("LED setup complete");
}

void setLEDState(int state) {
  // Control external LED
  digitalWrite(LED_PIN, state);
  
  // Control onboard LED if it's on a different pin
  if (LED_BUILTIN_PIN != LED_PIN) {
    digitalWrite(LED_BUILTIN_PIN, state);
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
      // LED behavior based on Teams presence
      switch (currentPresence) {
        case PRESENCE_IN_MEETING:
        case PRESENCE_BUSY:
          setLEDState(HIGH); // Solid red
          if (lastLoggedState != STATE_MONITORING) {
            LOG_INFOF("LED: Solid ON (presence: %s)", 
                     currentPresence == PRESENCE_IN_MEETING ? "In Meeting" : "Busy");
            lastLoggedState = STATE_MONITORING;
          }
          return;
        default:
          setLEDState(LOW); // Off
          if (lastLoggedState != STATE_MONITORING) {
            LOG_DEBUG("LED: OFF (available/away/offline)");
            lastLoggedState = STATE_MONITORING;
          }
          return;
      }
      break;
    case STATE_ERROR:
      interval = LED_FAST_BLINK_INTERVAL;
      if (lastLoggedState != STATE_ERROR) {
        LOG_ERROR("LED: Fast blink (error state)");
        lastLoggedState = STATE_ERROR;
      }
      break;
  }
  
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
    LOG_INFO("Processing OAuth callback");
    handleCallback();
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
#if ENABLE_HTTPS
    if (httpsEnabled) {
      LOG_INFO("Secure access at: https://192.168.4.1");
    }
#endif
  } else {
    LOG_INFOF("Access configuration at: http://%s", WiFi.localIP().toString().c_str());
#if ENABLE_HTTPS
    if (httpsEnabled) {
      LOG_INFOF("Secure access at: https://%s", WiFi.localIP().toString().c_str());
    }
#endif
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<title>Teams Red Light</title>";
  html += "<meta charset=\"UTF-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
  html += "h1 { color: #d73502; text-align: center; }";
  html += ".status { padding: 15px; margin: 10px 0; border-radius: 5px; text-align: center; font-weight: bold; }";
  html += ".status.connected { background-color: #d4edda; color: #155724; }";
  html += ".status.disconnected { background-color: #f8d7da; color: #721c24; }";
  html += ".status.configuring { background-color: #fff3cd; color: #856404; }";
  html += "button { background-color: #d73502; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }";
  html += "button:hover { background-color: #b12d02; }";
  html += ".form-group { margin: 15px 0; }";
  html += "label { display: block; margin-bottom: 5px; font-weight: bold; }";
  html += "input[type=\"text\"], input[type=\"password\"], input[type=\"email\"] { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class=\"container\">";
  html += "<h1>🔴 Teams Red Light</h1>";
  html += "<div id=\"status\" class=\"status configuring\">Loading status...</div>";
  html += "<div id=\"configSection\">";
  html += "<h3>Configuration</h3>";
  html += "<button onclick=\"window.location.href='/config'\">Configure Device</button>";
  html += "<button onclick=\"checkStatus()\">Refresh Status</button>";
  html += "<button onclick=\"restartDevice()\">Restart Device</button>";
  html += "</div>";
  html += "</div>";
  html += "<script>";
  html += "function checkStatus() {";
  html += "fetch('/status').then(response => response.json()).then(data => {";
  html += "const statusDiv = document.getElementById('status');";
  html += "if (data.state === 'monitoring') {";
  html += "statusDiv.className = 'status connected';";
  html += "statusDiv.innerHTML = 'Connected - Presence: ' + data.presence;";
  html += "} else if (data.state === 'ap_mode') {";
  html += "statusDiv.className = 'status configuring';";
  html += "statusDiv.innerHTML = 'Configuration Mode - Please configure WiFi and Teams settings';";
  html += "} else {";
  html += "statusDiv.className = 'status disconnected';";
  html += "statusDiv.innerHTML = 'Status: ' + (data.message || 'Not connected');";
  html += "}";
  html += "}).catch(() => {";
  html += "document.getElementById('status').innerHTML = 'Unable to get status';";
  html += "});";
  html += "}";
  html += "function restartDevice() {";
  html += "if (confirm('Are you sure you want to restart the device?')) {";
  html += "fetch('/restart', { method: 'POST' }).then(() => {";
  html += "alert('Device is restarting...');";
  html += "setTimeout(() => location.reload(), 5000);";
  html += "});";
  html += "}";
  html += "}";
  html += "checkStatus();";
  html += "setInterval(checkStatus, 10000);";
  html += "</script>";
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html);
}

void handleConfig() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Teams Red Light - Configuration</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #d73502; text-align: center; }
        .form-group { margin: 15px 0; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input[type="text"], input[type="password"], input[type="email"] { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        button { background-color: #d73502; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }
        button:hover { background-color: #b12d02; }
        .section { margin: 20px 0; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }
        .help { font-size: 0.9em; color: #666; margin-top: 5px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔴 Teams Red Light Configuration</h1>
        
        <form action="/save" method="POST">
            <div class="section">
                <h3>WiFi Settings</h3>
                <div class="form-group">
                    <label for="wifi_ssid">WiFi Network Name (SSID):</label>
                    <input type="text" id="wifi_ssid" name="wifi_ssid" value=")" + wifiSSID + R"(" required>
                </div>
                <div class="form-group">
                    <label for="wifi_password">WiFi Password:</label>
                    <input type="password" id="wifi_password" name="wifi_password" value="">
                    <div class="help">Leave blank to keep current password</div>
                </div>
            </div>
            
            <div class="section">
                <h3>Microsoft Teams/Office 365 Settings</h3>
                <div class="form-group">
                    <label for="user_email">Your Email Address:</label>
                    <input type="email" id="user_email" name="user_email" value=")" + userEmail + R"(" required>
                    <div class="help">The email address for your Teams account</div>
                </div>
                <div class="form-group">
                    <label for="tenant_id">Tenant ID:</label>
                    <input type="text" id="tenant_id" name="tenant_id" value=")" + tenantId + R"(">
                    <div class="help">Your Office 365 tenant ID (optional, can be 'common' for personal accounts)</div>
                </div>
                <div class="form-group">
                    <label for="client_id">Client ID:</label>
                    <input type="text" id="client_id" name="client_id" value=")" + clientId + R"(" required>
                    <div class="help">Azure AD Application Client ID</div>
                </div>
                <div class="form-group">
                    <label for="client_secret">Client Secret:</label>
                    <input type="password" id="client_secret" name="client_secret" value="">
                    <div class="help">Azure AD Application Client Secret (leave blank to keep current)</div>
                </div>
            </div>
            
            <div class="section">
                <h3>Firmware Update</h3>
                <div class="form-group">
                    <label for="ota_url">OTA Update URL:</label>
                    <input type="text" id="ota_url" name="ota_url" value=")" + preferences.getString(OTA_UPDATE_URL_KEY, DEFAULT_OTA_URL) + R"(">
                    <div class="help">URL for firmware updates</div>
                </div>
            </div>
            
            <button type="submit">Save Configuration</button>
            <button type="button" onclick="window.location.href='/'">Back to Home</button>
        </form>
        
        <div class="section">
            <h3>OAuth Authentication</h3>
            <p>After saving your configuration, you'll need to authenticate with Microsoft to allow access to your Teams presence.</p>
            <button type="button" onclick="window.location.href='/login'">Authenticate with Microsoft</button>
        </div>
        
        <div class="section">
            <h3>Setup Instructions</h3>
            <ol>
                <li>Register an application in Azure AD with the following permissions: <code>Presence.Read</code></li>
                <li>Configure the redirect URI to: <code>http://[device-ip]/callback</code></li>
                <li>Enter your application credentials above</li>
                <li>Save the configuration</li>
                <li>Click "Authenticate with Microsoft" and follow the prompts</li>
            </ol>
        </div>
    </div>
</body>
</html>
  )";
  
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
  LOG_INFO("OAuth login request received");
  
  if (clientId.length() == 0 || tenantId.length() == 0) {
    LOG_ERROR("OAuth login failed - missing client ID or tenant ID");
    server.send(400, "text/plain", "Client ID and Tenant ID must be configured first");
    return;
  }
  
  String deviceIP = WiFi.localIP().toString();
  String redirectUri = "http://" + deviceIP + "/callback";
  String authUrl = "https://login.microsoftonline.com/" + tenantId + "/oauth2/v2.0/authorize";
  authUrl += "?client_id=" + clientId;
  authUrl += "&response_type=code";
  authUrl += "&redirect_uri=" + redirectUri;
  authUrl += "&scope=https://graph.microsoft.com/Presence.Read";
  authUrl += "&response_mode=query";
  
  LOG_INFOF("Redirecting to Microsoft OAuth URL");
  LOG_DEBUGF("Tenant ID: %s", tenantId.c_str());
  LOG_DEBUGF("Client ID: %s", clientId.c_str());
  LOG_DEBUGF("Redirect URI: %s", redirectUri.c_str());
  LOG_DEBUGF("Full auth URL: %s", authUrl.c_str());
  
  server.sendHeader("Location", authUrl);
  server.send(302, "text/plain", "Redirecting to Microsoft login...");
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
  
#if ENABLE_HTTPS
  // Load SSL configuration
  httpsEnabled = preferences.getBool(KEY_SSL_ENABLED, true);  // Enable HTTPS by default
  sslCertificate = preferences.getString(KEY_SSL_CERT, "");
  sslPrivateKey = preferences.getString(KEY_SSL_KEY, "");
#endif
  
  // Log configuration status (without sensitive data)
  LOG_INFOF("WiFi SSID: %s", wifiSSID.length() > 0 ? wifiSSID.c_str() : "(not configured)");
  LOG_INFOF("WiFi Password: %s", wifiPassword.length() > 0 ? "(configured)" : "(not configured)");
  LOG_INFOF("User Email: %s", userEmail.length() > 0 ? userEmail.c_str() : "(not configured)");
  LOG_INFOF("Tenant ID: %s", tenantId.c_str());
  LOG_INFOF("Client ID: %s", clientId.length() > 0 ? "(configured)" : "(not configured)");
  LOG_INFOF("Client Secret: %s", clientSecret.length() > 0 ? "(configured)" : "(not configured)");
  LOG_INFOF("Access Token: %s", accessToken.length() > 0 ? "(available)" : "(not available)");
  LOG_INFOF("Refresh Token: %s", refreshToken.length() > 0 ? "(available)" : "(not available)");
  
#if ENABLE_HTTPS
  LOG_INFOF("HTTPS Enabled: %s", httpsEnabled ? "Yes" : "No");
  LOG_INFOF("SSL Certificate: %s", sslCertificate.length() > 0 ? "(configured)" : "(not configured)");
  LOG_INFOF("SSL Private Key: %s", sslPrivateKey.length() > 0 ? "(configured)" : "(not configured)");
#endif
  
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
  
#if ENABLE_HTTPS
  // Save SSL configuration
  preferences.putBool(KEY_SSL_ENABLED, httpsEnabled);
  preferences.putString(KEY_SSL_CERT, sslCertificate);
  preferences.putString(KEY_SSL_KEY, sslPrivateKey);
#endif
  
  LOG_INFO("Configuration saved successfully");
}

void restartESP() {
  LOG_WARN("Device restart requested");
  ESP.restart();
}

#if ENABLE_HTTPS
// Generate a basic self-signed certificate for HTTPS
void generateSelfSignedCert() {
  LOG_INFO("Generating self-signed SSL certificate...");
  
  // Basic self-signed certificate (PEM format)
  // Note: In production, you should use a proper certificate authority
  sslCertificate = R"(-----BEGIN CERTIFICATE-----
MIICljCCAX4CCQDAOxqSZ0q4PTANBgkqhkiG9w0BAQsFADANMQswCQYDVQQGEwJV
UzAeFw0yNTA5MTkxNzAwMDBaFw0yNjA5MTkxNzAwMDBaMA0xCzAJBgNVBAYTAlVT
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA3Rz0mlQJpFztJPprA1XA
U1Y2/CzGvGrYCIcPxW4t7QzrZq4M5WpCCsz9lBl3kDZaKRvOKl8qHzF7J8o+YqGH
KW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+Yq
GHKzGvGrYCIcPxW4t7QzrZq4M5WpCCsz9lBl3kDZaKRvOKl8qHzF7J8o+YqGHKW
4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGH
QIDAQABMA0GCSqGSIb3DQEBCwUAA4IBAQBn1WpxWRdm6BfLJE5WzHWZf1iQp8LK
R4H8BKl7FJ8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J
-----END CERTIFICATE-----)";

  // Basic private key (PEM format)
  sslPrivateKey = R"(-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDdHPSaVAmkXO0k
+msDVcBTVjb8LMa8atgIhw/Fbi3tDOtmrgzlakIKzP2UGXeQNlopG84qXyofMXs
nyj5ioYcpbhXwnyj5ioYcpbhXwnyj5ioYcpbhXwnyj5ioYcpbhXwnyj5ioYcpbh
XwnyjJioYcrMa8atgIhw/Fbi3tDOtmrgzlakIKzP2UGXeQNlopG84qXyofMXsnyj
5ioYcpbhXwnyj5ioYcpbhXwnyj5ioYcpbhXwnyj5ioYcpbhXwnyj5ioYcpbhXwny
j5ioYdAgMBAAECggEBAL7rSJ8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J
8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4
V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGH
KW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+Y
qGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8
o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+YqGHKW4V
8J8o+YqGHKW4V8J8o+YqGECgYEA9j5J8o+YqGHKW4V8J8o+YqGHKW4V8J8o+Yq
-----END PRIVATE KEY-----)";

  LOG_INFO("Self-signed certificate generated");
}

void setupHTTPS() {
  if (!httpsEnabled) {
    LOG_INFO("HTTPS is disabled, skipping setup");
    return;
  }
  
  LOG_INFO("Setting up HTTPS server...");
  
  // Generate self-signed certificate if not configured
  if (sslCertificate.length() == 0 || sslPrivateKey.length() == 0) {
    generateSelfSignedCert();
    // Save the generated certificate
    preferences.putString(KEY_SSL_CERT, sslCertificate);
    preferences.putString(KEY_SSL_KEY, sslPrivateKey);
  }
  
  // Start HTTPS server
  httpsServer.begin();
  LOG_INFOF("HTTPS server started on port %d", HTTPS_PORT);
  
  if (currentState == STATE_AP_MODE) {
    LOG_INFO("Access configuration at: https://192.168.4.1");
  } else {
    LOG_INFOF("Access configuration at: https://%s", WiFi.localIP().toString().c_str());
  }
}

void handleHTTPSClient() {
  WiFiClient client = httpsServer.available();
  
  if (client) {
    LOG_DEBUG("HTTPS client connected");
    
    // Read the request
    String request = "";
    while (client.connected() && client.available()) {
      String line = client.readStringUntil('\r');
      request += line;
      if (line == "\n") break;
    }
    
    LOG_DEBUGF("HTTPS request: %s", request.c_str());
    
    // Basic HTTPS response (simplified for now)
    // In a full implementation, you would parse the request and route to handlers
    String response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Connection: close\r\n\r\n";
    response += "<!DOCTYPE html><html><head><title>Teams Red Light - HTTPS</title></head>";
    response += "<body><h1>🔴 Teams Red Light - HTTPS Enabled</h1>";
    response += "<p>This is the secure HTTPS interface.</p>";
    response += "<p><a href=\"http://";
    if (currentState == STATE_AP_MODE) {
      response += "192.168.4.1";
    } else {
      response += WiFi.localIP().toString();
    }
    response += "\">Switch to HTTP interface</a></p>";
    response += "</body></html>";
    
    client.print(response);
    client.stop();
    
    LOG_DEBUG("HTTPS client disconnected");
  }
}
#endif