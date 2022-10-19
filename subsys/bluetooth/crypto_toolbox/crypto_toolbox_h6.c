#include <zephyr/bluetooth/crypto_toolbox/aes_cmac.h>
#include <zephyr/bluetooth/crypto_toolbox/h6.h>
#include <zephyr/sys/byteorder.h>

#define BT_DBG_ENABLED	IS_ENABLED(CONFIG_BT_DEBUG_CRYPTO_TOOLBOX)
#define LOG_MODULE_NAME bt_crypto_toolbox_h6
#include "common/log.h"

int bt_crypto_toolbox_h6(const uint8_t w[16], const uint8_t key_id[4], uint8_t res[16])
{
	uint8_t ws[16];
	uint8_t key_id_s[4];
	int err;

	BT_DBG("w %s", bt_hex(w, 16));
	BT_DBG("key_id %s", bt_hex(key_id, 4));

	sys_memcpy_swap(ws, w, 16);
	sys_memcpy_swap(key_id_s, key_id, 4);

	err = bt_crypto_toolbox_aes_cmac(ws, key_id_s, 4, res);
	if (err) {
		return err;
	}

	BT_DBG("res %s", bt_hex(res, 16));

	sys_mem_swap(res, 16);

	return 0;
}
