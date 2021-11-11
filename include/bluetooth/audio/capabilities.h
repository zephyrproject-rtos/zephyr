/* @file
 * @brief Internal APIs for Audio Capabilities handling
 *
 * Copyright (c) 2021 Intel Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAPABILITIES_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAPABILITIES_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Get list of capabilities by type */
sys_slist_t *bt_audio_capability_get(uint8_t type);

#define BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED     0x00
#define BT_AUDIO_CAPABILITY_UNFRAMED_NOT_SUPPORTED 0x01

/** @def BT_AUDIO_CAPABILITY_PREF
 *  @brief Helper to declare elements of @ref bt_audio_capability_pref
 *
 *  @param _framing Framing Support
 *  @param _phy Preferred Target PHY
 *  @param _rtn Preferred Retransmission number
 *  @param _latency Preferred Maximum Transport Latency (msec)
 *  @param _pd_min Minimum Presentation Delay (usec)
 *  @param _pd_max Maximum Presentation Delay (usec)
 *  @param _pref_pd_min Preferred Minimum Presentation Delay (usec)
 *  @param _pref_pd_max Preferred Maximum Presentation Delay (usec)
 */
#define BT_AUDIO_CAPABILITY_PREF(_framing, _phy, _rtn, _latency, _pd_min, \
				 _pd_max, _pref_pd_min, _pref_pd_max) \
	{ \
		.framing = _framing, \
		.phy = _phy, \
		.rtn = _rtn, \
		.latency = _latency, \
		.pd_min = _pd_min, \
		.pd_max = _pd_max, \
		.pref_pd_min = _pref_pd_min, \
		.pref_pd_max = _pref_pd_max, \
	}

/** @brief Audio Capability Preference structure. */
struct bt_audio_capability_pref {
	/** @brief Supported framing
	 *
	 *  Unlike the other fields, this is not a preference but whether
	 *  the capability supports unframed ISOAL PDUs.
	 *
	 *  Possible values: BT_AUDIO_CAPABILITY_UNFRAMED_SUPPORTED and
	 *  BT_AUDIO_CAPABILITY_UNFRAMED_NOT_SUPPORTED.
	 */
	uint8_t  framing;

	/** Preferred PHY */
	uint8_t  phy;

	/** Preferred Retransmission Number */
	uint8_t  rtn;

	/** Preferred Transport Latency */
	uint16_t latency;

	/** @brief Minimun Presentation Delay
	 *
	 *  Unlike the other fields, this is not a preference but a minimum
	 *  requirement.
	 */
	uint32_t pd_min;

	/** @brief Maximum Presentation Delay
	 *
	 *  Unlike the other fields, this is not a preference but a maximum
	 *  requirement.
	 */
	uint32_t pd_max;

	/** @brief Preferred minimun Presentation Delay */
	uint32_t pref_pd_min;

	/** @brief Preferred maximum Presentation Delay	*/
	uint32_t pref_pd_max;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_CAPABILITIES_H_ */
