#include <stddef.h>
#include <stdint.h>

/* Cypher based Message Authentication Code (CMAC) with AES 128 bit
 *
 * Input    : key    ( 128-bit key )
 *          : in     ( message to be authenticated )
 *          : len    ( length of the message in octets )
 * Output   : out    ( message authentication code )
 */
int bt_crypto_toolbox_aes_cmac(const uint8_t *key, const uint8_t *in, 
							   size_t len, uint8_t *out);