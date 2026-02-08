# Certificates Directory

This folder contains your AWS IoT certificates for provisioning to the nRF9151 modem.

## Required Files

Place your actual AWS IoT certificates here (download from AWS IoT Console):

1. **Root CA Certificate** - `AmazonRootCA1.pem`
   - Download from: https://www.amazontrust.com/repository/AmazonRootCA1.pem

2. **Device Certificate** - `xxxxx-certificate.pem.crt`
   - Downloaded when creating your AWS IoT Thing

3. **Private Key** - `xxxxx-private.pem.key`
   - Downloaded when creating your AWS IoT Thing (only available once!)

## File Naming

The build system auto-detects files by pattern:
- CA cert: `*CA*.pem` or `*root*.pem`
- Device cert: `*certificate*.crt` or `*certificate*.pem`
- Private key: `*private*.key` or `*private*.pem`

## Example Files

The `*.example.*` files in this folder are placeholders showing the expected structure.

**To use this project:**
1. Delete or ignore the `.example.*` files
2. Place your actual AWS IoT certificates here
3. Build and flash the provisioner

## Security Warning

⚠️ **NEVER commit real certificates or private keys to git!**

The `.gitignore` is configured to exclude actual certificate files but allow the examples for documentation purposes.
