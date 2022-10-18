#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/crypto_toolbox/f4.h>

#include <zephyr/bluetooth/crypto_toolbox/aes_cmac.h>

int bt_crypto_toolbox_f4(const uint8_t *u, const uint8_t *v, 
                                const uint8_t *x, uint8_t z, uint8_t res[16])
{
	uint8_t xs[16];
	uint8_t m[65];
	int err;

	/*
	 * U, V and Z are concatenated and used as input m to the function
	 * AES-CMAC and X is used as the key k.
	 *
	 * Core Spec 4.2 Vol 3 Part H 2.2.5
	 *
	 * note:
	 * bt_smp_aes_cmac uses BE data and smp_f4 accept LE so we swap
	 */
	sys_memcpy_swap(m, u, 32);
	sys_memcpy_swap(m + 32, v, 32);
	m[64] = z;

	sys_memcpy_swap(xs, x, 16);

	err = bt_crypto_toolbox_aes_cmac(xs, m, sizeof(m), res);
	if (err) {
		return err;
	}

	sys_mem_swap(res, 16);

	return err;
}
