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

#ifndef _DNS_UTILS_H_
#define _DNS_UTILS_H_

#include "app_buf.h"
#include <stdint.h>

int dns_print_msg_header(uint8_t *header, int size);
int dns_print_msg_query(uint8_t *qname, int qname_size, int qtype, int qclass);
int dns_print_label(uint8_t *label, int size);
int dns_print_readable_msg_label(int offset, uint8_t *buf, int size);
int print_buf(uint8_t *buf, size_t size);
int print_app_buf(struct app_buf_t *buf);

#endif
