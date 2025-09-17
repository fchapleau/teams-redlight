#include <unity.h>
#include <Arduino.h>
#include "../src/main.cpp"

void setUp(void) {
    // Set up code here
}

void tearDown(void) {
    // Clean up code here
}

void test_led_pin_setup() {
    pinMode(LED_PIN, OUTPUT);
    TEST_ASSERT_EQUAL(OUTPUT, digitalPinToDigitalPinSource(LED_PIN));
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
    
    // Ensure keys are different
    TEST_ASSERT_NOT_EQUAL_STRING(KEY_WIFI_SSID, KEY_WIFI_PASS);
    TEST_ASSERT_NOT_EQUAL_STRING(KEY_CLIENT_ID, KEY_CLIENT_SECRET);
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
    
    UNITY_END();
}

void loop() {
    // Empty loop for testing
}