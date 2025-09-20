# OAuth Device Code Flow Fix for AADSTS7000218

## Issue Description

The error `AADSTS7000218: The request body must contain the following parameter: 'client_assertion' or 'client_secret'` occurs when the Azure AD application is configured as a **confidential client** rather than a **public client**.

## Root Cause

Microsoft Azure AD applications can be configured in two ways:

1. **Public Client** - Designed for devices that cannot securely store secrets (like IoT devices, mobile apps)
2. **Confidential Client** - Designed for applications that can securely store secrets (like web servers)

When an Azure AD application is configured as a confidential client, Microsoft requires client authentication (via `client_secret` or `client_assertion`) even for device code flows.

## Solution

The firmware now implements a **graceful fallback mechanism**:

1. **First attempt**: Try device code flow as a public client (without `client_secret`)
2. **If AADSTS7000218 error occurs**: Automatically retry with `client_secret` for confidential client authentication

## Code Changes

### Enhanced Error Handling

The `pollDeviceCodeToken()` function now detects the specific AADSTS7000218 error and falls back to confidential client authentication:

```cpp
} else if (error == "invalid_client" && doc.containsKey("error_description")) {
  String errorDesc = doc["error_description"].as<String>();
  if (errorDesc.indexOf("AADSTS7000218") >= 0 && clientSecret.length() > 0) {
    LOG_WARN("Azure AD requires client authentication for device code flow - retrying with client_secret");
    http.end();
    return pollDeviceCodeTokenWithSecret();
  }
}
```

### New Function: pollDeviceCodeTokenWithSecret()

A new function handles device code token requests for confidential clients:

```cpp
String postData = "grant_type=urn:ietf:params:oauth:grant-type:device_code";
postData += "&client_id=" + clientId;
postData += "&client_secret=" + clientSecret;  // Added for confidential clients
postData += "&device_code=" + deviceCode;
```

## Azure AD Configuration

### Option 1: Configure as Public Client (Recommended)

To avoid needing client secrets, configure your Azure AD application as a public client:

1. Go to Azure Portal → Azure Active Directory → App registrations
2. Select your application
3. Go to **Authentication**
4. Under **Advanced settings**, enable **"Allow public client flows"**
5. Save the configuration

With this setting, the device code flow will work without requiring `client_secret`.

### Option 2: Use as Confidential Client

If your organization requires confidential client configuration:

1. Ensure a client secret is created and stored securely
2. Configure the client secret in the ESP32 device
3. The firmware will automatically detect the AADSTS7000218 error and use the secret

## Benefits

- **Automatic compatibility** with both public and confidential Azure AD applications
- **No manual configuration** required - the device automatically adapts
- **Minimal code changes** - existing functionality remains unchanged
- **Backward compatibility** - works with existing Azure AD setups
- **Security-first approach** - tries public client first, falls back to confidential client only when needed

## Testing

The fix includes comprehensive tests in `test/test_device_code_fix.cpp`:

- `test_device_code_flow_public_client()` - Validates public client parameters
- `test_aadsts7000218_error_handling()` - Tests error detection logic
- `test_confidential_client_fallback()` - Validates confidential client parameters

## Troubleshooting

### Still getting AADSTS7000218 errors?

1. **Check client secret**: Ensure the client secret is correctly configured
2. **Verify secret expiry**: Client secrets expire and need renewal
3. **Test Azure AD config**: Try enabling "Allow public client flows" in Azure AD
4. **Check logs**: Enable debug logging to see which authentication method is being used

### No client secret available

If you get "No client secret available for confidential client authentication":

1. Either configure a client secret in the ESP32 settings
2. Or enable "Allow public client flows" in Azure AD to use public client authentication

## Log Messages

The fix adds specific log messages to help with troubleshooting:

- `"Azure AD requires client authentication for device code flow - retrying with client_secret"`
- `"Polling for device code token with client secret (confidential client)"`
- `"Device code authentication with secret successful!"`
- `"No client secret available for confidential client authentication"`

## Security Considerations

- The client secret is only used when specifically required by Azure AD
- Public client authentication is always attempted first (more secure for IoT devices)
- Client secrets are stored securely in ESP32 flash memory
- The fallback mechanism prevents authentication failures due to Azure AD configuration