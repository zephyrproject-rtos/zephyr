#ifndef DEVICE_H
#define DEVICE_H

#include <zephyr/ztest.h>

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/fff.h>

DECLARE_FAKE_VALUE_FUNC(bool, device_is_ready, const struct device *);

#endif /* DEVICE_H */
