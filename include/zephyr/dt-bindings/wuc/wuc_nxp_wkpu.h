/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NXP WKPU (Wake-up Unit) wakeup-source encoding helpers.
 *
 * Encodes a WKPU wakeup source as a single 32-bit value, plus helper macros to
 * build and decode such values for use with the NXP WKPU WUC driver and
 * devicetree "wakeup-ctrls" properties.
 *
 * The WKPU exposes a flat set of up to 64 wakeup sources. Sources 0-3 are
 * internal peripheral events and sources 4..63 are the external pins
 * WKPU[0]..WKPU[59]; all of them are configured through the same registers, so
 * a single encoding (index + edge + event) covers every source.
 *
 * Wakeup source encoding:
 *   Bits [23:16] - Index (WKPU source number, 0..63)
 *   Bits [15:8]  - Edge type (1=RISING, 2=FALLING, 3=ANY)
 *   Bits [7:0]   - Event type (0=INT, 1=INT_WAKEUP)
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_WUC_NXP_WKPU_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_WUC_NXP_WKPU_H_

/** Bit position of the index field (WKPU source number). */
#define NXP_WKPU_SOURCE_INDEX_POS 16
/** Bit position of the edge-type field. */
#define NXP_WKPU_SOURCE_EDGE_POS  8
/** Bit position of the event-type field. */
#define NXP_WKPU_SOURCE_EVENT_POS 0

/** Mask covering one 8-bit field of the encoded wakeup source. */
#define NXP_WKPU_SOURCE_MASK 0xFF

/** Edge-type value: trigger on rising edge. */
#define NXP_WKPU_EDGE_RISING  1
/** Edge-type value: trigger on falling edge. */
#define NXP_WKPU_EDGE_FALLING 2
/** Edge-type value: trigger on any edge. */
#define NXP_WKPU_EDGE_ANY     3

/** Event-type value: generate a core interrupt only. */
#define NXP_WKPU_EVENT_INT        0
/** Event-type value: generate a core interrupt and a system wakeup. */
#define NXP_WKPU_EVENT_INT_WAKEUP 1

/**
 * @brief Encode a WKPU wakeup source into a single 32-bit value.
 *
 * @param index WKPU source number (0..63).
 * @param edge  Edge type (NXP_WKPU_EDGE_*).
 * @param event Event type (NXP_WKPU_EVENT_*).
 */
#define NXP_WKPU_WAKEUP_SOURCE_ENCODE(index, edge, event)                                          \
	(((index) << NXP_WKPU_SOURCE_INDEX_POS) | ((edge) << NXP_WKPU_SOURCE_EDGE_POS) |           \
	 ((event) << NXP_WKPU_SOURCE_EVENT_POS))

/**
 * @brief Extract one 8-bit field from an encoded wakeup source.
 *
 * @param source Encoded wakeup source value.
 * @param pos    Bit position of the field (one of NXP_WKPU_SOURCE_*_POS).
 */
#define NXP_WKPU_WAKEUP_SOURCE_DECODE(source, pos) (((source) >> (pos)) & NXP_WKPU_SOURCE_MASK)

/** @brief Decode the index field (WKPU source number). */
#define NXP_WKPU_WAKEUP_SOURCE_DECODE_INDEX(source)                                                \
	NXP_WKPU_WAKEUP_SOURCE_DECODE(source, NXP_WKPU_SOURCE_INDEX_POS)

/** @brief Decode the edge-type field. */
#define NXP_WKPU_WAKEUP_SOURCE_DECODE_EDGE(source)                                                 \
	NXP_WKPU_WAKEUP_SOURCE_DECODE(source, NXP_WKPU_SOURCE_EDGE_POS)

/** @brief Decode the event-type field. */
#define NXP_WKPU_WAKEUP_SOURCE_DECODE_EVENT(source)                                                \
	NXP_WKPU_WAKEUP_SOURCE_DECODE(source, NXP_WKPU_SOURCE_EVENT_POS)

/** @brief Encode an external WKPU pin (WKPU[n] == source index 4 + n). */
#define NXP_WKPU_PIN(n, edge, event) NXP_WKPU_WAKEUP_SOURCE_ENCODE((4 + (n)), edge, event)

/*
 * Internal WKPU wakeup sources (indices 0-3). See the chip reference manual,
 * "WKPU wake-up source connectivity".
 */

/** Source 0: SWT_0 timeout / RTC-API API wakeup. */
#define NXP_WKPU_SWT0_RTC_API(edge, event) NXP_WKPU_WAKEUP_SOURCE_ENCODE(0, edge, event)
/** Source 1: RTC timeout. */
#define NXP_WKPU_RTC_TIMEOUT(edge, event)  NXP_WKPU_WAKEUP_SOURCE_ENCODE(1, edge, event)
/** Source 2: LPCMP_0/1/2 round-robin. */
#define NXP_WKPU_LPCMP_RR(edge, event)     NXP_WKPU_WAKEUP_SOURCE_ENCODE(2, edge, event)
/** Source 3: RTI (PIT0-RTI). */
#define NXP_WKPU_RTI(edge, event)          NXP_WKPU_WAKEUP_SOURCE_ENCODE(3, edge, event)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_WUC_NXP_WKPU_H_ */
