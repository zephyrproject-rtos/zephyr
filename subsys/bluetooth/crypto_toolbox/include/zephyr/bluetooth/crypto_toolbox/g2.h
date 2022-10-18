#include <stdint.h>

int bt_crypto_toolbox_g2(const uint8_t u[32], const uint8_t v[32],
		                 const uint8_t x[16], const uint8_t y[16], 
                         uint32_t *passkey);
