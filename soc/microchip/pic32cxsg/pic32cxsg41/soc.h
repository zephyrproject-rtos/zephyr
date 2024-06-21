/*
 * Copyright (c) 2024 Microchip
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_MICROCHIP_PIC32CXSG41_SOC_H_
#define _SOC_MICROCHIP_PIC32CXSG41_SOC_H_

#ifndef _ASMLANGUAGE

#define DONT_USE_CMSIS_INIT

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CX1025SG41064)
#include <pic32cx1025sg41064.h>
#elif defined(CONFIG_SOC_PIC32CX1025SG41100)
#include <pic32cx1025sg41100.h>
#elif defined(CONFIG_SOC_PIC32CX1025SG41128)
#include <pic32cx1025sg41128.h>

#else
#error Library does not support the specified device.
#endif

#endif /* _ASMLANGUAGE */

#include "sercom_fixup_pic32cxsg.h"
#include "tc_fixup_pic32cxsg.h"
#include "gmac_fixup_pic32cxsg.h"
#include "adc_fixup_pic32cxsg.h"

#include "../common/soc_port.h"
#include "../common/microchip_pic32cxsg_dt.h"

#include "../common/component/ac_component_fixup_pic32cxsg.h"
#include "../common/component/adc_component_fixup_pic32cxsg.h"
#include "../common/component/aes_component_fixup_pic32cxsg.h"
#include "../common/component/can_component_fixup_pic32cxsg.h"
#include "../common/component/ccl_component_fixup_pic32cxsg.h"
#include "../common/component/cmcc_component_fixup_pic32cxsg.h"
#include "../common/component/dac_component_fixup_pic32cxsg.h"
#include "../common/component/dmac_component_fixup_pic32cxsg.h"
#include "../common/component/dsu_component_fixup_pic32cxsg.h"
#include "../common/component/eic_component_fixup_pic32cxsg.h"
#include "../common/component/evsys_component_fixup_pic32cxsg.h"
#include "../common/component/freqm_component_fixup_pic32cxsg.h"
#include "../common/component/gclk_component_fixup_pic32cxsg.h"
#include "../common/component/gmac_component_fixup_pic32cxsg.h"
#include "../common/component/hmatrixb_component_fixup_pic32cxsg.h"
#include "../common/component/i2s_component_fixup_pic32cxsg.h"
#include "../common/component/icm_component_fixup_pic32cxsg.h"
#include "../common/component/mclk_component_fixup_pic32cxsg.h"
#include "../common/component/nvmctrl_component_fixup_pic32cxsg.h"
#include "../common/component/osc32kctrl_component_fixup_pic32cxsg.h"
#include "../common/component/oscctrl_component_fixup_pic32cxsg.h"
#include "../common/component/pac_component_fixup_pic32cxsg.h"
#include "../common/component/pcc_component_fixup_pic32cxsg.h"
#include "../common/component/pdec_component_fixup_pic32cxsg.h"
#include "../common/component/port_component_fixup_pic32cxsg.h"
#include "../common/component/qspi_component_fixup_pic32cxsg.h"
#include "../common/component/ramecc_component_fixup_pic32cxsg.h"
#include "../common/component/rstc_component_fixup_pic32cxsg.h"
#include "../common/component/rtc_component_fixup_pic32cxsg.h"
#include "../common/component/sdhc_component_fixup_pic32cxsg.h"
#include "../common/component/sercom_component_fixup_pic32cxsg.h"
#include "../common/component/supc_component_fixup_pic32cxsg.h"
#include "../common/component/tc_component_fixup_pic32cxsg.h"
#include "../common/component/tcc_component_fixup_pic32cxsg.h"
#include "../common/component/trng_component_fixup_pic32cxsg.h"
#include "../common/component/usb_component_fixup_pic32cxsg.h"
#include "../common/component/wdt_component_fixup_pic32cxsg.h"

#define SOC_MICROCHIP_PIC32CXSG_OSC32K_FREQ_HZ 32768
#define SOC_MICROCHIP_PIC32CXSG_DFLL48_FREQ_HZ 48000000

/** Processor Clock (HCLK) Frequency */
#define SOC_MICROCHIP_PIC32CXSG_HCLK_FREQ_HZ 	CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
/** Master Clock (MCK) Frequency */
#define SOC_MICROCHIP_PIC32CXSG_MCK_FREQ_HZ 	SOC_MICROCHIP_PIC32CXSG_HCLK_FREQ_HZ
#define SOC_MICROCHIP_PIC32CXSG_GCLK0_FREQ_HZ 	SOC_MICROCHIP_PIC32CXSG_MCK_FREQ_HZ
#define SOC_MICROCHIP_PIC32CXSG_GCLK2_FREQ_HZ 	48000000

#define SOC_ATMEL_SAM0_HCLK_FREQ_HZ 			CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#define SOC_ATMEL_SAM0_MCK_FREQ_HZ 				SOC_MICROCHIP_PIC32CXSG_HCLK_FREQ_HZ
#define SOC_ATMEL_SAM0_GCLK0_FREQ_HZ 			SOC_MICROCHIP_PIC32CXSG_MCK_FREQ_HZ
#define SOC_ATMEL_SAM0_GCLK2_FREQ_HZ 			48000000

#endif /* _SOC_MICROCHIP_PIC32CXSG41_SOC_H_ */
