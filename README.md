# nRF91 Certificate Provisioner

Standalone tool for writing AWS IoT certificates to the nRF9151 modem secure storage. Flash this once per device before running your main application.

## Supported Boards

- `nrf9151dk/nrf9151/ns` (nRF9151 DK)
- `conexio_stratus_pro/nrf9151/ns` (Conexio Stratus Pro)
- Any nRF91-based board with modem firmware

## Quick Start

### 1. Download your AWS IoT certificates

From the AWS IoT Console, download the three files when creating a Thing:
- **Root CA certificate** (e.g., `AmazonRootCA1.pem`)
- **Device certificate** (e.g., `xxxxx-certificate.pem.crt`)
- **Private key** (e.g., `xxxxx-private.pem.key`)

### 2. Place certificates in the `certificates/` folder

```
nrf91_cert_provisioner/
├── certificates/
│   ├── AmazonRootCA1.pem                          <- CA certificate
│   ├── e47f26bb...-certificate.pem.crt             <- Device certificate
│   └── e47f26bb...-private.pem.key                 <- Private key
├── src/
├── CMakeLists.txt
├── prj.conf
└── ...
```

The build system auto-detects files by name pattern:

| Certificate Type | File Pattern |
|---|---|
| CA certificate | `*CA*.pem` or `*root*.pem` |
| Device certificate | `*certificate*.crt` or `*certificate*.pem` |
| Private key | `*private*.key` or `*private*.pem` |

### 3. Build and flash

**nRF9151 DK:**
```bash
west build --board nrf9151dk/nrf9151/ns
west flash
```

**Conexio Stratus Pro:**
```bash
west build --board conexio_stratus_pro/nrf9151/ns -- -DBOARD_ROOT=c:/ncs/v2.9.0/conexio-firmware-sdk
west flash
```

### 4. Check the output

Connect a serial terminal (115200 baud) and you should see:

```
Starting nRF91 Certificate Provisioner...

========================================
  nRF91 Certificate Provisioner
========================================

Modem firmware: mfw_nrf91x1_2.0.2
Hardware:       nRF9151 LACA A1
IMEI:           352656100012345

── Certificate Provisioning (sec_tag 16) ──
  CA cert: Provisioned OK
  Device cert: Provisioned OK
  Private key: Provisioned OK

All certificates provisioned successfully!
You can now flash the main gateway application.

========================================
  Done. Device is ready.
========================================
```

If certificates were already provisioned and match, you'll see:
```
  CA cert: Already provisioned (match)
  Device cert: Already provisioned (match)
  Private key: Mismatch, updating...
  Private key: Provisioned OK
```
> Note: Private key always shows "Mismatch" because the modem does not allow reading back private keys for comparison. This is normal and secure.

### 5. Flash your main application

Once provisioning is complete, flash your main application (e.g., the gateway firmware). The certificates persist in the modem's secure storage across firmware updates.

## Build-Time Validation

The build system automatically checks your certificate files for common mistakes:

| Check | Error Message |
|---|---|
| `certificates/` folder missing | `certificates/ folder not found!` |
| Any of the 3 files missing | `Certificate files missing in .../certificates/` |
| Multiple files matching same pattern | Warning: `Using first match only` |
| File is empty or too small | `file appears empty or too small` |
| File is not PEM format (binary DER) | `does not contain a valid PEM header` |
| Private key file is actually a certificate | `does not contain 'PRIVATE KEY' header` |
| Device cert file is actually a key | `does not contain 'CERTIFICATE' header` |
| CA cert and device cert are the same file | `CA certificate and device certificate are identical!` |

## Configuration

In `prj.conf`:

| Config | Default | Description |
|---|---|---|
| `CONFIG_MQTT_HELPER_SEC_TAG` | `16` | Security tag for storing certificates in modem |

The security tag must match the one used in your main application.

## Security

- The `certificates/` folder and generated `src/certs_credentials.h` are in `.gitignore`
- **Never commit certificate files or private keys to version control**
- After provisioning, certificates are stored in the modem's internal secure storage (NVM)
- The modem does not allow reading back private keys
- For production devices, consider using Nordic's secure provisioning tools instead

## Project Structure

```
nrf91_cert_provisioner/
├── certificates/              <- Your AWS IoT PEM files (gitignored)
├── src/
│   ├── main.c                 <- Modem init, print info, run provisioning
│   ├── cert_provision.c       <- Certificate write logic
│   ├── cert_provision.h       <- API header
│   └── certs_credentials.h    <- Auto-generated at build time (gitignored)
├── CMakeLists.txt             <- Build config + cert auto-import + validation
├── Kconfig                    <- Security tag configuration
├── prj.conf                   <- Minimal modem config
├── .gitignore
└── README.md
```

## Requirements

- nRF Connect SDK (NCS) v2.6.0 or later
- Zephyr SDK 0.17.0 or later
- nRF91-series device with modem firmware
