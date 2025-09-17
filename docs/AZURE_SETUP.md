# Azure AD Configuration Guide

This guide will walk you through setting up an Azure AD application for the Teams Red Light device.

## Prerequisites

- Access to Azure Portal (portal.azure.com)
- Admin permissions to register applications (or permission from your IT department)
- Office 365/Microsoft 365 account with Teams

## Step-by-Step Setup

### 1. Access Azure Portal

1. Go to [Azure Portal](https://portal.azure.com)
2. Sign in with your organizational or personal Microsoft account
3. Navigate to **Azure Active Directory**

### 2. Register New Application

1. In Azure AD, click on **App registrations** in the left sidebar
2. Click **New registration** at the top
3. Fill in the application details:

   **Name:** `Teams Red Light Device`
   
   **Supported account types:** Choose one:
   - **Single tenant**: Only your organization (most restrictive)
   - **Multitenant**: Any organization directory (more flexible)
   - **Personal accounts**: Include personal Microsoft accounts
   
   **Redirect URI:**
   - Platform: Web
   - URI: `http://192.168.4.1/callback` (default ESP32 AP IP)
   
   > Note: You'll update this with your actual ESP32 IP after configuration

4. Click **Register**

### 3. Configure API Permissions

1. In your newly created app, go to **API permissions**
2. Click **Add a permission**
3. Choose **Microsoft Graph**
4. Select **Delegated permissions**
5. Search for and select: **`Presence.Read`**
6. Click **Add permissions**

**Important:** If your organization requires admin consent:
7. Click **Grant admin consent for [Your Organization]**
8. Confirm by clicking **Yes**

### 4. Create Client Secret

1. Go to **Certificates & secrets**
2. Click **New client secret**
3. Enter description: `Teams Red Light Device Secret`
4. Choose expiration (recommend 24 months for balance of security and convenience)
5. Click **Add**
6. **Copy the secret value immediately** - you won't be able to see it again!

### 5. Note Important Values

Copy these values for ESP32 configuration:

| Field | Where to Find | Example |
|-------|---------------|---------|
| **Client ID** | Overview page → Application (client) ID | `12345678-1234-1234-1234-123456789012` |
| **Tenant ID** | Overview page → Directory (tenant) ID | `87654321-4321-4321-4321-210987654321` |
| **Client Secret** | The value you copied in step 4 | `abc123def456...` |

### 6. Update Redirect URI (After ESP32 Setup)

After you've configured your ESP32 and it's connected to your WiFi:

1. Find your ESP32's IP address (shown in web interface)
2. Go back to your Azure AD app registration
3. Go to **Authentication**
4. Update the redirect URI to: `http://[ESP32-IP]/callback`
5. For example: `http://192.168.1.100/callback`

## Security Best Practices

### Client Secret Management

✅ **Do:**
- Store client secret securely
- Use minimum required expiration time
- Rotate secrets before expiration
- Never share secrets in plain text

❌ **Don't:**
- Store secrets in version control
- Share secrets via email or chat
- Use secrets that never expire
- Reuse secrets across multiple applications

### Application Permissions

✅ **Do:**
- Use minimum required permissions (`Presence.Read` only)
- Review permissions regularly
- Document why each permission is needed

❌ **Don't:**
- Request excessive permissions
- Use application permissions unless absolutely necessary
- Grant permissions without understanding their impact

### Network Security

✅ **Do:**
- Use HTTPS where possible
- Keep ESP32 firmware updated
- Monitor for security updates

❌ **Don't:**
- Expose device to public internet unnecessarily
- Use default passwords
- Ignore security warnings

## Common Issues and Solutions

### Permission Denied Errors

**Problem:** Authentication succeeds but presence API returns 403 Forbidden

**Solutions:**
1. Ensure `Presence.Read` permission is added and consented
2. Check if admin consent is required for your organization
3. Verify the user account has Teams enabled

### Invalid Redirect URI

**Problem:** OAuth flow fails with redirect URI mismatch

**Solutions:**
1. Ensure redirect URI in Azure matches ESP32 IP exactly
2. Check that ESP32 is accessible at the configured IP
3. Try using `http://` instead of `https://` for local network

### Token Refresh Issues

**Problem:** Device works initially but stops working after some time

**Solutions:**
1. Check client secret hasn't expired
2. Verify refresh token functionality in device logs
3. Re-authenticate through device web interface

### Corporate Network Issues

**Problem:** Device can't reach Microsoft Graph API

**Solutions:**
1. Check if corporate firewall blocks external HTTPS
2. Verify DNS resolution for graph.microsoft.com
3. Contact IT department about allowing Microsoft Graph API access

## Testing Your Configuration

### 1. Quick Test in Browser

Before configuring ESP32, test your Azure AD app:

1. Create test URL:
   ```
   https://login.microsoftonline.com/[TENANT-ID]/oauth2/v2.0/authorize?client_id=[CLIENT-ID]&response_type=code&redirect_uri=http://localhost&scope=https://graph.microsoft.com/Presence.Read
   ```

2. Replace `[TENANT-ID]` and `[CLIENT-ID]` with your values
3. Open URL in browser
4. Should see Microsoft login page
5. After login, should redirect to localhost with authorization code

### 2. Test API Access

After getting authorization code, you can test API access using tools like Postman or curl.

## Organizational Considerations

### IT Department Approval

Many organizations require approval for new app registrations:

1. **Prepare justification**: Explain the productivity benefit
2. **Security review**: Highlight minimal permissions required
3. **Data handling**: Explain that only presence status is accessed
4. **Network impact**: Minimal traffic to Microsoft Graph API

### Multi-User Deployment

For deploying multiple devices:

1. **Single app registration**: Can be shared across multiple devices
2. **User-specific authentication**: Each device authenticates as individual user
3. **Centralized management**: Consider Azure AD Conditional Access policies

### Compliance Requirements

Consider your organization's compliance needs:

1. **Data residency**: Teams presence data stays in Microsoft cloud
2. **Audit logging**: Azure AD logs all authentication events
3. **Access reviews**: Regular review of app permissions and users

## Alternative Authentication Methods

### Device Code Flow (Future Enhancement)

For environments where web browser access is limited:
- Device displays code on screen
- User enters code on separate device
- More secure for public environments

### Certificate-Based Authentication

For higher security environments:
- Use X.509 certificates instead of client secrets
- Eliminates need for secret management
- Requires more complex setup

These advanced authentication methods could be added in future firmware versions based on user needs.