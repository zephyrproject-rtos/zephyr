/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
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

#ifndef _HCI_CONTROLLER_H_
#define _HCI_CONTROLLER_H_

#include <net/buf.h>

int hci_handle(struct net_buf *buf, uint8_t *len, uint8_t **out);
void hcic_encode(uint8_t *buf, uint8_t *len, uint8_t **out);
void hcic_encode_num_cmplt(uint16_t instance, uint8_t num, uint8_t *len,
			   uint8_t **out);

#endif /* _HCI_CONTROLLER_H_ */
