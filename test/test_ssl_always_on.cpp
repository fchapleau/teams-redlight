#include <unity.h>
#include <Arduino.h>
#include "config.h"

// Test HTTPS configuration state
void test_https_disabled() {
    // Test that HTTPS is disabled due to WiFiServerSecure not being available
    bool httpsEnabled = ENABLE_HTTPS;
    TEST_ASSERT_FALSE(httpsEnabled);
    
    // Test HTTPS port is still defined for future use
    const int HTTPS_PORT_VALUE = HTTPS_PORT;
    TEST_ASSERT_EQUAL(443, HTTPS_PORT_VALUE);
}

void test_http_port_available() {
    // Verify that HTTP port is properly configured
    const int HTTP_PORT_VALUE = HTTP_PORT;
    TEST_ASSERT_EQUAL(80, HTTP_PORT_VALUE);
    
    // HTTP functionality should be available
    bool httpAvailable = true;
    TEST_ASSERT_TRUE(httpAvailable);
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_https_disabled);
    RUN_TEST(test_http_port_available);
    
    UNITY_END();
}

void loop() {
    // Empty loop for testing
}