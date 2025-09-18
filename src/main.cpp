#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Update.h>
#include "config.h"

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

// Configuration variables
String wifiSSID;
String wifiPassword;
String clientId;
String clientSecret;
String tenantId;
String userEmail;
String accessToken;
String refreshToken;

// Function declarations
void setupLED();
void updateLED();
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
void checkTeamsPresence();
bool refreshAccessToken();
void loadConfiguration();
void saveConfiguration();

void setup() {
  Serial.begin(115200);
  Serial.println("Teams Red Light - Starting...");
  
  setupLED();
  
  // Initialize preferences
  preferences.begin(PREF_NAMESPACE, false);
  
  // Load saved configuration
  loadConfiguration();
  
  // Set up WiFi
  if (wifiSSID.length() > 0) {
    setupWiFiSTA();
  } else {
    setupWiFiAP();
  }
  
  // Set up web server
  setupWebServer();
  
  Serial.println("Setup complete");
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
        Serial.println("WiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        
        if (accessToken.length() > 0) {
          currentState = STATE_AUTHENTICATED;
        } else {
          currentState = STATE_CONNECTING_OAUTH;
        }
      }
      break;
      
    case STATE_CONNECTING_OAUTH:
      // Wait for OAuth configuration
      break;
      
    case STATE_AUTHENTICATED:
      currentState = STATE_MONITORING;
      break;
      
    case STATE_MONITORING:
      if (millis() - lastPresenceCheck > 30000) { // Check every 30 seconds
        checkTeamsPresence();
        lastPresenceCheck = millis();
      }
      break;
      
    case STATE_ERROR:
      // Handle error state
      break;
  }
  
  delay(100);
}

void setupLED() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void updateLED() {
  unsigned long interval;
  
  switch (currentState) {
    case STATE_AP_MODE:
      interval = LED_VERY_FAST_BLINK_INTERVAL;
      break;
    case STATE_CONNECTING_WIFI:
      interval = LED_SLOW_BLINK_INTERVAL;
      break;
    case STATE_CONNECTING_OAUTH:
      interval = LED_FAST_BLINK_INTERVAL;
      break;
    case STATE_AUTHENTICATED:
    case STATE_MONITORING:
      // LED behavior based on Teams presence
      switch (currentPresence) {
        case PRESENCE_IN_MEETING:
        case PRESENCE_BUSY:
          digitalWrite(LED_PIN, HIGH); // Solid red
          return;
        default:
          digitalWrite(LED_PIN, LOW); // Off
          return;
      }
      break;
    case STATE_ERROR:
      interval = LED_FAST_BLINK_INTERVAL;
      break;
  }
  
  if (millis() - lastLedToggle > interval) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    lastLedToggle = millis();
  }
}

void setupWiFiAP() {
  Serial.println("Setting up WiFi Access Point...");
  currentState = STATE_AP_MODE;
  
  WiFi.mode(WIFI_AP);
  
  // Configure AP with custom IP in 172.16.0.0/12 range
  IPAddress local_IP(172, 16, 4, 1);      // Gateway IP
  IPAddress gateway(172, 16, 4, 1);       // Gateway IP  
  IPAddress subnet(255, 240, 0, 0);       // Subnet mask for /12
  
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

void setupWiFiSTA() {
  Serial.println("Connecting to WiFi...");
  currentState = STATE_CONNECTING_WIFI;
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  
  // Wait for connection with timeout
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi, starting AP mode");
    setupWiFiAP();
  }
}

void setupWebServer() {
  // Serve static files
  server.on("/", [](){
    handleRoot();
  });
  
  server.on("/config", [](){
    handleConfig();
  });
  
  server.on("/save", HTTP_POST, [](){
    handleSave();
  });
  
  server.on("/status", [](){
    handleStatus();
  });
  
  server.on("/update", HTTP_POST, [](){
    handleUpdate();
  });
  
  server.on("/login", [](){
    handleLogin();
  });
  
  server.on("/callback", [](){
    handleCallback();
  });
  
  server.on("/restart", HTTP_POST, [](){
    server.send(200, "text/plain", "Restarting...");
    delay(1000);
    ESP.restart();
  });
  
  server.begin();
  Serial.println("Web server started");
}

