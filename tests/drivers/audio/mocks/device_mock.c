#include <zephyr/ztest.h>

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/fff.h>

#include "device_mock.h"

DEFINE_FAKE_VALUE_FUNC(bool, device_is_ready, const struct device *);
