#ifndef ZEPHYR_DRIVERS_MISC_TM1637_H_
#define ZEPHYR_DRIVERS_MISC_TM1637_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

int tm1637_display_number(const struct device *dev, int num, bool colon);
int tm1637_display_raw_segments(const struct device *dev, uint8_t pos, uint8_t segments);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_TM1637_H_ */
