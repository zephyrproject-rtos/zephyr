#include <stdint.h>
#include <zephyr/bluetooth/bluetooth.h>

/**
 * @brief Cryptographic Toolbox f5
 *
 * Defined in Core Vol. 3, part H 2.2.7.
 * 
 * @param w 256-bit
 * @param n1 128-bit
 * @param n2 128-bit
 * @param a1 56-bit
 * @param a2 56-bit
 * @param[out] mackey most significant 128-bit of the result
 * @param[out] ltk least significant 128-bit of the result
 * @retval 0 Computation was successful. @p res contains the result.
 * @retval -EIO
 */
int bt_crypto_toolbox_f5(const uint8_t *w, const uint8_t *n1, const uint8_t *n2,
		                 const bt_addr_le_t *a1, const bt_addr_le_t *a2, 
                         uint8_t *mackey, uint8_t *ltk);
