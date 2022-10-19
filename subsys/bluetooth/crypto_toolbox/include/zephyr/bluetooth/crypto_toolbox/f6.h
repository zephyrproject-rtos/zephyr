#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>

/**
 * @brief Cryptographic Toolbox f6
 *
 * Defined in Core Vol. 3, part H 2.2.8.
 *
 * @param w 128-bit
 * @param n1 128-bit
 * @param n2 128-bit
 * @param r 128-bit
 * @param iocap 24-bit
 * @param a1 56-bit
 * @param a2 56-bit
 * @param[out] check
 * @retval 0 Computation was successful. @p res contains the result.
 * @retval -EIO
 */
int bt_crypto_toolbox_f6(const uint8_t *w, const uint8_t *n1, const uint8_t *n2, const uint8_t *r,
			 const uint8_t *iocap, const bt_addr_le_t *a1, const bt_addr_le_t *a2,
			 uint8_t *check);
