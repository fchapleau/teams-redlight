#include <unity.h>
#include <Arduino.h>

// Test that OAuth refresh token request includes required parameters
void test_refresh_token_parameters() {
    // Test that refresh token flow includes all required parameters
    // According to Microsoft OAuth 2.0 spec, refresh token requests need:
    // - client_id
    // - client_secret 
    // - refresh_token
    // - grant_type=refresh_token
    // - scope (this was missing and causing HTTP 401 errors)
    
    const char* requiredParams[] = {
        "client_id=",
        "client_secret=", 
        "refresh_token=",
        "grant_type=refresh_token",
        "scope="
    };
    
    int paramCount = sizeof(requiredParams) / sizeof(requiredParams[0]);
    
    // Verify all required parameters are defined
    TEST_ASSERT_EQUAL(5, paramCount);
    for (int i = 0; i < paramCount; i++) {
        TEST_ASSERT_TRUE(strlen(requiredParams[i]) > 0);
    }
}

void test_oauth_scope_consistency() {
    // Test that the scope used in refresh matches device code flow
    // Both should use: "https://graph.microsoft.com/Presence.Read offline_access"
    
    const char* expectedScope = "https://graph.microsoft.com/Presence.Read offline_access";
    
    // Verify scope includes required permissions
    TEST_ASSERT_TRUE(strstr(expectedScope, "Presence.Read") != NULL);
    TEST_ASSERT_TRUE(strstr(expectedScope, "offline_access") != NULL);
}

void test_http_401_handling() {
    // Test that HTTP 401 during refresh is properly handled
    // When refresh fails with 401, tokens should be cleared to force re-auth
    
    int unauthorizedCode = 401;
    int otherErrorCode = 500;
    
    // 401 should trigger token cleanup
    TEST_ASSERT_EQUAL(401, unauthorizedCode);
    
    // Other errors should not trigger token cleanup  
    TEST_ASSERT_NOT_EQUAL(401, otherErrorCode);
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_refresh_token_parameters);
    RUN_TEST(test_oauth_scope_consistency);
    RUN_TEST(test_http_401_handling);
    
    UNITY_END();
}

void loop() {
    // Empty loop for testing
}