/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MEC_SOC_H
#define __MEC_SOC_H

#define SYSCLK_DEFAULT_IOSC_HZ MHZ(96)

#ifndef _ASMLANGUAGE

#include <sys/util.h>

#include "reg/mec172x_regs.h"
#include "reg/mec_defs.h"
#include "reg/mec_regaccess.h"
#include "reg/mec_acpi_ec.h"
#include "reg/mec_adc.h"
#include "reg/mec_bcl.h"
#include "reg/mec_dma.h"
#include "reg/mec_ecia.h"
#include "reg/mec_ecs.h"
#include "reg/mec_eepromc.h"
#include "reg/mec_emi.h"
#include "reg/mec_espi_io.h"
#include "reg/mec_espi_mem.h"
#include "reg/mec_espi_saf.h"
#include "reg/mec_espi_vw.h"
#include "reg/mec_global_cfg.h"
#include "reg/mec_glue.h"
#include "reg/mec_gpio.h"
#include "reg/mec_gpspi.h"
#include "reg/mec_i2c_smb.h"
#include "reg/mec_kbc.h"
#include "reg/mec_keyscan.h"
#include "reg/mec_led.h"
#include "reg/mec_mailbox.h"
#include "reg/mec_p80bd.h"
#include "reg/mec_pcr.h"
#include "reg/mec_peci.h"
#include "reg/mec_port92.h"
#include "reg/mec_prochot.h"
#include "reg/mec_ps2_ctrl.h"
#include "reg/mec_pwm.h"
#include "reg/mec_qspi.h"
#include "reg/mec_rcid.h"
#include "reg/mec_rpmfan.h"
#include "reg/mec_rtc.h"
#include "reg/mec_spip.h"
#include "reg/mec_tach.h"
#include "reg/mec_tfdp.h"
#include "reg/mec_timers.h"
#include "reg/mec_uart.h"
#include "reg/mec_vbat.h"
#include "reg/mec_wdt.h"

#include "../common/soc_dma.h"
#include "../common/soc_gpio.h"
#include "../common/soc_i2c.h"
#include "../common/soc_pins.h"
#include "../common/soc_espi_channels.h"
#include "../common/soc_espi_saf.h"

uint32_t soc_get_core_clock(void);

#endif

#endif
