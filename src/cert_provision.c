/*
 * Certificate Provisioning Module
 * Writes AWS IoT certificates to modem secure storage at boot
 */

#include <zephyr/kernel.h>
#include <modem/modem_key_mgmt.h>
#include <nrf_modem_at.h>

#include "cert_provision.h"
#include "certs_credentials.h"

/* Security tag matching CONFIG_MQTT_HELPER_SEC_TAG in prj.conf */
#define SEC_TAG CONFIG_MQTT_HELPER_SEC_TAG

static int provision_credential(enum modem_key_mgmt_cred_type type,
				const char *name, const void *data, size_t len)
{
	int err;
	bool exists;

	err = modem_key_mgmt_exists(SEC_TAG, type, &exists);
	if (err) {
		printk("  %s: Failed to check (%d)\n", name, err);
		return err;
	}

	if (exists) {
		/* Compare with stored credential */
		err = modem_key_mgmt_cmp(SEC_TAG, type, data, len);
		if (err == 0) {
			printk("  %s: Already provisioned (match)\n", name);
			return 0;
		}
		/* Mismatch - delete and re-write */
		printk("  %s: Mismatch, updating...\n", name);
		modem_key_mgmt_delete(SEC_TAG, type);
	}

	err = modem_key_mgmt_write(SEC_TAG, type, data, len);
	if (err) {
		printk("  %s: Failed to write (%d)\n", name, err);
		return err;
	}

	printk("  %s: Provisioned OK\n", name);
	return 0;
}

int cert_provision(void)
{
	int err;
	char response[64];

	printk("── Certificate Provisioning (sec_tag %d) ──\n", SEC_TAG);

	/* Ensure modem is in offline mode before writing credentials */
	err = nrf_modem_at_cmd(response, sizeof(response), "AT+CFUN=4");
	if (err) {
		printk("  Warning: Could not set offline mode (%d)\n", err);
	}

	/* Provision CA certificate */
	err = provision_credential(MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   "CA cert", ca_cert, sizeof(ca_cert) - 1);
	if (err) {
		return err;
	}

	/* Provision device certificate */
	err = provision_credential(MODEM_KEY_MGMT_CRED_TYPE_PUBLIC_CERT,
				   "Device cert", device_cert,
				   sizeof(device_cert) - 1);
	if (err) {
		return err;
	}

	/* Provision private key */
	err = provision_credential(MODEM_KEY_MGMT_CRED_TYPE_PRIVATE_CERT,
				   "Private key", private_key,
				   sizeof(private_key) - 1);
	if (err) {
		return err;
	}

	printk("  Provisioning complete\n");
	return 0;
}
