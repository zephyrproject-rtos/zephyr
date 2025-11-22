#pragma once

int i2c_hid_device_set_vid(const struct device *dev, const uint16_t vid);

int i2c_hid_device_set_pid(const struct device *dev, const uint16_t pid);