void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Teams Red Light</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #d73502; text-align: center; }
        .status { padding: 15px; margin: 10px 0; border-radius: 5px; text-align: center; font-weight: bold; }
        .status.connected { background-color: #d4edda; color: #155724; }
        .status.disconnected { background-color: #f8d7da; color: #721c24; }
        .status.configuring { background-color: #fff3cd; color: #856404; }
        button { background-color: #d73502; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }
        button:hover { background-color: #b12d02; }
        .form-group { margin: 15px 0; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input[type="text"], input[type="password"], input[type="email"] { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔴 Teams Red Light</h1>
        <div id="status" class="status configuring">Loading status...</div>
        
        <div id="configSection">
            <h3>Configuration</h3>
            <button onclick="window.location.href='/config'">Configure Device</button>
            <button onclick="checkStatus()">Refresh Status</button>
            <button onclick="restartDevice()">Restart Device</button>
        </div>
    </div>
    
    <script>
        function checkStatus() {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    const statusDiv = document.getElementById('status');
                    if (data.state === 'monitoring') {
                        statusDiv.className = 'status connected';
                        statusDiv.innerHTML = `✅ Connected - Presence: ${data.presence}`;
                    } else if (data.state === 'ap_mode') {
                        statusDiv.className = 'status configuring';
                        statusDiv.innerHTML = '⚙️ Configuration Mode - Please configure WiFi and Teams settings';
                    } else {
                        statusDiv.className = 'status disconnected';
                        statusDiv.innerHTML = `⚠️ ${data.message || 'Not connected'}`;
                    }
                })
                .catch(() => {
                    document.getElementById('status').innerHTML = '❌ Unable to get status';
                });
        }
        
        function restartDevice() {
            if (confirm('Are you sure you want to restart the device?')) {
                fetch('/restart', { method: 'POST' })
                    .then(() => {
                        alert('Device is restarting...');
                        setTimeout(() => location.reload(), 5000);
                    });
            }
        }
        
        // Check status on page load
        checkStatus();
        
        // Auto-refresh status every 10 seconds
        setInterval(checkStatus, 10000);
    </script>
</body>
</html>
  )";
  
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
  // Save WiFi configuration
  if (server.hasArg("wifi_ssid")) {
    wifiSSID = server.arg("wifi_ssid");
    preferences.putString(KEY_WIFI_SSID, wifiSSID);
  }
  
  if (server.hasArg("wifi_password") && server.arg("wifi_password").length() > 0) {
    wifiPassword = server.arg("wifi_password");
    preferences.putString(KEY_WIFI_PASS, wifiPassword);
  }
  
  // Save OAuth configuration
  if (server.hasArg("user_email")) {
    userEmail = server.arg("user_email");
    preferences.putString(KEY_USER_EMAIL, userEmail);
  }
  
  if (server.hasArg("tenant_id")) {
    tenantId = server.arg("tenant_id");
    if (tenantId.length() == 0) tenantId = "common";
    preferences.putString(KEY_TENANT_ID, tenantId);
  }
  
  if (server.hasArg("client_id")) {
    clientId = server.arg("client_id");
    preferences.putString(KEY_CLIENT_ID, clientId);
  }
  
  if (server.hasArg("client_secret") && server.arg("client_secret").length() > 0) {
    clientSecret = server.arg("client_secret");
    preferences.putString(KEY_CLIENT_SECRET, clientSecret);
  }
  
  if (server.hasArg("ota_url")) {
    String otaUrl = server.arg("ota_url");
    preferences.putString(OTA_UPDATE_URL_KEY, otaUrl);
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
  // OTA Update functionality - simplified for now
  server.send(200, "text/plain", "OTA Update not implemented in this version");
}

void handleLogin() {
  if (clientId.length() == 0 || tenantId.length() == 0) {
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
  
  server.sendHeader("Location", authUrl);
  server.send(302, "text/plain", "Redirecting to Microsoft login...");
}

void handleCallback() {
  if (server.hasArg("code")) {
    String code = server.arg("code");
    
    // Exchange code for tokens
    HTTPClient http;
    http.begin("https://login.microsoftonline.com/" + tenantId + "/oauth2/v2.0/token");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    String deviceIP = WiFi.localIP().toString();
    String redirectUri = "http://" + deviceIP + "/callback";
    
    String postData = "client_id=" + clientId;
    postData += "&client_secret=" + clientSecret;
    postData += "&code=" + code;
    postData += "&grant_type=authorization_code";
    postData += "&redirect_uri=" + redirectUri;
    
    int httpCode = http.POST(postData);
    
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, payload);
      
      if (doc.containsKey("access_token")) {
        accessToken = doc["access_token"].as<String>();
        refreshToken = doc["refresh_token"].as<String>();
        tokenExpires = millis() + (doc["expires_in"].as<unsigned long>() * 1000);
        
        preferences.putString(KEY_ACCESS_TOKEN, accessToken);
        preferences.putString(KEY_REFRESH_TOKEN, refreshToken);
        preferences.putULong64(KEY_TOKEN_EXPIRES, tokenExpires);
        
        currentState = STATE_AUTHENTICATED;
        
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
        server.send(400, "text/plain", "Authentication failed: No access token received");
      }
    } else {
      server.send(400, "text/plain", "Authentication failed: HTTP " + String(httpCode));
    }
    
    http.end();
  } else if (server.hasArg("error")) {
    String error = server.arg("error");
    server.send(400, "text/plain", "Authentication failed: " + error);
  } else {
    server.send(400, "text/plain", "Authentication failed: No authorization code received");
  }
}

void checkTeamsPresence() {
  if (accessToken.length() == 0) {
    return;
  }
  
  // Check if token needs refresh
  if (millis() > tokenExpires - 300000) { // Refresh 5 minutes before expiry
    if (!refreshAccessToken()) {
      currentState = STATE_CONNECTING_OAUTH;
      return;
    }
  }
  
  HTTPClient http;
  http.begin("https://graph.microsoft.com/v1.0/me/presence");
  http.addHeader("Authorization", "Bearer " + accessToken);
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    
    String availability = doc["availability"].as<String>();
    String activity = doc["activity"].as<String>();
    
    // Map Teams presence to our enum
    if (activity == "InAMeeting" || activity == "InACall" || activity == "InAConferenceCall") {
      currentPresence = PRESENCE_IN_MEETING;
    } else if (availability == "Busy" || availability == "DoNotDisturb") {
      currentPresence = PRESENCE_BUSY;
    } else if (availability == "Available") {
      currentPresence = PRESENCE_AVAILABLE;
    } else if (availability == "Away" || availability == "BeRightBack") {
      currentPresence = PRESENCE_AWAY;
    } else if (availability == "Offline") {
      currentPresence = PRESENCE_OFFLINE;
    } else {
      currentPresence = PRESENCE_UNKNOWN;
    }
    
    Serial.println("Presence updated: " + availability + " (" + activity + ")");
  } else if (httpCode == HTTP_CODE_UNAUTHORIZED) {
    Serial.println("Token expired, attempting refresh...");
    if (!refreshAccessToken()) {
      currentState = STATE_CONNECTING_OAUTH;
    }
  } else {
    Serial.println("Failed to get presence: HTTP " + String(httpCode));
  }
  
  http.end();
}

bool refreshAccessToken() {
  if (refreshToken.length() == 0) {
    return false;
  }
  
  HTTPClient http;
  http.begin("https://login.microsoftonline.com/" + tenantId + "/oauth2/v2.0/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  String postData = "client_id=" + clientId;
  postData += "&client_secret=" + clientSecret;
  postData += "&refresh_token=" + refreshToken;
  postData += "&grant_type=refresh_token";
  
  int httpCode = http.POST(postData);
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, payload);
    
    if (doc.containsKey("access_token")) {
      accessToken = doc["access_token"].as<String>();
      if (doc.containsKey("refresh_token")) {
        refreshToken = doc["refresh_token"].as<String>();
      }
      tokenExpires = millis() + (doc["expires_in"].as<unsigned long>() * 1000);
      
      preferences.putString(KEY_ACCESS_TOKEN, accessToken);
      preferences.putString(KEY_REFRESH_TOKEN, refreshToken);
      preferences.putULong64(KEY_TOKEN_EXPIRES, tokenExpires);
      
      return true;
    }
  }
  
  http.end();
  return false;
}

void loadConfiguration() {
  wifiSSID = preferences.getString(KEY_WIFI_SSID, "");
  wifiPassword = preferences.getString(KEY_WIFI_PASS, "");
  clientId = preferences.getString(KEY_CLIENT_ID, "");
  clientSecret = preferences.getString(KEY_CLIENT_SECRET, "");
  tenantId = preferences.getString(KEY_TENANT_ID, "common");
  userEmail = preferences.getString(KEY_USER_EMAIL, "");
  accessToken = preferences.getString(KEY_ACCESS_TOKEN, "");
  refreshToken = preferences.getString(KEY_REFRESH_TOKEN, "");
  tokenExpires = preferences.getULong64(KEY_TOKEN_EXPIRES, 0);
}

void saveConfiguration() {
  preferences.putString(KEY_WIFI_SSID, wifiSSID);
  preferences.putString(KEY_WIFI_PASS, wifiPassword);
  preferences.putString(KEY_CLIENT_ID, clientId);
  preferences.putString(KEY_CLIENT_SECRET, clientSecret);
  preferences.putString(KEY_TENANT_ID, tenantId);
  preferences.putString(KEY_USER_EMAIL, userEmail);
  preferences.putString(KEY_ACCESS_TOKEN, accessToken);
  preferences.putString(KEY_REFRESH_TOKEN, refreshToken);
  preferences.putULong64(KEY_TOKEN_EXPIRES, tokenExpires);
}

void restartESP() {
  ESP.restart();
}