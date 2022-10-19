#include <stdint.h>

/**
 * @brief Cryptographic Toolbox f4
 *
 * Defined in Core Vol. 3, part H 2.2.6.
 * 
 * @param u 256-bit
 * @param v 256-bit
 * @param x 128-bit key
 * @param z 8-bit
 * @param[out] res 
 * @retval 0 Computation was successful. @p res contains the result.
 * @retval @-EIO
 */
int bt_crypto_toolbox_f4(const uint8_t *u, const uint8_t *v, 
                         const uint8_t *x, uint8_t z, uint8_t res[16]);
