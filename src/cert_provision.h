/*
 * Certificate Provisioning Module
 * Writes AWS IoT certificates to modem secure storage
 */

#ifndef CERT_PROVISION_H_
#define CERT_PROVISION_H_

/**
 * @brief Provision AWS IoT certificates to modem
 *
 * Checks if certificates already exist in modem storage.
 * If not, writes CA cert, device cert, and private key
 * to the security tag defined in CONFIG_MQTT_HELPER_SEC_TAG.
 *
 * Must be called after nrf_modem_lib_init().
 * Modem will be set to offline mode (CFUN=4) internally.
 *
 * @return 0 on success, negative errno on error
 */
int cert_provision(void);

#endif /* CERT_PROVISION_H_ */
