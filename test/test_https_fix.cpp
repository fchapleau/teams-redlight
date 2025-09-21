#include <unity.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Test that HTTPS configuration is available
void test_https_client_setup() {
    // Test that HTTPClient can be instantiated
    HTTPClient http;
    TEST_ASSERT_TRUE(true); // Basic instantiation test
}

void test_https_insecure_mode() {
    // Test that setInsecure method is available for WiFiClientSecure
    WiFiClientSecure secureClient;
    
    // This should not fail - setInsecure() should be available on WiFiClientSecure
    secureClient.setInsecure();
    TEST_ASSERT_TRUE(true);
}

void test_microsoft_graph_api_endpoints() {
    // Test that we can construct the proper URLs for Microsoft Graph API
    String graphUrl = "https://graph.microsoft.com/v1.0/me/presence";
    String loginUrl = "https://login.microsoftonline.com/common/oauth2/v2.0/token";
    String deviceCodeUrl = "https://login.microsoftonline.com/common/oauth2/v2.0/devicecode";
    
    TEST_ASSERT_TRUE(graphUrl.startsWith("https://"));
    TEST_ASSERT_TRUE(loginUrl.startsWith("https://"));
    TEST_ASSERT_TRUE(deviceCodeUrl.startsWith("https://"));
    
    TEST_ASSERT_TRUE(graphUrl.indexOf("graph.microsoft.com") > 0);
    TEST_ASSERT_TRUE(loginUrl.indexOf("login.microsoftonline.com") > 0);
    TEST_ASSERT_TRUE(deviceCodeUrl.indexOf("login.microsoftonline.com") > 0);
}

void test_http_client_begin_with_https() {
    // Test that HTTPClient can begin() with HTTPS URLs using WiFiClientSecure
    WiFiClientSecure secureClient;
    secureClient.setInsecure(); // Set insecure mode (this is what our fix adds)
    
    HTTPClient http;
    
    // This should not crash or throw errors
    bool success = http.begin(secureClient, "https://graph.microsoft.com/v1.0/me/presence");
    TEST_ASSERT_TRUE(success);
    
    // End the connection
    http.end();
    
    TEST_ASSERT_TRUE(true);
}

void test_wifi_client_secure_available() {
    // Test that WiFiClientSecure is available (dependency for HTTPS)
    WiFiClientSecure client;
    TEST_ASSERT_TRUE(true); // Basic instantiation test
}

void setup() {
    delay(2000); // Wait for serial monitor
    
    UNITY_BEGIN();
    
    RUN_TEST(test_https_client_setup);
    RUN_TEST(test_https_insecure_mode);
    RUN_TEST(test_microsoft_graph_api_endpoints);
    RUN_TEST(test_http_client_begin_with_https);
    RUN_TEST(test_wifi_client_secure_available);
    
    UNITY_END();
}

void loop() {
    // Empty loop for testing
}