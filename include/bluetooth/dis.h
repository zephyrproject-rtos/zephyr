/** @file
 *  @brief Bluetooth DIS Service sample
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_DIS_H
#define __BT_DIS_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Register DIS Service
 *
 *  This registers DIS attributes into GATT database.
 *
 *  @param model Device model
 *  @param manuf Device manufactorer
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_dis_register(const char *model, const char *manuf);

#ifdef __cplusplus
}
#endif

#endif /* __BT_DIS_H */
