#include <stddef.h>
#include <stdint.h>

/**
 * @brief Cypher based Message Authentication Code (CMAC) with AES 128 bit
 * 
 * Defined in Core Vol. 3, part H 2.2.5.
 *
 * @param key 128-bit key
 * @param in message to be authenticated
 * @param len length of the message in octets
 * @param[out] out message authentication code
 * @retval 0 Computation was successful. @p res contains the result.
 * @retval -EIO
 */
int bt_crypto_toolbox_aes_cmac(const uint8_t *key, const uint8_t *in, 
							   size_t len, uint8_t *out);
