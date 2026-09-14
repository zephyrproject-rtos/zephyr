/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Interrupt source definitions for Espressif ESP32-S31
 *
 * Maps peripheral interrupt sources to CLIC interrupt matrix indices.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32S31_INTMUX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32S31_INTMUX_H_

/* Derived from components/soc/esp32s31/include/soc/interrupts.h */

#define SYS_ICM_INTR_SOURCE                     0   /**< SYS ICM interrupt */
#define AXI_PERF_MON_INTR_SOURCE                1   /**< AXI PERF MON interrupt */
#define USB_SERIAL_JTAG_INTR_SOURCE             2   /**< USB SERIAL JTAG interrupt */
#define SDIO_HOST_INTR_SOURCE                   3   /**< SDIO HOST interrupt */
#define SPI2_INTR_SOURCE                        4   /**< SPI2 interrupt */
#define SPI3_INTR_SOURCE                        5   /**< SPI3 interrupt */
#define I2S0_INTR_SOURCE                        6   /**< I2S0 interrupt */
#define I2S1_INTR_SOURCE                        7   /**< I2S1 interrupt */
#define UHCI0_INTR_SOURCE                       8   /**< UHCI0 interrupt */
#define UART0_INTR_SOURCE                       9   /**< UART0 interrupt */
#define UART1_INTR_SOURCE                       10  /**< UART1 interrupt */
#define UART2_INTR_SOURCE                       11  /**< UART2 interrupt */
#define UART3_INTR_SOURCE                       12  /**< UART3 interrupt */
#define LCD_CAM_INTR_SOURCE                     13  /**< LCD CAM interrupt */
#define PWM0_INTR_SOURCE                        14  /**< PWM0 interrupt */
#define PWM1_INTR_SOURCE                        15  /**< PWM1 interrupt */
#define PWM2_INTR_SOURCE                        16  /**< PWM2 interrupt */
#define PWM3_INTR_SOURCE                        17  /**< PWM3 interrupt */
#define TWAI0_INTR_SOURCE                       18  /**< TWAI0 interrupt */
#define TWAI0_TIMER_INTR_SOURCE                 19  /**< TWAI0 TIMER interrupt */
#define TWAI1_INTR_SOURCE                       20  /**< TWAI1 interrupt */
#define TWAI1_TIMER_INTR_SOURCE                 21  /**< TWAI1 TIMER interrupt */
#define RMT_INTR_SOURCE                         22  /**< RMT interrupt */
#define I2C0_INTR_SOURCE                        23  /**< I2C0 interrupt */
#define I2C1_INTR_SOURCE                        24  /**< I2C1 interrupt */
#define TIMERGRP0_T0_INTR_SOURCE                25  /**< TIMERGRP0 T0 interrupt */
#define TIMERGRP0_T1_INTR_SOURCE                26  /**< TIMERGRP0 T1 interrupt */
#define TIMERGRP0_WDT_INTR_SOURCE               27  /**< TIMERGRP0 WDT interrupt */
#define TIMERGRP1_T0_INTR_SOURCE                28  /**< TIMERGRP1 T0 interrupt */
#define TIMERGRP1_T1_INTR_SOURCE                29  /**< TIMERGRP1 T1 interrupt */
#define TIMERGRP1_WDT_INTR_SOURCE               30  /**< TIMERGRP1 WDT interrupt */
#define LEDC0_INTR_SOURCE                       31  /**< LEDC0 interrupt */
#define LEDC1_INTR_SOURCE                       32  /**< LEDC1 interrupt */
#define SYSTIMER_TARGET0_INTR_SOURCE            33  /**< SYSTIMER TARGET0 interrupt */
#define SYSTIMER_TARGET1_INTR_SOURCE            34  /**< SYSTIMER TARGET1 interrupt */
#define SYSTIMER_TARGET2_INTR_SOURCE            35  /**< SYSTIMER TARGET2 interrupt */
#define AHB_PDMA_IN_CH0_INTR_SOURCE             36  /**< AHB PDMA IN CH0 interrupt */
#define AHB_PDMA_IN_CH1_INTR_SOURCE             37  /**< AHB PDMA IN CH1 interrupt */
#define AHB_PDMA_IN_CH2_INTR_SOURCE             38  /**< AHB PDMA IN CH2 interrupt */
#define AHB_PDMA_IN_CH3_INTR_SOURCE             39  /**< AHB PDMA IN CH3 interrupt */
#define AHB_PDMA_IN_CH4_INTR_SOURCE             40  /**< AHB PDMA IN CH4 interrupt */
#define AHB_PDMA_OUT_CH0_INTR_SOURCE            41  /**< AHB PDMA OUT CH0 interrupt */
#define AHB_PDMA_OUT_CH1_INTR_SOURCE            42  /**< AHB PDMA OUT CH1 interrupt */
#define AHB_PDMA_OUT_CH2_INTR_SOURCE            43  /**< AHB PDMA OUT CH2 interrupt */
#define AHB_PDMA_OUT_CH3_INTR_SOURCE            44  /**< AHB PDMA OUT CH3 interrupt */
#define AHB_PDMA_OUT_CH4_INTR_SOURCE            45  /**< AHB PDMA OUT CH4 interrupt */
#define ASRC_CHNL0_INTR_SOURCE                  46  /**< ASRC CHNL0 interrupt */
#define ASRC_CHNL1_INTR_SOURCE                  47  /**< ASRC CHNL1 interrupt */
#define AXI_PDMA_IN_CH0_INTR_SOURCE             48  /**< AXI PDMA IN CH0 interrupt */
#define AXI_PDMA_IN_CH1_INTR_SOURCE             49  /**< AXI PDMA IN CH1 interrupt */
#define AXI_PDMA_IN_CH2_INTR_SOURCE             50  /**< AXI PDMA IN CH2 interrupt */
#define AXI_PDMA_OUT_CH0_INTR_SOURCE            51  /**< AXI PDMA OUT CH0 interrupt */
#define AXI_PDMA_OUT_CH1_INTR_SOURCE            52  /**< AXI PDMA OUT CH1 interrupt */
#define AXI_PDMA_OUT_CH2_INTR_SOURCE            53  /**< AXI PDMA OUT CH2 interrupt */
#define RSA_INTR_SOURCE                         54  /**< RSA interrupt */
#define AES_INTR_SOURCE                         55  /**< AES interrupt */
#define SHA_INTR_SOURCE                         56  /**< SHA interrupt */
#define ECC_INTR_SOURCE                         57  /**< ECC interrupt */
#define ECDSA_INTR_SOURCE                       58  /**< ECDSA interrupt */
#define KM_INTR_SOURCE                          59  /**< KM interrupt */
#define RMA_INTR_SOURCE                         60  /**< RMA interrupt */
#define GPIO_INTR0_SOURCE                       61  /**< GPIO INTR0 interrupt */
#define GPIO_INTR1_SOURCE                       62  /**< GPIO INTR1 interrupt */
#define GPIO_INTR2_SOURCE                       63  /**< GPIO INTR2 interrupt */
#define GPIO_INTR3_SOURCE                       64  /**< GPIO INTR3 interrupt */
#define CPU_INTR_FROM_CPU_0_SOURCE              65  /**< CPU INTR FROM CPU 0 interrupt */
#define CPU_INTR_FROM_CPU_1_SOURCE              66  /**< CPU INTR FROM CPU 1 interrupt */
#define CPU_INTR_FROM_CPU_2_SOURCE              67  /**< CPU INTR FROM CPU 2 interrupt */
#define CPU_INTR_FROM_CPU_3_SOURCE              68  /**< CPU INTR FROM CPU 3 interrupt */
#define CACHE_INTR_SOURCE                       69  /**< CACHE interrupt */
#define CPU_APM_M0_INTR_SOURCE                  70  /**< CPU APM M0 interrupt */
#define CPU_APM_M1_INTR_SOURCE                  71  /**< CPU APM M1 interrupt */
#define CPU_APM_M2_INTR_SOURCE                  72  /**< CPU APM M2 interrupt */
#define CPU_APM_M3_INTR_SOURCE                  73  /**< CPU APM M3 interrupt */
#define HP_MEM_APM_M0_INTR_SOURCE               74  /**< HP MEM APM M0 interrupt */
#define HP_MEM_APM_M1_INTR_SOURCE               75  /**< HP MEM APM M1 interrupt */
#define HP_MEM_APM_M2_INTR_SOURCE               76  /**< HP MEM APM M2 interrupt */
#define HP_MEM_APM_M3_INTR_SOURCE               77  /**< HP MEM APM M3 interrupt */
#define HP_MEM_APM_M4_INTR_SOURCE               78  /**< HP MEM APM M4 interrupt */
#define HP_MEM_APM_M5_INTR_SOURCE               79  /**< HP MEM APM M5 interrupt */
#define CPU_PERI0_TIMEOUT_INTR_SOURCE           80  /**< CPU PERI0 TIMEOUT interrupt */
#define CPU_PERI1_TIMEOUT_INTR_SOURCE           81  /**< CPU PERI1 TIMEOUT interrupt */
#define HP_PERI0_TIMEOUT_INTR_SOURCE            82  /**< HP PERI0 TIMEOUT interrupt */
#define HP_PERI1_TIMEOUT_INTR_SOURCE            83  /**< HP PERI1 TIMEOUT interrupt */
#define HP_APM_M0_INTR_SOURCE                   84  /**< HP APM M0 interrupt */
#define HP_APM_M1_INTR_SOURCE                   85  /**< HP APM M1 interrupt */
#define HP_APM_M2_INTR_SOURCE                   86  /**< HP APM M2 interrupt */
#define HP_APM_M3_INTR_SOURCE                   87  /**< HP APM M3 interrupt */
#define HP_APM_M4_INTR_SOURCE                   88  /**< HP APM M4 interrupt */
#define HP_APM_M5_INTR_SOURCE                   89  /**< HP APM M5 interrupt */
#define HP_APM_M6_INTR_SOURCE                   90  /**< HP APM M6 interrupt */
#define HP_PERI0_PMS_INTR_SOURCE                91  /**< HP PERI0 PMS interrupt */
#define HP_PERI1_PMS_INTR_SOURCE                92  /**< HP PERI1 PMS interrupt */
#define CPU0_PERI_PMS_INTR_SOURCE               93  /**< CPU0 PERI PMS interrupt */
#define CPU1_PERI_PMS_INTR_SOURCE               94  /**< CPU1 PERI PMS interrupt */
#define MSPI_FLASH_INTR_SOURCE                  95  /**< MSPI FLASH interrupt */
#define LPI_INTR_SOURCE                         96  /**< LPI interrupt */
#define PMT_INTR_SOURCE                         97  /**< PMT interrupt */
#define SBD_INTR_SOURCE                         98  /**< SBD interrupt */
#define USB_OTGHS_INTR_SOURCE                   99  /**< USB OTGHS interrupt */
#define USB_OTGHS_ENDP_MULTI_PROC_INTR_SOURCE   100 /**< USB OTGHS ENDP MULTI PROC interrupt */
#define JPEG_INTR_SOURCE                        101 /**< JPEG interrupt */
#define PPA_INTR_SOURCE                         102 /**< PPA interrupt */
#define CORE0_TRACE_INTR_SOURCE                 103 /**< CORE0 TRACE interrupt */
#define CORE1_TRACE_INTR_SOURCE                 104 /**< CORE1 TRACE interrupt */
#define DMA2D_IN_CH0_INTR_SOURCE                105 /**< DMA2D IN CH0 interrupt */
#define DMA2D_IN_CH1_INTR_SOURCE                106 /**< DMA2D IN CH1 interrupt */
#define DMA2D_IN_CH2_INTR_SOURCE                107 /**< DMA2D IN CH2 interrupt */
#define DMA2D_OUT_CH0_INTR_SOURCE               108 /**< DMA2D OUT CH0 interrupt */
#define DMA2D_OUT_CH1_INTR_SOURCE               109 /**< DMA2D OUT CH1 interrupt */
#define DMA2D_OUT_CH2_INTR_SOURCE               110 /**< DMA2D OUT CH2 interrupt */
#define DMA2D_OUT_CH3_INTR_SOURCE               111 /**< DMA2D OUT CH3 interrupt */
#define MSPI_PSRAM_INTR_SOURCE                  112 /**< MSPI PSRAM interrupt */
#define HP_SYSREG_INTR_SOURCE                   113 /**< HP SYSREG interrupt */
#define PCNT0_INTR_SOURCE                       114 /**< PCNT0 interrupt */
#define PCNT1_INTR_SOURCE                       115 /**< PCNT1 interrupt */
#define HP_PAU_INTR_SOURCE                      116 /**< HP PAU interrupt */
#define HP_PARLIO_RX_INTR_SOURCE                117 /**< HP PARLIO RX interrupt */
#define HP_PARLIO_TX_INTR_SOURCE                118 /**< HP PARLIO TX interrupt */
#define ASSIST_DEBUG_INTR_SOURCE                119 /**< ASSIST DEBUG interrupt */
#define MODEM_WIFI_MAC_INTR_SOURCE              120 /**< MODEM WIFI MAC interrupt */
#define MODEM_WIFI_MAC_NMI_INTR_SOURCE          121 /**< MODEM WIFI MAC NMI interrupt */
#define MODEM_WIFI_PWR_INTR_SOURCE              122 /**< MODEM WIFI PWR interrupt */
#define MODEM_WIFI_BB_INTR_SOURCE               123 /**< MODEM WIFI BB interrupt */
#define MODEM_BT_MAC_INTR_SOURCE                124 /**< MODEM BT MAC interrupt */
#define MODEM_BT_BB_INTR_SOURCE                 125 /**< MODEM BT BB interrupt */
#define MODEM_BT_BB_NMI_INTR_SOURCE             126 /**< MODEM BT BB NMI interrupt */
#define MODEM_LP_TIMER_INTR_SOURCE              127 /**< MODEM LP TIMER interrupt */
#define MODEM_COEX_INTR_SOURCE                  128 /**< MODEM COEX interrupt */
#define MODEM_BLE_TIMER_INTR_SOURCE             129 /**< MODEM BLE TIMER interrupt */
#define MODEM_BLE_SEC_INTR_SOURCE               130 /**< MODEM BLE SEC interrupt */
#define MODEM_I2C_MST_INTR_SOURCE               131 /**< MODEM I2C MST interrupt */
#define MODEM_ZB_MAC_INTR_SOURCE                132 /**< MODEM ZB MAC interrupt */
#define MODEM_BT_MAC_INT1_INTR_SOURCE           133 /**< MODEM BT MAC INT1 interrupt */
#define CORDIC_INTR_SOURCE                      134 /**< CORDIC interrupt */
#define ZERO_DET_INTR_SOURCE                    135 /**< ZERO DET interrupt */
#define LP_WDT_INTR_SOURCE                      136 /**< LP WDT interrupt */
#define LP_TIMER_REG_0_INTR_SOURCE              137 /**< LP TIMER REG 0 interrupt */
#define LP_TIMER_REG_1_INTR_SOURCE              138 /**< LP TIMER REG 1 interrupt */
#define MB_HP_INTR_SOURCE                       139 /**< MB HP interrupt */
#define MB_LP_INTR_SOURCE                       140 /**< MB LP interrupt */
#define PMU_REG_0_INTR_SOURCE                   141 /**< PMU REG 0 interrupt */
#define PMU_REG_1_INTR_SOURCE                   142 /**< PMU REG 1 interrupt */
#define LP_ANAPERI_INTR_SOURCE                  143 /**< LP ANAPERI interrupt */
#define LP_ADC_INTR_SOURCE                      144 /**< LP ADC interrupt */
#define LP_DAC_INTR_SOURCE                      145 /**< LP DAC interrupt */
#define LP_GPIO_INTR_SOURCE                     146 /**< LP GPIO interrupt */
#define LP_I2C_INTR_SOURCE                      147 /**< LP I2C interrupt */
#define LP_SPI_INTR_SOURCE                      148 /**< LP SPI interrupt */
#define LP_TOUCH_INTR_SOURCE                    149 /**< LP TOUCH interrupt */
#define LP_TSENS_INTR_SOURCE                    150 /**< LP TSENS interrupt */
#define LP_UART_INTR_SOURCE                     151 /**< LP UART interrupt */
#define LP_EFUSE_INTR_SOURCE                    152 /**< LP EFUSE interrupt */
#define LP_SW_INTR_SOURCE                       153 /**< LP SW interrupt */
#define LP_TRNG_INTR_SOURCE                     154 /**< LP TRNG interrupt */
#define LP_SYSREG_INTR_SOURCE                   155 /**< LP SYSREG interrupt */
#define LP_APM_M0_INTR_SOURCE                   156 /**< LP APM M0 interrupt */
#define LP_APM_M1_INTR_SOURCE                   157 /**< LP APM M1 interrupt */
#define LP_APM_M2_INTR_SOURCE                   158 /**< LP APM M2 interrupt */
#define LP_APM_M3_INTR_SOURCE                   159 /**< LP APM M3 interrupt */
#define LP_PERI0_PMS_INTR_SOURCE                160 /**< LP PERI0 PMS interrupt */
#define LP_PERI1_PMS_INTR_SOURCE                161 /**< LP PERI1 PMS interrupt */
#define LP_HUK_INTR_SOURCE                      162 /**< LP HUK interrupt */
#define LP_PERI_TIMEOUT_INTR_SOURCE             163 /**< LP PERI TIMEOUT interrupt */
#define LP_AHB_PDMA_IN_CH0_INTR_SOURCE          164 /**< LP AHB PDMA IN CH0 interrupt */
#define LP_AHB_PDMA_IN_CH1_INTR_SOURCE          165 /**< LP AHB PDMA IN CH1 interrupt */
#define LP_AHB_PDMA_OUT_CH0_INTR_SOURCE         166 /**< LP AHB PDMA OUT CH0 interrupt */
#define LP_AHB_PDMA_OUT_CH1_INTR_SOURCE         167 /**< LP AHB PDMA OUT CH1 interrupt */
#define LP_SW_INVALID_SLEEP_INTR_SOURCE         168 /**< LP SW INVALID SLEEP interrupt */

#define IRQ_DEFAULT_PRIORITY 0

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_ESP32S31_INTMUX_H_ */
