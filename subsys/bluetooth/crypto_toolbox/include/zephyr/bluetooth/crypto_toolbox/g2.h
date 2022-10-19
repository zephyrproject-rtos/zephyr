#include <stdint.h>

/**
 * @brief Cryptographic Toolbox g2

 * Defined in Core Vol. 3, part H 2.2.9.
 *
 * @param u 256-bit
 * @param v 256-bit
 * @param x 128-bit
 * @param y 128-bit
 * @param[out] passkey
 * @retval 0 Computation was successful. @p res contains the result.
 * @retval -EIO
 */
int bt_crypto_toolbox_g2(const uint8_t u[32], const uint8_t v[32], const uint8_t x[16],
			 const uint8_t y[16], uint32_t *passkey);
