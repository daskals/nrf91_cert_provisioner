# nRF91 Certificate Provisioner

A simple utility to provision AWS IoT certificates (Root CA, device certificate, and private key) into the nRF9151 modem's secure storage. Run this once per device before deploying your main application. Certificates are stored using the modem's secure key management and persist across firmware updates.

## Supported Boards

- `nrf9151dk/nrf9151/ns` (nRF9151 DK)
- `conexio_stratus_pro/nrf9151/ns` (Conexio Stratus Pro)
- Any nRF91-series board with modem firmware

## Usage

### 1. Download AWS IoT Certificates

From the AWS IoT Console, download these three files:
- **Root CA certificate** (`AmazonRootCA1.pem`)
- **Device certificate** (`xxxxx-certificate.pem.crt`)
- **Private key** (`xxxxx-private.pem.key`)

### 2. Place in `certificates/` Folder

```
nrf91_cert_provisioner/
├── certificates/
│   ├── AmazonRootCA1.pem           <- Root CA
│   ├── xxx-certificate.pem.crt     <- Device cert
│   └── xxx-private.pem.key         <- Private key
```

The build system auto-detects files by pattern:

| Type | Pattern |
|---|---|
| CA certificate | `*CA*.pem` or `*root*.pem` |
| Device certificate | `*certificate*.crt` or `*.pem` |
| Private key | `*private*.key` or `*.pem` |

### 3. Expected Output

After flashing, connect a serial terminal (115200 baud):

```
── Certificate Provisioning (sec_tag 16) ──
  CA cert: Provisioned OK
  Device cert: Provisioned OK
  Private key: Provisioned OK

All certificates provisioned successfully!
```

> **Note:** The private key may show "Mismatch, updating..." even if unchanged. This is normal — the modem doesn't allow reading back private keys for security.

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
