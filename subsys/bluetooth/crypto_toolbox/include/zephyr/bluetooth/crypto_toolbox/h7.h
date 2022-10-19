#include <stdint.h>

/**
 * @brief Cryptographic Toolbox h7
 *
 * Link key conversion defined in Core Vol. 3, part H 2.2.11.
 *
 * @param salt 128-bit key
 * @param w 128-bit input of the AES-CMAC function
 * @param[out] res 128-bit
 * @retval 0 Computation was successful. @p res contains the result.
 * @retval @-EIO
 */
int bt_crypto_toolbox_h7(const uint8_t salt[16], const uint8_t w[16], 
                         uint8_t res[16]);