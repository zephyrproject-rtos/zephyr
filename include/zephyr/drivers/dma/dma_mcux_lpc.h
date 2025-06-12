/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_MCUX_LPC_H_
#define ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_MCUX_LPC_H_

/*
 * LPC DMA engine channel hardware trigger attributes.
 * These attributes can be set to the "dma_slot" field
 * in a dma_config structure to configure a channel for
 * hardware triggering.
 */

/* Peripheral request enable. When set, the peripheral
 * request line associated with this channel is used to pace DMA transfers.
 */
#define LPC_DMA_PERIPH_REQ_EN BIT(0)

/* Hardware trigger enable. When set, the hardware trigger connected to this
 * channel via INPUTMUX can be used to trigger a transfer
 */
#define LPC_DMA_HWTRIG_EN BIT(1)

/* HW trigger polarity. When this bit is set, the trigger will be active
 * high or rising edge triggered, based on TRIG_TYPE selection
 */
#define LPC_DMA_TRIGPOL_HIGH_RISING BIT(2)

/* HW trigger type. When this bit is set, the trigger will be level triggered.
 * When it is cleared, the hardware trigger will be edge triggered.
 */
#define LPC_DMA_TRIGTYPE_LEVEL BIT(3)

/* HW trigger burst mode. When set, the hardware trigger will cause a burst
 * transfer to occur, the length of which is determined by BURST_POWER.
 * When cleared, a single transfer (of the width selected by XFERCFG register)
 * will occur.
 */
#define LPC_DMA_TRIGBURST BIT(4)

/* HW trigger burst power. Note that due to the size limit of the dma_slot
 * field, the maximum transfer burst possible is 128. The hardware supports
 * up to 1024 transfers in BURSTPOWER. The value set here will result in
 * 2^BURSTPOWER transfers occurring. So for BURSTPOWER=3, 8 transfers would
 * occur.
 */
#define LPC_DMA_BURSTPOWER(pwr) (((pwr) & 0x7) << 5)


/* Used by driver to extract burstpower setting */
#define LPC_DMA_GET_BURSTPOWER(slot) (((slot) & 0xE0) >> 5)

/*
 * Special channel_direction for SPI_MCUX_FLEXCOMM_TX so
 * that DMA driver can configure the last descriptor
 * to deassert CS.
 */
#define LPC_DMA_SPI_MCUX_FLEXCOMM_TX (DMA_CHANNEL_DIRECTION_PRIV_START)

#endif /* ZEPHYR_INCLUDE_DRIVERS_DMA_DMA_MCUX_LPC_H_ */
