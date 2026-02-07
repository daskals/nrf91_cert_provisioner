#
# nRF91 Certificate Provisioner
# Standalone tool for writing AWS IoT certificates to modem secure storage
#

cmake_minimum_required(VERSION 3.8.2)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrf91_cert_provisioner)

target_sources(app PRIVATE
    src/main.c
    src/cert_provision.c
)

zephyr_include_directories(src)

# ============================================================================
# Certificate auto-import: reads PEM files from certificates/ folder and
# generates src/certs_credentials.h at build time
# ============================================================================

set(CERTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/certificates)
set(CREDS_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/certs_credentials.h)

# Check that certificates/ folder exists
if(NOT EXISTS ${CERTS_DIR})
    message(FATAL_ERROR
        "certificates/ folder not found!\n"
        "  Create the folder and place your AWS IoT PEM files inside:\n"
        "    mkdir certificates\n"
        "  Required files:\n"
        "    - CA certificate:     *CA*.pem or *root*.pem\n"
        "    - Device certificate: *certificate*.crt or *certificate*.pem\n"
        "    - Private key:        *private*.key or *private*.pem")
endif()

# Find certificate files by pattern
file(GLOB CA_CERT_FILES "${CERTS_DIR}/*CA*.pem" "${CERTS_DIR}/*root*.pem")
file(GLOB DEVICE_CERT_FILES "${CERTS_DIR}/*certificate*.crt" "${CERTS_DIR}/*certificate*.pem")
file(GLOB PRIVATE_KEY_FILES "${CERTS_DIR}/*private*.key" "${CERTS_DIR}/*private*.pem")

list(LENGTH CA_CERT_FILES CA_COUNT)
list(LENGTH DEVICE_CERT_FILES DEV_COUNT)
list(LENGTH PRIVATE_KEY_FILES KEY_COUNT)

# --- Check: all three certificate types must be present ---
set(_MISSING "")
if(CA_COUNT EQUAL 0)
    string(APPEND _MISSING "  - CA certificate:     missing (*CA*.pem or *root*.pem)\n")
endif()
if(DEV_COUNT EQUAL 0)
    string(APPEND _MISSING "  - Device certificate: missing (*certificate*.crt or *certificate*.pem)\n")
endif()
if(KEY_COUNT EQUAL 0)
    string(APPEND _MISSING "  - Private key:        missing (*private*.key or *private*.pem)\n")
endif()
if(NOT _MISSING STREQUAL "")
    message(FATAL_ERROR
        "Certificate files missing in ${CERTS_DIR}/\n"
        "${_MISSING}"
        "  Download your AWS IoT certificates and place them in the certificates/ folder.")
endif()

# --- Check: warn if multiple files match the same pattern ---
if(CA_COUNT GREATER 1)
    message(WARNING "Multiple CA certificates found (${CA_COUNT}). Using first match only.")
endif()
if(DEV_COUNT GREATER 1)
    message(WARNING "Multiple device certificates found (${DEV_COUNT}). Using first match only.")
endif()
if(KEY_COUNT GREATER 1)
    message(WARNING "Multiple private keys found (${KEY_COUNT}). Using first match only.")
endif()

# Take only the first match from each glob
list(GET CA_CERT_FILES 0 CA_CERT_FILE)
list(GET DEVICE_CERT_FILES 0 DEVICE_CERT_FILE)
list(GET PRIVATE_KEY_FILES 0 PRIVATE_KEY_FILE)

# --- Check: files must not be empty ---
file(READ ${CA_CERT_FILE} _CA_RAW)
file(READ ${DEVICE_CERT_FILE} _DEV_RAW)
file(READ ${PRIVATE_KEY_FILE} _KEY_RAW)

string(LENGTH "${_CA_RAW}" _CA_LEN)
string(LENGTH "${_DEV_RAW}" _DEV_LEN)
string(LENGTH "${_KEY_RAW}" _KEY_LEN)

if(_CA_LEN LESS 50)
    message(FATAL_ERROR "CA certificate file appears empty or too small (${_CA_LEN} bytes): ${CA_CERT_FILE}")
endif()
if(_DEV_LEN LESS 50)
    message(FATAL_ERROR "Device certificate file appears empty or too small (${_DEV_LEN} bytes): ${DEVICE_CERT_FILE}")
endif()
if(_KEY_LEN LESS 50)
    message(FATAL_ERROR "Private key file appears empty or too small (${_KEY_LEN} bytes): ${PRIVATE_KEY_FILE}")
