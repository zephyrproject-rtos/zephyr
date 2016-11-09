/** @file
 * @brief Audio/Video Distribution Transport Protocol header.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
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
#ifndef __BT_AVDTP_H
#define __BT_AVDTP_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief AVDTP SEID Information */
struct bt_avdtp_seid_info {
	/** Stream End Point ID */
	uint8_t id:6;
	/** End Point usage status */
	uint8_t inuse:1;
	/** Reserved */
	uint8_t rfa0:1;
	/** Media-type of the End Point */
	uint8_t media_type:4;
	/** TSEP of the End Point */
	uint8_t tsep:1;
	/** Reserved */
	uint8_t rfa1:3;
} __packed;

/** @brief AVDTP Local SEP*/
struct bt_avdtp_seid_lsep {
	/** Stream End Point information */
	struct bt_avdtp_seid_info sep;
	/** Pointer to next local Stream End Point structure */
	struct bt_avdtp_seid_lsep *next;
};

#ifdef __cplusplus
}
#endif

#endif /* __BT_AVDTP_H */
