#include <zephyr/bluetooth/crypto_toolbox/aes_cmac.h>
#include <zephyr/bluetooth/crypto_toolbox/f5.h>
#include <zephyr/sys/byteorder.h>

#define BT_DBG_ENABLED	IS_ENABLED(CONFIG_BT_DEBUG_CRYPTO_TOOLBOX)
#define LOG_MODULE_NAME bt_crypto_toolbox_f5
#include "common/log.h"

int bt_crypto_toolbox_f5(const uint8_t *w, const uint8_t *n1, const uint8_t *n2,
			 const bt_addr_le_t *a1, const bt_addr_le_t *a2, uint8_t *mackey,
			 uint8_t *ltk)
{
	static const uint8_t salt[16] = {0x6c, 0x88, 0x83, 0x91, 0xaa, 0xf5, 0xa5, 0x38,
					 0x60, 0x37, 0x0b, 0xdb, 0x5a, 0x60, 0x83, 0xbe};
	uint8_t m[53] = {0x00,						 /* counter */
			 0x62, 0x74, 0x6c, 0x65,			 /* keyID */
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*n1*/
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*2*/
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* a1 */
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* a2 */
			 0x01, 0x00 /* length */};
	uint8_t t[16], ws[32];
	int err;

	BT_DBG("w %s", bt_hex(w, 32));
	BT_DBG("n1 %s", bt_hex(n1, 16));
	BT_DBG("n2 %s", bt_hex(n2, 16));

	sys_memcpy_swap(ws, w, 32);

	err = bt_crypto_toolbox_aes_cmac(salt, ws, 32, t);
	if (err) {
		return err;
	}

	BT_DBG("t %s", bt_hex(t, 16));

	sys_memcpy_swap(m + 5, n1, 16);
	sys_memcpy_swap(m + 21, n2, 16);
	m[37] = a1->type;
	sys_memcpy_swap(m + 38, a1->a.val, 6);
	m[44] = a2->type;
	sys_memcpy_swap(m + 45, a2->a.val, 6);

	err = bt_crypto_toolbox_aes_cmac(t, m, sizeof(m), mackey);
	if (err) {
		return err;
	}

	BT_DBG("mackey %1s", bt_hex(mackey, 16));

	sys_mem_swap(mackey, 16);

	/* counter for ltk is 1 */
	m[0] = 0x01;

	err = bt_crypto_toolbox_aes_cmac(t, m, sizeof(m), ltk);
	if (err) {
		return err;
	}

	BT_DBG("ltk %s", bt_hex(ltk, 16));

	sys_mem_swap(ltk, 16);

	return 0;
}
