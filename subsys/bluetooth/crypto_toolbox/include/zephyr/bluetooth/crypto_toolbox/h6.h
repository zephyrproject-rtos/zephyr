#include <stdint.h>

/**
 * @brief Cryptographic Toolbox h6
 *
 * Link key conversion defined in Core Vol. 3, part H 2.2.10.
 *
 * @param w 128-bit key
 * @param key_id 32-bit
 * @param[out] res 128-bit
 * @retval 0 Computation was successful. @p res contains the result.
 * @retval @-EIO
 */
int bt_crypto_toolbox_h6(const uint8_t w[16], const uint8_t key_id[4], 
                         uint8_t res[16]);
