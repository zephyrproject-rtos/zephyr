/** @file
 *  @brief Bluetooth HoG Service
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BT_HOG_H
#define __BT_HOG_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Register HoG Service
 *
 *  This registers HoG attributes into GATT database.
 *
 *  @return 0 in case of success or negative value in case of error.
 */
int bt_hog_register(void);

#ifdef __cplusplus
}
#endif

#endif /* __BT_HOG_H */
