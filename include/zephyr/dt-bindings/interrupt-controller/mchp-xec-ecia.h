/*
 * Copyright (c) 2021 Microchip Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __DT_BINDING_MCHP_XEC_ECIA_H
#define __DT_BINDING_MCHP_XEC_ECIA_H

/*
 * Encode peripheral interrupt information into a 32-bit unsigned.
 * g  = bits[0:4], GIRQ number in [8, 26]
 * gb = bits[12:8], peripheral source bit position [0, 31] in the GIRQ
 * na = bits[23:16], aggregated GIRQ NVIC number
 * nd = bits[31:24], direct NVIC number. For sources without a direct
 *      connection nd = na.
 * NOTE: GIRQ22 is a peripheral clock wake only. GIRQ22 and its sources
 * are not connected to the NVIC. Use 255 for na and nd.
 */
#define MCHP_XEC_ECIA(g, gb, na, nd)					\
	(((g) & 0x1f) + (((gb) & 0x1f) << 8) + (((na) & 0xff) << 16) +	\
	(((nd) & 0xff) << 24))

/* extract specific information from encoded MCHP_XEC_ECIA */
#define MCHP_XEC_ECIA_GIRQ(e)		((e) & 0x1f)
#define MCHP_XEC_ECIA_GIRQ_POS(e)	(((e) >> 8) & 0x1f)
#define MCHP_XEC_ECIA_NVIC_AGGR(e)	(((e) >> 16) & 0xff)
#define MCHP_XEC_ECIA_NVIC_DIRECT(e)	(((e) >> 24) & 0xff)

#endif /* __DT_BINDING_MCHP_XEC_ECIA_H */
