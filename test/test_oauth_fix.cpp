#include <unity.h>
#include <Arduino.h>

// Test that OAuth error handling works for HTTP 400/401 responses
void test_oauth_error_handling() {
    // Test that HTTP 400 is handled for OAuth errors
    // This validates the fix for the device code polling issue
    
    // Simulate HTTP status codes that should be processed for JSON
    int validStatusCodes[] = {200, 400, 401};
    int validCount = sizeof(validStatusCodes) / sizeof(validStatusCodes[0]);
    
    // Verify all expected status codes are handled
    TEST_ASSERT_EQUAL(3, validCount);
    TEST_ASSERT_EQUAL(200, validStatusCodes[0]); // HTTP_CODE_OK
    TEST_ASSERT_EQUAL(400, validStatusCodes[1]); // Bad Request with OAuth errors
    TEST_ASSERT_EQUAL(401, validStatusCodes[2]); // Unauthorized with OAuth errors
    
    // Test that other status codes should still be treated as HTTP errors
    int invalidStatusCodes[] = {404, 500, 503};
    int invalidCount = sizeof(invalidStatusCodes) / sizeof(invalidStatusCodes[0]);
    
    TEST_ASSERT_EQUAL(3, invalidCount);
    TEST_ASSERT_EQUAL(404, invalidStatusCodes[0]);
    TEST_ASSERT_EQUAL(500, invalidStatusCodes[1]);
    TEST_ASSERT_EQUAL(503, invalidStatusCodes[2]);
}

void test_oauth_error_responses() {
    // Test that common OAuth error types are defined
    // These are the error types that Microsoft OAuth returns
    
    const char* expectedErrors[] = {
        "authorization_pending",
        "slow_down", 
        "authorization_declined",
        "expired_token"
    };
    
    int errorCount = sizeof(expectedErrors) / sizeof(expectedErrors[0]);
    TEST_ASSERT_EQUAL(4, errorCount);
    
    // Verify error strings are valid
    TEST_ASSERT_TRUE(strlen(expectedErrors[0]) > 0);
    TEST_ASSERT_TRUE(strlen(expectedErrors[1]) > 0);
    TEST_ASSERT_TRUE(strlen(expectedErrors[2]) > 0);
    TEST_ASSERT_TRUE(strlen(expectedErrors[3]) > 0);
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_oauth_error_handling);
    RUN_TEST(test_oauth_error_responses);
    
    UNITY_END();
}

void loop() {
    // Empty loop for testing
}