#include <stdint.h>
#include <zephyr/bluetooth/bluetooth.h>

int bt_crypto_toolbox_f6(const uint8_t *w, const uint8_t *n1, const uint8_t *n2,
		                 const uint8_t *r, const uint8_t *iocap, 
                         const bt_addr_le_t *a1, const bt_addr_le_t *a2, 
                         uint8_t *check);