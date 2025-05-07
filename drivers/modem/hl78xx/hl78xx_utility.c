#include <zephyr/modem/chat.h>
#include <zephyr/modem/backend/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/offloaded_netdev.h>
#include <zephyr/net/socket_offload.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <string.h>
#include <stdlib.h>
#include "hl78xx.h"

LOG_MODULE_REGISTER(hl78xx_utility, CONFIG_MODEM_LOG_LEVEL);

/**
 * @brief  Convert string to long integer, but handle errors
 *
 * @param  s: string with representation of integer number
 * @param  err_value: on error return this value instead
 * @param  desc: name the string being converted
 * @param  func: function where this is called (typically __func__)
 *
 * @retval return integer conversion on success, or err_value on error
 */
int modem_atoi(const char *s, const int err_value, const char *desc, const char *func)
{
	int ret;
	char *endptr;

	ret = (int)strtol(s, &endptr, 10);
	if (!endptr || *endptr != '\0') {
		LOG_ERR("bad %s '%s' in %s", s, desc, func);
		return err_value;
	}

	return ret;
}

bool modem_cellular_is_registered(struct modem_hl78xx_data *data)
{
	return (data->mdm_registration_status.network_state ==
		CELLULAR_REGISTRATION_REGISTERED_HOME) ||
	       (data->mdm_registration_status.network_state ==
		CELLULAR_REGISTRATION_REGISTERED_ROAMING);
}

#define HASH_MULTIPLIER 37
uint32_t hash32(const char *str, int len)
{
	uint32_t h = 0;
	int i;

	for (i = 0; i < len; ++i) {
		h = (h * HASH_MULTIPLIER) + str[i];
	}

	return h;
}

/**
 * Portable memmem() replacement for C99.
 */
void *c99_memmem(const void *haystack, size_t haystacklen, const void *needle, size_t needlelen)
{
	if (!haystack || !needle || needlelen == 0 || haystacklen < needlelen) {
		return NULL;
	}

	const uint8_t *h = haystack;

	for (size_t i = 0; i <= haystacklen - needlelen; i++) {
		if (memcmp(h + i, needle, needlelen) == 0) {
			return (void *)(h + i);
		}
	}

	return NULL;
}

#if defined(CONFIG_MODEM_HL78XX_APN_SOURCE_ICCID) || defined(CONFIG_MODEM_HL78XX_APN_SOURCE_IMSI)
int find_apn(char *apn, int apnlen, const char *profiles, const char *associated_number)
{
	int rc = -1;

	/* try to find a match */
	char *s = strstr(profiles, associated_number);

	if (s) {
		char *eos;
		/* find the assignment operator preceding the match */
		while (s >= profiles && !strchr("=", *s)) {
			s--;
		}
		/* find the apn preceding the assignment operator */
		while (s >= profiles && strchr(" =", *s)) {
			s--;
		}
		/* mark end of apn string */
		eos = s + 1;

		/* find first character of the apn */
		while (s >= profiles && !strchr(" ,", *s)) {
			s--;
		}
		s++;
		/* copy the key */
		if (s >= profiles) {
			int len = eos - s;

			if (len < apnlen) {
				memcpy(apn, s, len);
				apn[len] = '\0';
				rc = 0;
			} else {
				LOG_ERR("buffer overflow");
			}
		}
	}
	return rc;
}

/* try to detect APN automatically, based on IMSI / ICCID */
int modem_detect_apn(struct modem_hl78xx_data *data, const char *associated_number)
{
	int rc = -1;

	if (associated_number != NULL && strlen(associated_number) >= 5) {
/* extract MMC and MNC from IMSI */
#if defined(CONFIG_MODEM_HL78XX_APN_SOURCE_IMSI)
		char mmcmnc[6] = {0};
#else
		char mmcmnc[8] = {0};
#endif
		*mmcmnc = 0;
		strncat(mmcmnc, associated_number, sizeof(mmcmnc) - 1);
		/* try to find a matching IMSI, and assign the APN */
		rc = find_apn(data->mdm_apn, sizeof(data->mdm_apn),
			      CONFIG_MODEM_HL78XX_APN_PROFILES, mmcmnc);
		if (rc < 0) {
			rc = find_apn(data->mdm_apn, sizeof(data->mdm_apn),
				      CONFIG_MODEM_HL78XX_APN_PROFILES, "*");
		}
	}

	if (rc == 0) {
		LOG_INF("Assign APN: \"%s\"", data->mdm_apn);
	} else {
		LOG_INF("No assigned APN: \"%d\"", rc);
	}

	return rc;
}
#endif
