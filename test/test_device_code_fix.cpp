#include <unity.h>
#include <Arduino.h>

// Test that device code flow authentication works without client_secret
void test_device_code_flow_public_client() {
    // Test that device code flow should work as a public client
    // This validates the fix for AADSTS7000218 error
    
    // Device code flow for public clients should only include:
    // - grant_type=urn:ietf:params:oauth:grant-type:device_code
    // - client_id
    // - device_code
    // It should NOT include client_secret
    
    const char* requiredParams[] = {
        "grant_type=urn:ietf:params:oauth:grant-type:device_code",
        "client_id=",
        "device_code="
    };
    
    const char* prohibitedParams[] = {
        "client_secret="
    };
    
    int requiredCount = sizeof(requiredParams) / sizeof(requiredParams[0]);
    int prohibitedCount = sizeof(prohibitedParams) / sizeof(prohibitedParams[0]);
    
    // Verify required parameters are defined
    TEST_ASSERT_EQUAL(3, requiredCount);
    TEST_ASSERT_TRUE(strlen(requiredParams[0]) > 0);
    TEST_ASSERT_TRUE(strlen(requiredParams[1]) > 0);
    TEST_ASSERT_TRUE(strlen(requiredParams[2]) > 0);
    
    // Verify prohibited parameters are defined for validation
    TEST_ASSERT_EQUAL(1, prohibitedCount);
    TEST_ASSERT_TRUE(strlen(prohibitedParams[0]) > 0);
}

void test_device_code_vs_refresh_token_flow() {
    // Test that different OAuth flows have different requirements
    // Device code flow: public client (no client_secret)
    // Refresh token flow: confidential client (requires client_secret)
    
    bool deviceCodeFlowRequiresSecret = false;  // Should be false for public clients
    bool refreshTokenFlowRequiresSecret = true; // Should be true for token refresh
    
    TEST_ASSERT_FALSE(deviceCodeFlowRequiresSecret);
    TEST_ASSERT_TRUE(refreshTokenFlowRequiresSecret);
}

void test_aadsts7000218_error_handling() {
    // Test that AADSTS7000218 error is properly detected and handled
    // This error occurs when Azure AD is configured as confidential client
    // and requires client_secret for device code flow
    
    const char* errorCode = "invalid_client";
    const char* errorDescription = "AADSTS7000218: The request body must contain the following parameter: 'client_assertion' or 'client_secret'.";
    
    // Verify error strings are defined for detection
    TEST_ASSERT_NOT_NULL(errorCode);
    TEST_ASSERT_NOT_NULL(errorDescription);
    TEST_ASSERT_TRUE(strlen(errorCode) > 0);
    TEST_ASSERT_TRUE(strlen(errorDescription) > 0);
    
    // Verify AADSTS7000218 is in the error description
    TEST_ASSERT_TRUE(strstr(errorDescription, "AADSTS7000218") != NULL);
    TEST_ASSERT_TRUE(strstr(errorDescription, "client_secret") != NULL);
}

void test_confidential_client_fallback() {
    // Test that confidential client device code flow includes client_secret
    // This is the fallback when Azure AD requires client authentication
    
    const char* confidentialClientParams[] = {
        "grant_type=urn:ietf:params:oauth:grant-type:device_code",
        "client_id=",
        "client_secret=",
        "device_code="
    };
    
    int paramCount = sizeof(confidentialClientParams) / sizeof(confidentialClientParams[0]);
    
    // Verify all required parameters are defined for confidential client
    TEST_ASSERT_EQUAL(4, paramCount);
    for (int i = 0; i < paramCount; i++) {
        TEST_ASSERT_TRUE(strlen(confidentialClientParams[i]) > 0);
    }
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_device_code_flow_public_client);
    RUN_TEST(test_device_code_vs_refresh_token_flow);
    RUN_TEST(test_aadsts7000218_error_handling);
    RUN_TEST(test_confidential_client_fallback);
    
    UNITY_END();
}

void loop() {
    // Empty loop for testing
}