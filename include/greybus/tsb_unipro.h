/*
 * Copyright (c) 2015 Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @author: Marti Bolivar
 */

#ifndef _NUTTX_GREYBUS_TSB_UNIPRO_H_
#define _NUTTX_GREYBUS_TSB_UNIPRO_H_

#include <nuttx/unipro/unipro.h>

/*
 * TSB attributes
 */
#define TSB_T_REGACCCTRL_TESTONLY  0x007f
#define TSB_DME_DDBL2_A            0x6000
#define TSB_DME_DDBL2_B            0x6001
#define TSB_DME_ES3_INIT_STATUS    0x6101
#define TSB_MAILBOX                0xa000
    #define TSB_MAIL_RESET         (0x00)
    #define TSB_MAIL_READY_AP      (0x01)
    #define TSB_MAIL_READY_OTHER   (0x02)
#define TSB_DME_LAYERENABLEREQ     0xd000
    #define TSB_LAYER_UNIPRO_EN    (0x01)
#define TSB_DME_LAYERENABLECNF     0xd000
    #define TSB_LAYER_SUCCESS      (0x01)
    #define TSB_LAYER_FAILURE      (0x02)
#define TSB_DME_RESETREQ           0xd010
    #define TSB_RESET_COLD         (0x01)
    #define TSB_RESET_WARM         (0x02)
#define TSB_DME_RESETCNF           0xd010
    #define TSB_RESET_COMPLETED    (0x01)
#define TSB_DME_ENDPOINTRESETREQ   0xd011
#define TSB_DME_ENDPOINTRESETCNF   0xd011
#define TSB_DME_ENDPOINTRESETIND   0xd012
#define TSB_DME_LINKSTARTUPREQ     0xd020
    #define TSB_LINKUP_INITIATE    (0x01)
#define TSB_DME_LINKSTARTUPCNF     0xd020
    #define TSB_LINKUP_SUCCESS     (0x01)
    #define TSB_LINKUP_FAIL        (0x02)
#define TSB_DME_LINKSTARTUPIND     0xd021
    #define TSB_LINKUP_IND_FAIL    (0x00)
    #define TSB_LINKUP_IND_SUCCESS (0x01)
#define TSB_DME_LINKLOSTIND        0xd022
#define TSB_DME_HIBERNATEENTERREQ  0xd030
#define TSB_DME_HIBERNATEENTERCNF  0xd030
#define TSB_DME_HIBERNATEENTERIND  0xd031
#define TSB_DME_HIBERNATEEXITREQ   0xd032
#define TSB_DME_HIBERNATEEXITCNF   0xd032
#define TSB_DME_HIBERNATEEXITIND   0xd033
#define TSB_DME_POWERMODEIND       0xd040
#define TSB_DME_TESTMODEREQ        0xd050
#define TSB_DME_TESTMODECNF        0xd050
#define TSB_DME_TESTMODEIND        0xd051
#define TSB_DME_ERRORPHYIND        0xd060
#define TSB_DME_ERRORPAIND         0xd061
#define TSB_DME_ERRORDIND          0xd062
#define TSB_DME_ERRORNIND          0xd063
#define TSB_DME_ERRORTIND          0xd064
#define TSB_INTERRUPTENABLE        0xd080
#define TSB_INTERRUPTSTATUS        0xd081
    #define TSB_INTERRUPTSTATUS_MAILBOX (1 << 15)
#define TSB_L2STATUS               0xd082
#define TSB_POWERSTATE             0xd083
#define TSB_TXBURSTCLOSUREDELAY    0xd084
#define TSB_MPHYCFGUPDT            0xd085
#define TSB_ADJUSTTRAILINGCLOCKS   0xd086
#define TSB_SUPPRESSRREQ           0xd087
#define TSB_L2TIMEOUT              0xd088
#define TSB_MAXSEGMENTCONFIG       0xd089
#define TSB_TBD                    0xd090
#define TSB_RBD                    0xd091
#define TSB_DEBUGTXBYTECOUNT       0xd092
#define TSB_DEBUGRXBYTECOUNT       0xd093
#define TSB_DEBUGINVALIDBYTEENABLE 0xd094
#define TSB_DEBUGLINKSTARTUP       0xd095
#define TSB_DEBUGPWRCHANGE         0xd096
#define TSB_DEBUGSTATES            0xd097
#define TSB_DEBUGCOUNTER0          0xd098
#define TSB_DEBUGCOUNTER1          0xd099
#define TSB_DEBUGCOUNTER0MASK      0xd09a
#define TSB_DEBUGCOUNTER1MASK      0xd09b
#define TSB_DEBUGCOUNTERCONTROL    0xd09c
#define TSB_DEBUGCOUNTEROVERFLOW   0xd09d
#define TSB_DEBUGOMC               0xd09e
#define TSB_DEBUGCOUNTERBMASK      0xd09f
#define TSB_DEBUGSAVECONFIGTIME    0xd0a0
#define TSB_DEBUGCLOCKENABLE       0xd0a1
#define TSB_DEEPSTALLCFG           0xd0a2
#define TSB_DEEPSTALLSTATUS        0xd0a3

/* Special attributes for dealing with chip-specific Mailbox ACK attribute */
#define ES2_MBOX_ACK_ATTR       T_TSTSRCINTERMESSAGEGAP
#define ES3_SYSTEM_STATUS_15    0x610f
#define ES3_MBOX_ACK_ATTR       ES3_SYSTEM_STATUS_15

/* Special-purpose usage of existing atttributes */
#define TSB_ARA_VID                TSB_DME_DDBL2_A
#define TSB_ARA_PID                TSB_DME_DDBL2_B

int tsb_unipro_mbox_send(uint32_t val);

/* Init status values */
#define INIT_STATUS_UNINITIALIZED                            (0x000000000)
#define INIT_STATUS_OPERATING                                (1 << 24)
#define INIT_STATUS_SPI_BOOT_STARTED                         (2 << 24)
#define INIT_STATUS_TRUSTED_SPI_FLASH_BOOT_FINISHED          (3 << 24)
#define INIT_STATUS_UNTRUSTED_SPI_FLASH_BOOT_FINISHED        (4 << 24)
#define INIT_STATUS_UNIPRO_BOOT_STARTED                      (6 << 24)
#define INIT_STATUS_TRUSTED_UNIPRO_BOOT_FINISHED             (7 << 24)
#define INIT_STATUS_UNTRUSTED_UNIPRO_BOOT_FINISHED           (8 << 24)
#define INIT_STATUS_FALLLBACK_UNIPRO_BOOT_STARTED            (9 << 24)
#define INIT_STATUS_FALLLBACK_TRUSTED_UNIPRO_BOOT_FINISHED   (10 << 24)
#define INIT_STATUS_FALLLBACK_UNTRUSTED_UNIPRO_BOOT_FINISHED (11 << 24)
#define INIT_STATUS_RESUMED_FROM_STANDBY                     (12 << 24)
#define INIT_STATUS_S3FW_BOOT_FINISHED                       (16 << 24)
#define INIT_STATUS_FAILED                                   (0x80000000)
#define INIT_STATUS_ERROR_MASK                               (0x80000000)
#define INIT_STATUS_STATUS_MASK                              (0x7f000000)
#define INIT_STATUS_ERROR_CODE_MASK                          (0x00ffffff)

int tsb_unipro_set_init_status(uint32_t val);

#endif
