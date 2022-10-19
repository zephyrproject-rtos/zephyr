#include <zephyr/bluetooth/crypto_toolbox/aes_cmac.h>
#include <zephyr/bluetooth/crypto_toolbox/f6.h>
#include <zephyr/sys/byteorder.h>

#define BT_DBG_ENABLED	IS_ENABLED(CONFIG_BT_DEBUG_CRYPTO_TOOLBOX)
#define LOG_MODULE_NAME bt_crypto_toolbox_f6
#include "common/log.h"

int bt_crypto_toolbox_f6(const uint8_t *w, const uint8_t *n1, const uint8_t *n2, const uint8_t *r,
			 const uint8_t *iocap, const bt_addr_le_t *a1, const bt_addr_le_t *a2,
			 uint8_t *check)
{
	uint8_t ws[16];
	uint8_t m[65];
	int err;

	BT_DBG("w %s", bt_hex(w, 16));
	BT_DBG("n1 %s", bt_hex(n1, 16));
	BT_DBG("n2 %s", bt_hex(n2, 16));
	BT_DBG("r %s", bt_hex(r, 16));
	BT_DBG("io_cap %s", bt_hex(iocap, 3));
	BT_DBG("a1 %s", bt_hex(a1, 7));
	BT_DBG("a2 %s", bt_hex(a2, 7));

	sys_memcpy_swap(m, n1, 16);
	sys_memcpy_swap(m + 16, n2, 16);
	sys_memcpy_swap(m + 32, r, 16);
	sys_memcpy_swap(m + 48, iocap, 3);

	m[51] = a1->type;
	memcpy(m + 52, a1->a.val, 6);
	sys_memcpy_swap(m + 52, a1->a.val, 6);

	m[58] = a2->type;
	memcpy(m + 59, a2->a.val, 6);
	sys_memcpy_swap(m + 59, a2->a.val, 6);

	sys_memcpy_swap(ws, w, 16);

	err = bt_crypto_toolbox_aes_cmac(ws, m, sizeof(m), check);
	if (err) {
		return err;
	}

	BT_DBG("res %s", bt_hex(check, 16));

	sys_mem_swap(check, 16);

	return 0;
}