/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __BT_HCI_RAW_INTERNAL_H
#define __BT_HCI_RAW_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

struct bt_dev_raw {
	/* Registered HCI driver */
	struct bt_hci_driver	*drv;
};

extern struct bt_dev_raw bt_dev;

#ifdef __cplusplus
}
#endif

#endif /* __BT_HCI_RAW_INTERNAL_H */
