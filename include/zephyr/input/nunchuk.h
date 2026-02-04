#ifndef ZEPHYR_INCLUDE_INPUT_NUNCHUK_H_
#define ZEPHYR_INCLUDE_INPUT_NUNCHUK_H_

#include <zephyr/device.h>
#include <stdint.h>

enum nunchuk_buttons
{
    NUNCHUK_BUTTON_NONE = 0,
    NUNCHUK_BUTTON_Z = BIT(0),
    NUNCHUK_BUTTON_C = BIT(1),
    NUNCHUK_BUTTON_ALL = NUNCHUK_BUTTON_Z | NUNCHUK_BUTTON_C
};

struct nunchuk_reading
{
#ifdef CONFIG_INPUT_NUNCHUK_ACCEL
    uint16_t accel[3];
#endif // CONFIG_INPUT_NUNCHUK_ACCEL
    uint8_t buttons;
    uint8_t joystick[2];
};

struct nunchuk_driver_api {
    int (*fetch)(const struct device *);
    int (*read)(const struct device *, struct nunchuk_reading*);
};

static inline int nunchuk_fetch(const struct device *dev)
{
    const struct nunchuk_driver_api *api = (const struct nunchuk_driver_api *)dev->api;

    if (api == NULL || api->fetch == NULL) {
        return -ENOTSUP;
    }

    return api->fetch(dev);
}

static inline int nunchuk_read(const struct device *dev, struct nunchuk_reading* reading)
{
    const struct nunchuk_driver_api *api = (const struct nunchuk_driver_api *)dev->api;

    if (api == NULL || api->read == NULL) {
        return -ENOTSUP;
    }

    return api->read(dev, reading);
}

#endif /* ZEPHYR_INCLUDE_INPUT_NUNCHUK_H_ */
