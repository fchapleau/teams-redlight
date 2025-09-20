#include <unity.h>
#include <Arduino.h>
#include "../include/config.h"

void setUp(void) {
    // Set up code here
}

void tearDown(void) {
    // Clean up code here
}

void test_led_pin_setup() {
    // Test that LED pin constants are defined
    TEST_ASSERT_NOT_EQUAL(0, LED_PIN);
    TEST_ASSERT_NOT_EQUAL(0, LED_BUILTIN_PIN);
}

void test_wifi_ssid_constants() {
    TEST_ASSERT_EQUAL_STRING("Teams Red Light", AP_SSID);
    TEST_ASSERT_EQUAL_STRING("configure", AP_PASSWORD);
}

void test_device_state_enum() {
    DeviceState state = STATE_AP_MODE;
    TEST_ASSERT_EQUAL(STATE_AP_MODE, state);
    
    state = STATE_MONITORING;
    TEST_ASSERT_EQUAL(STATE_MONITORING, state);
}

void test_teams_presence_enum() {
    TeamsPresence presence = PRESENCE_UNKNOWN;
    TEST_ASSERT_EQUAL(PRESENCE_UNKNOWN, presence);
    
    presence = PRESENCE_IN_MEETING;
    TEST_ASSERT_EQUAL(PRESENCE_IN_MEETING, presence);
}

void test_led_intervals() {
    TEST_ASSERT_GREATER_THAN(0, LED_SLOW_BLINK_INTERVAL);
    TEST_ASSERT_GREATER_THAN(0, LED_FAST_BLINK_INTERVAL);
    TEST_ASSERT_GREATER_THAN(0, LED_VERY_FAST_BLINK_INTERVAL);
    
    // Verify interval ordering
    TEST_ASSERT_GREATER_THAN(LED_VERY_FAST_BLINK_INTERVAL, LED_FAST_BLINK_INTERVAL);
    TEST_ASSERT_GREATER_THAN(LED_FAST_BLINK_INTERVAL, LED_SLOW_BLINK_INTERVAL);
}

void test_graph_api_endpoints() {
    TEST_ASSERT_EQUAL_STRING("graph.microsoft.com", GRAPH_API_HOST);
    TEST_ASSERT_EQUAL_STRING("/v1.0/me/presence", GRAPH_API_ENDPOINT);
    TEST_ASSERT_EQUAL_STRING("login.microsoftonline.com", GRAPH_LOGIN_HOST);
}

void test_storage_keys() {
    TEST_ASSERT_NOT_NULL(KEY_WIFI_SSID);
    TEST_ASSERT_NOT_NULL(KEY_WIFI_PASS);
    TEST_ASSERT_NOT_NULL(KEY_CLIENT_ID);
    TEST_ASSERT_NOT_NULL(KEY_ACCESS_TOKEN);
    TEST_ASSERT_NOT_NULL(KEY_MEETING_PATTERN);
    TEST_ASSERT_NOT_NULL(KEY_NO_MEETING_PATTERN);
    
    // Ensure keys are different
    TEST_ASSERT_NOT_EQUAL(strcmp(KEY_WIFI_SSID, KEY_WIFI_PASS), 0);
    TEST_ASSERT_NOT_EQUAL(strcmp(KEY_CLIENT_ID, KEY_CLIENT_SECRET), 0);
    TEST_ASSERT_NOT_EQUAL(strcmp(KEY_MEETING_PATTERN, KEY_NO_MEETING_PATTERN), 0);
}

void test_led_pattern_enum() {
    LEDPattern pattern = PATTERN_OFF;
    TEST_ASSERT_EQUAL(PATTERN_OFF, pattern);
    
    pattern = PATTERN_SOLID;
    TEST_ASSERT_EQUAL(PATTERN_SOLID, pattern);
    
    pattern = PATTERN_DOUBLE_BLINK;
    TEST_ASSERT_EQUAL(PATTERN_DOUBLE_BLINK, pattern);
    
    // Test default patterns
    TEST_ASSERT_EQUAL(PATTERN_SOLID, DEFAULT_MEETING_PATTERN);
    TEST_ASSERT_EQUAL(PATTERN_OFF, DEFAULT_NO_MEETING_PATTERN);
}

void test_led_pattern_intervals() {
    TEST_ASSERT_GREATER_THAN(0, LED_PATTERN_SLOW_BLINK_INTERVAL);
    TEST_ASSERT_GREATER_THAN(0, LED_PATTERN_MEDIUM_BLINK_INTERVAL);
    TEST_ASSERT_GREATER_THAN(0, LED_PATTERN_FAST_BLINK_INTERVAL);
    TEST_ASSERT_GREATER_THAN(0, LED_PATTERN_DOUBLE_BLINK_INTERVAL);
    
    // Verify interval ordering
    TEST_ASSERT_GREATER_THAN(LED_PATTERN_FAST_BLINK_INTERVAL, LED_PATTERN_MEDIUM_BLINK_INTERVAL);
    TEST_ASSERT_GREATER_THAN(LED_PATTERN_MEDIUM_BLINK_INTERVAL, LED_PATTERN_SLOW_BLINK_INTERVAL);
}

void setup() {
    delay(2000); // Wait for serial monitor
    
    UNITY_BEGIN();
    
    RUN_TEST(test_led_pin_setup);
    RUN_TEST(test_wifi_ssid_constants);
    RUN_TEST(test_device_state_enum);
    RUN_TEST(test_teams_presence_enum);
    RUN_TEST(test_led_intervals);
    RUN_TEST(test_graph_api_endpoints);
    RUN_TEST(test_storage_keys);
    RUN_TEST(test_led_pattern_enum);
    RUN_TEST(test_led_pattern_intervals);
    
    UNITY_END();
}

void loop() {
    // Empty loop for testing
}