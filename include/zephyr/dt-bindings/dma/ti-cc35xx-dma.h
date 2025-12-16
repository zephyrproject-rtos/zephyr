/*
 *  Copyright (c) 2025-2026 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DMA_TI_CC35XX_DMA_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DMA_TI_CC35XX_DMA_H_

/** DMA  Peripheral index on bit 0-3*/
#define CC35xx_DMA_PERIPH_INDEX(val)     ((val & 0xf))
#define CC35xx_DMA_GET_PERIPH_INDEX(cfg) ((cfg) & 0xf)

/** DMA  Available peripheral indexes: */
#define CC35xx_DMA_PERIPH_NONE      CC35xx_DMA_PERIPH_INDEX(0)  /*!< NONE */
#define CC35xx_DMA_PERIPH_UARTLIN_0 CC35xx_DMA_PERIPH_INDEX(1)  /*!< UARTLIN0 */
#define CC35xx_DMA_PERIPH_UARTLIN_1 CC35xx_DMA_PERIPH_INDEX(2)  /*!< UARTLIN1 */
#define CC35xx_DMA_PERIPH_SPI_0     CC35xx_DMA_PERIPH_INDEX(3)  /*!< SPI0 */
#define CC35xx_DMA_PERIPH_SPI_1     CC35xx_DMA_PERIPH_INDEX(4)  /*!< SPI1 */
#define CC35xx_DMA_PERIPH_I2C_0     CC35xx_DMA_PERIPH_INDEX(5)  /*!< I2C0 */
#define CC35xx_DMA_PERIPH_I2C_1     CC35xx_DMA_PERIPH_INDEX(6)  /*!< I2C1 */
#define CC35xx_DMA_PERIPH_SDMMC     CC35xx_DMA_PERIPH_INDEX(7)  /*!< SDMMC */
#define CC35xx_DMA_PERIPH_SDIO      CC35xx_DMA_PERIPH_INDEX(8)  /*!< SDIO */
#define CC35xx_DMA_PERIPH_MCAN      CC35xx_DMA_PERIPH_INDEX(9)  /*!< MCAN */
#define CC35xx_DMA_PERIPH_ADC       CC35xx_DMA_PERIPH_INDEX(10) /*!< ADC */
#define CC35xx_DMA_PERIPH_PDM       CC35xx_DMA_PERIPH_INDEX(11) /*!< PDM */
#define CC35xx_DMA_PERIPH_HIF       CC35xx_DMA_PERIPH_INDEX(12) /*!< HIF */
#define CC35xx_DMA_PERIPH_UARTLIN_2 CC35xx_DMA_PERIPH_INDEX(13) /*!< UARTLIN2 */

#define CC35XX_DMA_SET_CONFIG(peripheral) (peripheral)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DMA_TI_CC35XX_DMA_H_ */
