/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 *
 * Remote control protocol identifiers. Protocols known to Linux use the same
 * values as enum rc_proto in
 * https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/lirc.h
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RC_RC_PROTO_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RC_RC_PROTO_H_

/**
 * @defgroup rc_proto Remote Control Protocol Identifiers
 * @ingroup rc_interface
 * @{
 */

#define RC_PROTO_UNKNOWN	0  /**< Protocol not known */
#define RC_PROTO_OTHER		1  /**< Protocol known but proprietary */
#define RC_PROTO_RC5		2  /**< Philips RC5 */
#define RC_PROTO_RC5X_20	3  /**< Philips RC5x 20 bit */
#define RC_PROTO_RC5_SZ		4  /**< StreamZap variant of RC5 */
#define RC_PROTO_JVC		5  /**< JVC */
#define RC_PROTO_SONY12		6  /**< Sony 12 bit */
#define RC_PROTO_SONY15		7  /**< Sony 15 bit */
#define RC_PROTO_SONY20		8  /**< Sony 20 bit */
#define RC_PROTO_NEC		9  /**< NEC */
#define RC_PROTO_NECX		10 /**< Extended NEC */
#define RC_PROTO_NEC32		11 /**< NEC 32 bit */
#define RC_PROTO_SANYO		12 /**< Sanyo */
#define RC_PROTO_MCIR2_KBD	13 /**< RC6-like MCE keyboard */
#define RC_PROTO_MCIR2_MSE	14 /**< RC6-like MCE mouse */
#define RC_PROTO_RC6_0		15 /**< Philips RC6-0-16 */
#define RC_PROTO_RC6_6A_20	16 /**< Philips RC6-6A-20 */
#define RC_PROTO_RC6_6A_24	17 /**< Philips RC6-6A-24 */
#define RC_PROTO_RC6_6A_32	18 /**< Philips RC6-6A-32 */
#define RC_PROTO_RC6_MCE	19 /**< MCE, Philips RC6-6A-32 subtype */
#define RC_PROTO_SHARP		20 /**< Sharp */
#define RC_PROTO_XMP		21 /**< XMP */
#define RC_PROTO_CEC		22 /**< CEC */
#define RC_PROTO_IMON		23 /**< iMon Pad */
#define RC_PROTO_RCMM12		24 /**< RC-MM 12 bit */
#define RC_PROTO_RCMM24		25 /**< RC-MM 24 bit */
#define RC_PROTO_RCMM32		26 /**< RC-MM 32 bit */
#define RC_PROTO_XBOX_DVD	27 /**< Xbox DVD Movie Playback Kit */

/** First identifier of protocols not known to Linux */
#define RC_PROTO_ZEPHYR_BASE	0x100

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RC_RC_PROTO_H_ */
