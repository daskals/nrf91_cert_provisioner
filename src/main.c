/*
 * nRF91 Certificate Provisioner
 * Writes AWS IoT certificates to modem secure storage and prints modem info.
 * Flash this once per device, then switch to the main application.
 */

#include <zephyr/kernel.h>
#include <modem/nrf_modem_lib.h>
#include <modem/modem_info.h>
#include <nrf_modem_at.h>

#include "cert_provision.h"

static void print_modem_info(void)
{
	int err;
	char buf[128];

	printk("\n");
	printk("========================================\n");
	printk("  nRF91 Certificate Provisioner\n");
	printk("========================================\n\n");

	/* Modem firmware version */
	err = modem_info_init();
	if (err) {
		printk("Modem info init failed: %d\n", err);
	} else {
		err = modem_info_string_get(MODEM_INFO_FW_VERSION, buf,
					    sizeof(buf));
		if (err > 0) {
			printk("Modem firmware: %s\n", buf);
		} else {
			printk("Modem firmware: could not read (%d)\n", err);
		}
	}

	/* Hardware version via AT command */
	err = nrf_modem_at_cmd(buf, sizeof(buf), "AT%%HWVERSION");
	if (err == 0) {
		/* Remove trailing \r\n and OK */
		char *end = strstr(buf, "\r");
		if (end) {
			*end = '\0';
		}
		/* Skip "%HWVERSION: " prefix if present */
		char *hw = strstr(buf, ": ");
		printk("Hardware:       %s\n", hw ? hw + 2 : buf);
	}

	/* IMEI */
	err = modem_info_string_get(MODEM_INFO_IMEI, buf, sizeof(buf));
	if (err > 0) {
		printk("IMEI:           %s\n", buf);
	}

	printk("\n");
}

int main(void)
{
	int err;

	printk("Starting nRF91 Certificate Provisioner...\n");

	/* Initialize modem library */
	err = nrf_modem_lib_init();
	if (err) {
		printk("Modem library init failed: %d\n", err);
		return 0;
	}

	/* Print modem information */
	print_modem_info();

	/* Provision certificates */
	err = cert_provision();
	if (err) {
		printk("\nProvisioning FAILED (error %d)\n", err);
		printk("Check that certificate files are correct.\n");
	} else {
		printk("\nAll certificates provisioned successfully!\n");
		printk("You can now flash the main gateway application.\n");
	}

	printk("\n========================================\n");
	printk("  Done. Device is ready.\n");
	printk("========================================\n");

	return 0;
}
