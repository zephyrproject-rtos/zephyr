/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Interrupt source definitions for Espressif ESP32-P4
 *
 * Maps peripheral interrupt sources to CLIC interrupt matrix indices.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32P4_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32P4_INTMUX_H_

/* Derived from components/soc/esp32p4/include/soc/interrupts.h */
#define LP_RTC_INTR_SOURCE                  0   /**< LP RTC interrupt */
#define LP_WDT_INTR_SOURCE                  1   /**< LP watchdog interrupt */
#define LP_TIMER_REG0_INTR_SOURCE           2   /**< LP timer register 0 interrupt */
#define LP_TIMER_REG1_INTR_SOURCE           3   /**< LP timer register 1 interrupt */
#define MB_HP_INTR_SOURCE                   4   /**< Mailbox HP interrupt */
#define MB_LP_INTR_SOURCE                   5   /**< Mailbox LP interrupt */
#define PMU_0_INTR_SOURCE                   6   /**< PMU interrupt 0 */
#define PMU_1_INTR_SOURCE                   7   /**< PMU interrupt 1 */
#define LP_ANAPERI_INTR_SOURCE              8   /**< LP analog peripheral interrupt */
#define LP_ADC_INTR_SOURCE                  9   /**< LP ADC interrupt */
#define LP_GPIO_INTR_SOURCE                 10  /**< LP GPIO interrupt */
#define LP_I2C_INTR_SOURCE                  11  /**< LP I2C interrupt */
#define LP_I2S_INTR_SOURCE                  12  /**< LP I2S interrupt */
#define LP_SPI_INTR_SOURCE                  13  /**< LP SPI interrupt */
#define LP_TOUCH_INTR_SOURCE                14  /**< LP touch interrupt */
#define LP_TSENS_INTR_SOURCE                15  /**< LP temperature sensor interrupt */
#define LP_UART_INTR_SOURCE                 16  /**< LP UART interrupt */
#define LP_EFUSE_INTR_SOURCE                17  /**< LP eFuse interrupt */
#define LP_SW_INTR_SOURCE                   18  /**< LP software interrupt */
#define LP_SYSREG_INTR_SOURCE               19  /**< LP sysreg interrupt */
#define LP_HUK_INTR_SOURCE                  20  /**< LP HUK interrupt */
#define SYS_ICM_INTR_SOURCE                 21  /**< System ICM interrupt */
#define USB_SERIAL_JTAG_INTR_SOURCE         22  /**< USB Serial JTAG interrupt */
#define SDIO_HOST_INTR_SOURCE               23  /**< SDIO host interrupt */
#define DW_GDMA_INTR_SOURCE                 24  /**< DesignWare GDMA interrupt */
#define SPI2_INTR_SOURCE                    25  /**< SPI2 interrupt */
#define SPI3_INTR_SOURCE                    26  /**< SPI3 interrupt */
#define I2S0_INTR_SOURCE                    27  /**< I2S0 interrupt */
#define I2S1_INTR_SOURCE                    28  /**< I2S1 interrupt */
#define I2S2_INTR_SOURCE                    29  /**< I2S2 interrupt */
#define UHCI0_INTR_SOURCE                   30  /**< UHCI0 interrupt */
#define UART0_INTR_SOURCE                   31  /**< UART0 interrupt */
#define UART1_INTR_SOURCE                   32  /**< UART1 interrupt */
#define UART2_INTR_SOURCE                   33  /**< UART2 interrupt */
#define UART3_INTR_SOURCE                   34  /**< UART3 interrupt */
#define UART4_INTR_SOURCE                   35  /**< UART4 interrupt */
#define LCD_CAM_INTR_SOURCE                 36  /**< LCD/CAM interrupt */
#define ADC_INTR_SOURCE                     37  /**< ADC interrupt */
#define PWM0_INTR_SOURCE                    38  /**< PWM0 interrupt */
#define PWM1_INTR_SOURCE                    39  /**< PWM1 interrupt */
#define TWAI0_INTR_SOURCE                   40  /**< TWAI0 interrupt */
#define TWAI1_INTR_SOURCE                   41  /**< TWAI1 interrupt */
#define TWAI2_INTR_SOURCE                   42  /**< TWAI2 interrupt */
#define RMT_INTR_SOURCE                     43  /**< RMT interrupt */
#define I2C0_INTR_SOURCE                    44  /**< I2C0 interrupt */
#define I2C1_INTR_SOURCE                    45  /**< I2C1 interrupt */
#define TG0_T0_LEVEL_INTR_SOURCE            46  /**< Timer group 0, timer 0 interrupt, level */
#define TG0_T1_LEVEL_INTR_SOURCE            47  /**< Timer group 0, timer 1 interrupt, level */
#define TG0_WDT_LEVEL_INTR_SOURCE           48  /**< Timer group 0, watchdog interrupt, level */
#define TG1_T0_LEVEL_INTR_SOURCE            49  /**< Timer group 1, timer 0 interrupt, level */
#define TG1_T1_LEVEL_INTR_SOURCE            50  /**< Timer group 1, timer 1 interrupt, level */
#define TG1_WDT_LEVEL_INTR_SOURCE           51  /**< Timer group 1, watchdog interrupt, level */
#define LEDC_INTR_SOURCE                    52  /**< LEDC interrupt */
#define SYSTIMER_TARGET0_INTR_SOURCE        53  /**< System timer target 0 interrupt */
#define SYSTIMER_TARGET1_INTR_SOURCE        54  /**< System timer target 1 interrupt */
#define SYSTIMER_TARGET2_INTR_SOURCE        55  /**< System timer target 2 interrupt */
#define AHB_PDMA_IN_CH0_INTR_SOURCE         56  /**< AHB PDMA IN channel 0 interrupt */
#define AHB_PDMA_IN_CH1_INTR_SOURCE         57  /**< AHB PDMA IN channel 1 interrupt */
#define AHB_PDMA_IN_CH2_INTR_SOURCE         58  /**< AHB PDMA IN channel 2 interrupt */
#define AHB_PDMA_OUT_CH0_INTR_SOURCE        59  /**< AHB PDMA OUT channel 0 interrupt */
#define AHB_PDMA_OUT_CH1_INTR_SOURCE        60  /**< AHB PDMA OUT channel 1 interrupt */
#define AHB_PDMA_OUT_CH2_INTR_SOURCE        61  /**< AHB PDMA OUT channel 2 interrupt */
#define AXI_PDMA_IN_CH0_INTR_SOURCE         62  /**< AXI PDMA IN channel 0 interrupt */
#define AXI_PDMA_IN_CH1_INTR_SOURCE         63  /**< AXI PDMA IN channel 1 interrupt */
#define AXI_PDMA_IN_CH2_INTR_SOURCE         64  /**< AXI PDMA IN channel 2 interrupt */
#define AXI_PDMA_OUT_CH0_INTR_SOURCE        65  /**< AXI PDMA OUT channel 0 interrupt */
#define AXI_PDMA_OUT_CH1_INTR_SOURCE        66  /**< AXI PDMA OUT channel 1 interrupt */
#define AXI_PDMA_OUT_CH2_INTR_SOURCE        67  /**< AXI PDMA OUT channel 2 interrupt */
#define RSA_INTR_SOURCE                     68  /**< RSA accelerator interrupt */
#define AES_INTR_SOURCE                     69  /**< AES accelerator interrupt */
#define SHA_INTR_SOURCE                     70  /**< SHA accelerator interrupt */
#define ECC_INTR_SOURCE                     71  /**< ECC accelerator interrupt */
#define ECDSA_INTR_SOURCE                   72  /**< ECDSA interrupt */
#define KM_INTR_SOURCE                      73  /**< Key manager interrupt */
#define GPIO_INTR0_SOURCE                   74  /**< GPIO interrupt 0 */
#define GPIO_INTR1_SOURCE                   75  /**< GPIO interrupt 1 */
#define GPIO_INTR2_SOURCE                   76  /**< GPIO interrupt 2 */
#define GPIO_INTR3_SOURCE                   77  /**< GPIO interrupt 3 */
#define GPIO_PAD_COMP_INTR_SOURCE           78  /**< GPIO pad comparator interrupt */
#define FROM_CPU_INTR0_SOURCE               79  /**< CPU interrupt 0, level */
#define FROM_CPU_INTR1_SOURCE               80  /**< CPU interrupt 1, level */
#define FROM_CPU_INTR2_SOURCE               81  /**< CPU interrupt 2, level */
#define FROM_CPU_INTR3_SOURCE               82  /**< CPU interrupt 3, level */
#define CACHE_INTR_SOURCE                   83  /**< Cache interrupt */
#define MSPI_INTR_SOURCE                    84  /**< MSPI interrupt */
#define CSI_BRIDGE_INTR_SOURCE              85  /**< CSI bridge interrupt */
#define DSI_BRIDGE_INTR_SOURCE              86  /**< DSI bridge interrupt */
#define CSI_INTR_SOURCE                     87  /**< MIPI CSI interrupt */
#define DSI_INTR_SOURCE                     88  /**< MIPI DSI interrupt */
#define GMII_PHY_INTR_SOURCE                89  /**< GMII PHY interrupt */
#define LPI_INTR_SOURCE                     90  /**< Low Power Idle interrupt */
#define PMT_INTR_SOURCE                     91  /**< Power management timer interrupt */
#define ETH_MAC_INTR_SOURCE                 92  /**< Ethernet MAC interrupt */
#define USB_OTG_INTR_SOURCE                 93  /**< USB OTG interrupt */
#define USB_OTG_ENDP_MULTI_PROC_INTR_SOURCE 94  /**< USB OTG endpoint multi-proc interrupt */
#define JPEG_INTR_SOURCE                    95  /**< JPEG codec interrupt */
#define PPA_INTR_SOURCE                     96  /**< PPA interrupt */
#define CORE0_TRACE_INTR_SOURCE             97  /**< Core 0 trace interrupt */
#define CORE1_TRACE_INTR_SOURCE             98  /**< Core 1 trace interrupt */
#define HP_CORE_CTRL_INTR_SOURCE            99  /**< HP core control interrupt */
#define ISP_INTR_SOURCE                     100 /**< ISP interrupt */
#define I3C_MST_INTR_SOURCE                 101 /**< I3C master interrupt */
#define I3C_SLV_INTR_SOURCE                 102 /**< I3C slave interrupt */
#define USB_OTG11_CH0_INTR_SOURCE           103 /**< USB OTG 1.1 channel 0 interrupt */
#define DMA2D_IN_CH0_INTR_SOURCE            104 /**< 2D DMA IN channel 0 interrupt */
#define DMA2D_IN_CH1_INTR_SOURCE            105 /**< 2D DMA IN channel 1 interrupt */
#define DMA2D_OUT_CH0_INTR_SOURCE           106 /**< 2D DMA OUT channel 0 interrupt */
#define DMA2D_OUT_CH1_INTR_SOURCE           107 /**< 2D DMA OUT channel 1 interrupt */
#define DMA2D_OUT_CH2_INTR_SOURCE           108 /**< 2D DMA OUT channel 2 interrupt */
#define PSRAM_MSPI_INTR_SOURCE              109 /**< PSRAM MSPI interrupt */
#define HP_SYSREG_INTR_SOURCE               110 /**< HP sysreg interrupt */
#define PCNT_INTR_SOURCE                    111 /**< Pulse counter interrupt */
#define HP_PAU_INTR_SOURCE                  112 /**< HP PAU interrupt */
#define HP_PARLIO_RX_INTR_SOURCE            113 /**< HP Parallel IO RX interrupt */
#define HP_PARLIO_TX_INTR_SOURCE            114 /**< HP Parallel IO TX interrupt */
#define ASSIST_DEBUG_INTR_SOURCE            120 /**< Assist debug module interrupt */

/**
 * @brief Default interrupt priority.
 *
 * Zero will allocate low/medium levels of priority (ESP_INTR_FLAG_LOWMED).
 */
#define IRQ_DEFAULT_PRIORITY 0

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32P4_INTMUX_H_ */
