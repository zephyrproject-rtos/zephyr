/*
 * Copyright (c) 2021 Advanced Climate Systems
 *
 * SPDX-License-Identiier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_MGMT_HAWKBIT_CONFIG_DATA_H_
#define ZEPHYR_INCLUDE_MGMT_HAWKBIT_CONFIG_DATA_H_

/**
 * @brief Populate the config data struct.
 *
 * @param data pointer to the structure the device attributes should
 * should be set on.
 * @return 0 on succes, otherwise negative error code
 */
int hawkbit_get_config_data(struct hawkbit_cfg_data *data);

#endif /* ZEPHYR_INCLUDE_MGMT_HAWKBIT_HAWKBIT_CUSTOM_CONFIG_DATA_H_ */