endif()

# --- Check: files must contain valid PEM headers ---
string(FIND "${_CA_RAW}" "-----BEGIN" _CA_HAS_PEM)
string(FIND "${_DEV_RAW}" "-----BEGIN" _DEV_HAS_PEM)
string(FIND "${_KEY_RAW}" "-----BEGIN" _KEY_HAS_PEM)

if(_CA_HAS_PEM LESS 0)
    message(FATAL_ERROR
        "CA certificate file does not contain a valid PEM header (-----BEGIN ...):\n"
        "  ${CA_CERT_FILE}\n"
        "  Make sure it is a PEM-encoded file, not DER binary.")
endif()
if(_DEV_HAS_PEM LESS 0)
    message(FATAL_ERROR
        "Device certificate file does not contain a valid PEM header (-----BEGIN ...):\n"
        "  ${DEVICE_CERT_FILE}\n"
        "  Make sure it is a PEM-encoded file, not DER binary.")
endif()
if(_KEY_HAS_PEM LESS 0)
    message(FATAL_ERROR
        "Private key file does not contain a valid PEM header (-----BEGIN ...):\n"
        "  ${PRIVATE_KEY_FILE}\n"
        "  Make sure it is a PEM-encoded file, not DER binary.")
endif()

# --- Check: private key must actually be a key, not a certificate ---
string(FIND "${_KEY_RAW}" "PRIVATE KEY" _KEY_HAS_KEY)
if(_KEY_HAS_KEY LESS 0)
    message(FATAL_ERROR
        "Private key file does not contain 'PRIVATE KEY' header:\n"
        "  ${PRIVATE_KEY_FILE}\n"
        "  This file might be a certificate instead of a private key.")
endif()

# --- Check: device cert must be a certificate, not a key ---
string(FIND "${_DEV_RAW}" "CERTIFICATE" _DEV_HAS_CERT)
if(_DEV_HAS_CERT LESS 0)
    message(FATAL_ERROR
        "Device certificate file does not contain 'CERTIFICATE' header:\n"
        "  ${DEVICE_CERT_FILE}\n"
        "  This file might be a private key instead of a certificate.")
endif()

# --- Check: CA and device cert must not be identical ---
if("${_CA_RAW}" STREQUAL "${_DEV_RAW}")
    message(FATAL_ERROR
        "CA certificate and device certificate are identical!\n"
        "  CA:     ${CA_CERT_FILE}\n"
        "  Device: ${DEVICE_CERT_FILE}\n"
        "  These must be different files.")
endif()

message(STATUS "")
message(STATUS "Certificate auto-import:")
message(STATUS "  CA cert:     ${CA_CERT_FILE}")
message(STATUS "  Device cert: ${DEVICE_CERT_FILE}")
message(STATUS "  Private key: ${PRIVATE_KEY_FILE}")
message(STATUS "  All checks passed")

# ============================================================================
# Convert PEM files to C string literals
# ============================================================================

foreach(CERT_NAME CA_CERT DEVICE_CERT PRIVATE_KEY)
    file(READ ${${CERT_NAME}_FILE} _PEM)
    # Remove carriage returns, split into lines
    string(REPLACE "\r" "" _PEM "${_PEM}")
    string(REPLACE "\n" ";" _LINES "${_PEM}")
    # Build C string array from non-empty lines
    set(_C "")
    foreach(_LINE IN LISTS _LINES)
        if(NOT "${_LINE}" STREQUAL "")
            string(APPEND _C "\t\"${_LINE}\\n\"\n")
        endif()
    endforeach()
    # Remove trailing newline from result
    string(REGEX REPLACE "\n$" "" ${CERT_NAME}_C "${_C}")
endforeach()

# Write the generated header
file(WRITE ${CREDS_HEADER}
"/*
 * AUTO-GENERATED by CMakeLists.txt from certificates/ folder
 * DO NOT edit manually - DO NOT commit to version control!
 */

#ifndef CERTS_CREDENTIALS_H_
#define CERTS_CREDENTIALS_H_

/* Amazon Root CA */
static const char ca_cert[] =
${CA_CERT_C};

/* Device certificate */
static const char device_cert[] =
${DEVICE_CERT_C};

/* Private key */
static const char private_key[] =
${PRIVATE_KEY_C};

#endif /* CERTS_CREDENTIALS_H_ */
")

message(STATUS "  Generated: ${CREDS_HEADER}")
message(STATUS "")
