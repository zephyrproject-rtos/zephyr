#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/crypto_toolbox/g2.h>

#include <zephyr/bluetooth/crypto_toolbox/aes_cmac.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_CRYPTO_TOOLBOX)
#define LOG_MODULE_NAME bt_crypto_toolbox_g2
#include "common/log.h"

int bt_crypto_toolbox_g2(const uint8_t u[32], const uint8_t v[32],
		                 const uint8_t x[16], const uint8_t y[16], 
                         uint32_t *passkey)
{
	uint8_t m[80], xs[16];
	int err;

	BT_DBG("u %s", bt_hex(u, 32));
	BT_DBG("v %s", bt_hex(v, 32));
	BT_DBG("x %s", bt_hex(x, 16));
	BT_DBG("y %s", bt_hex(y, 16));

	sys_memcpy_swap(m, u, 32);
	sys_memcpy_swap(m + 32, v, 32);
	sys_memcpy_swap(m + 64, y, 16);

	sys_memcpy_swap(xs, x, 16);

	/* reuse xs (key) as buffer for result */
	err = bt_crypto_toolbox_aes_cmac(xs, m, sizeof(m), xs);
	if (err) {
		return err;
	}
	BT_DBG("res %s", bt_hex(xs, 16));

	memcpy(passkey, xs + 12, 4);
	*passkey = sys_be32_to_cpu(*passkey) % 1000000;

	BT_DBG("passkey %u", *passkey);

	return 0;
}
