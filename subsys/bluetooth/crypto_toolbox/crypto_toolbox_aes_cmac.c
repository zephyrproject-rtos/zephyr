#include <errno.h>
#include <tinycrypt/constants.h>
#include <tinycrypt/cmac_mode.h>

#include <zephyr/bluetooth/crypto_toolbox/aes_cmac.h>

int bt_crypto_toolbox_aes_cmac(const uint8_t *key, const uint8_t *in, 
							   size_t len, uint8_t *out)
{
	struct tc_aes_key_sched_struct sched;
	struct tc_cmac_struct state;

	if (tc_cmac_setup(&state, key, &sched) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	if (tc_cmac_update(&state, in, len) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	if (tc_cmac_final(out, &state) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	return 0;
}
