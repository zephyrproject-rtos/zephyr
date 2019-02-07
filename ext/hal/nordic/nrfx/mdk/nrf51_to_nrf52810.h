/*

Copyright (c) 2010 - 2018, Nordic Semiconductor ASA All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the name of Nordic Semiconductor ASA nor the names of its
   contributors may be used to endorse or promote products derived from this
   software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef NRF51_TO_NRF52810_H
#define NRF51_TO_NRF52810_H

/*lint ++flb "Enter library region */

/* This file is given to prevent your SW from not compiling with the name changes between nRF51 and nRF52840 devices.
 * It redefines the old nRF51 names into the new ones as long as the functionality is still supported. If the
 * functionality is gone, there old names are not defined, so compilation will fail. Note that also includes macros
 * from the nrf51_deprecated.h file. */


 /* Differences between latest nRF51 headers and nRF52810 headers. */
 
/* IRQ */
/* Several peripherals have been added to several indexes. Names of IRQ handlers and IRQ numbers have changed. */
#ifndef SWI0_IRQHandler
    #define SWI0_IRQHandler         SWI0_EGU0_IRQHandler
#endif
#ifndef SWI1_IRQHandler
    #define SWI1_IRQHandler         SWI1_EGU1_IRQHandler
#endif

#ifndef SWI0_IRQn
    #define SWI0_IRQn               SWI0_EGU0_IRQn
#endif
#ifndef SWI1_IRQn
    #define SWI1_IRQn               SWI1_EGU1_IRQn
#endif


/* UICR */
/* Register RBPCONF was renamed to APPROTECT. */
#ifndef RBPCONF
    #define RBPCONF     APPROTECT
#endif

#ifndef UICR_RBPCONF_PALL_Pos
    #define UICR_RBPCONF_PALL_Pos           UICR_APPROTECT_PALL_Pos
#endif
#ifndef UICR_RBPCONF_PALL_Msk
    #define UICR_RBPCONF_PALL_Msk           UICR_APPROTECT_PALL_Msk
#endif
#ifndef UICR_RBPCONF_PALL_Enabled
    #define UICR_RBPCONF_PALL_Enabled       UICR_APPROTECT_PALL_Enabled
#endif
#ifndef UICR_RBPCONF_PALL_Disabled
    #define UICR_RBPCONF_PALL_Disabled      UICR_APPROTECT_PALL_Disabled
#endif


/* GPIO */
/* GPIO port was renamed to P0. */
#ifndef NRF_GPIO
    #define NRF_GPIO        NRF_P0
#endif
#ifndef NRF_GPIO_BASE
    #define NRF_GPIO_BASE   NRF_P0_BASE
#endif


/* QDEC */
/* The registers PSELA, PSELB and PSELLED were restructured into a struct. */
#ifndef PSELLED
    #define PSELLED     PSEL.LED
#endif
#ifndef PSELA
    #define PSELA       PSEL.A
#endif
#ifndef PSELB
    #define PSELB       PSEL.B
#endif


/* SPIS */
/* The registers PSELSCK, PSELMISO, PSELMOSI, PSELCSN were restructured into a struct. */
#ifndef PSELSCK
    #define PSELSCK       PSEL.SCK
#endif
#ifndef PSELMISO
    #define PSELMISO      PSEL.MISO
#endif
#ifndef PSELMOSI
    #define PSELMOSI      PSEL.MOSI
#endif
#ifndef PSELCSN
    #define PSELCSN       PSEL.CSN
#endif 


/* The registers RXDPTR, MAXRX, AMOUNTRX were restructured into a struct */
#ifndef RXDPTR
    #define RXDPTR        RXD.PTR
#endif
#ifndef MAXRX
    #define MAXRX         RXD.MAXCNT
#endif
#ifndef AMOUNTRX
    #define AMOUNTRX      RXD.AMOUNT
#endif

#ifndef SPIS_MAXRX_MAXRX_Pos
    #define SPIS_MAXRX_MAXRX_Pos        SPIS_RXD_MAXCNT_MAXCNT_Pos
#endif
#ifndef SPIS_MAXRX_MAXRX_Msk
    #define SPIS_MAXRX_MAXRX_Msk        SPIS_RXD_MAXCNT_MAXCNT_Msk
#endif

#ifndef SPIS_AMOUNTRX_AMOUNTRX_Pos
    #define SPIS_AMOUNTRX_AMOUNTRX_Pos  SPIS_RXD_AMOUNT_AMOUNT_Pos
#endif
#ifndef SPIS_AMOUNTRX_AMOUNTRX_Msk
    #define SPIS_AMOUNTRX_AMOUNTRX_Msk  SPIS_RXD_AMOUNT_AMOUNT_Msk
#endif

/* The registers TXDPTR, MAXTX, AMOUNTTX were restructured into a struct */
#ifndef TXDPTR
    #define TXDPTR        TXD.PTR
#endif
#ifndef MAXTX
    #define MAXTX         TXD.MAXCNT
#endif
#ifndef AMOUNTTX
    #define AMOUNTTX      TXD.AMOUNT
#endif

#ifndef SPIS_MAXTX_MAXTX_Pos
    #define SPIS_MAXTX_MAXTX_Pos        SPIS_TXD_MAXCNT_MAXCNT_Pos
#endif
#ifndef SPIS_MAXTX_MAXTX_Msk
    #define SPIS_MAXTX_MAXTX_Msk        SPIS_TXD_MAXCNT_MAXCNT_Msk
#endif

#ifndef SPIS_AMOUNTTX_AMOUNTTX_Pos
    #define SPIS_AMOUNTTX_AMOUNTTX_Pos  SPIS_TXD_AMOUNT_AMOUNT_Pos
#endif
#ifndef SPIS_AMOUNTTX_AMOUNTTX_Msk
    #define SPIS_AMOUNTTX_AMOUNTTX_Msk  SPIS_TXD_AMOUNT_AMOUNT_Msk
#endif


/* From nrf51_deprecated.h. Several macros changed in different versions of nRF52 headers. By defining the following, any code written for any version of nRF52 headers will still compile. */

/* NVMC */
/* The register ERASEPROTECTEDPAGE changed name to ERASEPCR0 in the documentation. */
#ifndef ERASEPROTECTEDPAGE
    #define ERASEPROTECTEDPAGE      ERASEPCR0
#endif


/* RADIO */
/* The name of the field SKIPADDR was corrected. Old macros added for compatibility. */
#ifndef RADIO_CRCCNF_SKIP_ADDR_Pos
    #define RADIO_CRCCNF_SKIP_ADDR_Pos      RADIO_CRCCNF_SKIPADDR_Pos
#endif
#ifndef RADIO_CRCCNF_SKIP_ADDR_Msk
    #define RADIO_CRCCNF_SKIP_ADDR_Msk      RADIO_CRCCNF_SKIPADDR_Msk
#endif
#ifndef RADIO_CRCCNF_SKIP_ADDR_Include
    #define RADIO_CRCCNF_SKIP_ADDR_Include  RADIO_CRCCNF_SKIPADDR_Include
#endif
#ifndef RADIO_CRCCNF_SKIP_ADDR_Skip
    #define RADIO_CRCCNF_SKIP_ADDR_Skip     RADIO_CRCCNF_SKIPADDR_Skip
#endif


/* FICR */
/* The registers FICR.DEVICEID0 and FICR.DEVICEID1 were renamed into an array. */
#ifndef DEVICEID0
    #define DEVICEID0       DEVICEID[0]
#endif
#ifndef DEVICEID1
    #define DEVICEID1       DEVICEID[1]
#endif

/* The registers FICR.ER0, FICR.ER1, FICR.ER2 and FICR.ER3 were renamed into an array. */
#ifndef ER0
    #define ER0             ER[0]
#endif
#ifndef ER1
    #define ER1             ER[1]
#endif
#ifndef ER2
    #define ER2             ER[2]
#endif
#ifndef ER3
    #define ER3             ER[3]
#endif

/* The registers FICR.IR0, FICR.IR1, FICR.IR2 and FICR.IR3 were renamed into an array. */
#ifndef IR0
    #define IR0             IR[0]
#endif
#ifndef IR1
    #define IR1             IR[1]
#endif
#ifndef IR2
    #define IR2             IR[2]
#endif
#ifndef IR3
    #define IR3             IR[3]
#endif

/* The registers FICR.DEVICEADDR0 and FICR.DEVICEADDR1 were renamed into an array. */
#ifndef DEVICEADDR0
    #define DEVICEADDR0     DEVICEADDR[0]
#endif
#ifndef DEVICEADDR1
    #define DEVICEADDR1     DEVICEADDR[1]
#endif


/* PPI */
/* The tasks PPI.TASKS_CHGxEN and PPI.TASKS_CHGxDIS were renamed into an array of structs. */
#ifndef TASKS_CHG0EN
    #define TASKS_CHG0EN     TASKS_CHG[0].EN
#endif
#ifndef TASKS_CHG0DIS
    #define TASKS_CHG0DIS    TASKS_CHG[0].DIS
#endif
#ifndef TASKS_CHG1EN
    #define TASKS_CHG1EN     TASKS_CHG[1].EN
#endif
#ifndef TASKS_CHG1DIS
    #define TASKS_CHG1DIS    TASKS_CHG[1].DIS
#endif
#ifndef TASKS_CHG2EN
    #define TASKS_CHG2EN     TASKS_CHG[2].EN
#endif
#ifndef TASKS_CHG2DIS
    #define TASKS_CHG2DIS    TASKS_CHG[2].DIS
#endif
#ifndef TASKS_CHG3EN
    #define TASKS_CHG3EN     TASKS_CHG[3].EN
#endif
#ifndef TASKS_CHG3DIS
    #define TASKS_CHG3DIS    TASKS_CHG[3].DIS
#endif

/* The registers PPI.CHx_EEP and PPI.CHx_TEP were renamed into an array of structs. */
#ifndef CH0_EEP
    #define CH0_EEP          CH[0].EEP
#endif
#ifndef CH0_TEP
    #define CH0_TEP          CH[0].TEP
#endif
#ifndef CH1_EEP
    #define CH1_EEP          CH[1].EEP
#endif
#ifndef CH1_TEP
    #define CH1_TEP          CH[1].TEP
#endif
#ifndef CH2_EEP
    #define CH2_EEP          CH[2].EEP
#endif
#ifndef CH2_TEP
    #define CH2_TEP          CH[2].TEP
#endif
#ifndef CH3_EEP
    #define CH3_EEP          CH[3].EEP
#endif
#ifndef CH3_TEP
    #define CH3_TEP          CH[3].TEP
#endif
#ifndef CH4_EEP
    #define CH4_EEP          CH[4].EEP
#endif
#ifndef CH4_TEP
    #define CH4_TEP          CH[4].TEP
#endif
#ifndef CH5_EEP
    #define CH5_EEP          CH[5].EEP
#endif
#ifndef CH5_TEP
    #define CH5_TEP          CH[5].TEP
#endif
#ifndef CH6_EEP
    #define CH6_EEP          CH[6].EEP
#endif
#ifndef CH6_TEP
    #define CH6_TEP          CH[6].TEP
#endif 
#ifndef CH7_EEP
    #define CH7_EEP          CH[7].EEP
#endif
#ifndef CH7_TEP
    #define CH7_TEP          CH[7].TEP
#endif
#ifndef CH8_EEP
    #define CH8_EEP          CH[8].EEP
#endif
#ifndef CH8_TEP
    #define CH8_TEP          CH[8].TEP
#endif
#ifndef CH9_EEP
    #define CH9_EEP          CH[9].EEP
#endif
#ifndef CH9_TEP
    #define CH9_TEP          CH[9].TEP
#endif
#ifndef CH10_EEP
    #define CH10_EEP         CH[10].EEP
#endif
#ifndef CH10_TEP
    #define CH10_TEP         CH[10].TEP
#endif
#ifndef CH11_EEP
    #define CH11_EEP         CH[11].EEP
#endif
#ifndef CH11_TEP
    #define CH11_TEP         CH[11].TEP
#endif
#ifndef CH12_EEP
    #define CH12_EEP         CH[12].EEP
#endif
#ifndef CH12_TEP
    #define CH12_TEP         CH[12].TEP
#endif
#ifndef CH13_EEP
    #define CH13_EEP         CH[13].EEP
#endif
#ifndef CH13_TEP
    #define CH13_TEP         CH[13].TEP
#endif
#ifndef CH14_EEP
    #define CH14_EEP         CH[14].EEP
#endif
#ifndef CH14_TEP 
    #define CH14_TEP         CH[14].TEP
#endif
#ifndef CH15_EEP
    #define CH15_EEP         CH[15].EEP
#endif
#ifndef CH15_TEP
    #define CH15_TEP         CH[15].TEP
#endif

/* The registers PPI.CHG0, PPI.CHG1, PPI.CHG2 and PPI.CHG3 were renamed into an array. */
#ifndef CHG0
    #define CHG0             CHG[0]
#endif
#ifndef CHG1
    #define CHG1             CHG[1]
#endif
#ifndef CHG2
    #define CHG2             CHG[2]
#endif
#ifndef CHG3
    #define CHG3             CHG[3]
#endif

/* All bitfield macros for the CHGx registers therefore changed name. */
#ifndef PPI_CHG0_CH15_Pos
    #define PPI_CHG0_CH15_Pos       PPI_CHG_CH15_Pos
#endif 
#ifndef PPI_CHG0_CH15_Msk
    #define PPI_CHG0_CH15_Msk       PPI_CHG_CH15_Msk
#endif
#ifndef PPI_CHG0_CH15_Excluded
    #define PPI_CHG0_CH15_Excluded  PPI_CHG_CH15_Excluded
#endif
#ifndef PPI_CHG0_CH15_Included
    #define PPI_CHG0_CH15_Included  PPI_CHG_CH15_Included
#endif

#ifndef PPI_CHG0_CH14_Pos
    #define PPI_CHG0_CH14_Pos       PPI_CHG_CH14_Pos
#endif
#ifndef PPI_CHG0_CH14_Msk
    #define PPI_CHG0_CH14_Msk       PPI_CHG_CH14_Msk
#endif
#ifndef PPI_CHG0_CH14_Excluded
    #define PPI_CHG0_CH14_Excluded  PPI_CHG_CH14_Excluded
#endif
#ifndef PPI_CHG0_CH14_Included
    #define PPI_CHG0_CH14_Included  PPI_CHG_CH14_Included
#endif

#ifndef PPI_CHG0_CH13_Pos
    #define PPI_CHG0_CH13_Pos       PPI_CHG_CH13_Pos
#endif
#ifndef PPI_CHG0_CH13_Msk
    #define PPI_CHG0_CH13_Msk       PPI_CHG_CH13_Msk
#endif
#ifndef PPI_CHG0_CH13_Excluded
    #define PPI_CHG0_CH13_Excluded  PPI_CHG_CH13_Excluded
#endif
#ifndef PPI_CHG0_CH13_Included
    #define PPI_CHG0_CH13_Included  PPI_CHG_CH13_Included
#endif

#ifndef PPI_CHG0_CH12_Pos
    #define PPI_CHG0_CH12_Pos       PPI_CHG_CH12_Pos
#endif
#ifndef PPI_CHG0_CH12_Msk
    #define PPI_CHG0_CH12_Msk       PPI_CHG_CH12_Msk
#endif
#ifndef PPI_CHG0_CH12_Excluded
    #define PPI_CHG0_CH12_Excluded  PPI_CHG_CH12_Excluded
#endif
#ifndef PPI_CHG0_CH12_Included
    #define PPI_CHG0_CH12_Included  PPI_CHG_CH12_Included
#endif

#ifndef PPI_CHG0_CH11_Pos
    #define PPI_CHG0_CH11_Pos       PPI_CHG_CH11_Pos
#endif
#ifndef PPI_CHG0_CH11_Msk
    #define PPI_CHG0_CH11_Msk       PPI_CHG_CH11_Msk
#endif
#ifndef PPI_CHG0_CH11_Excluded
    #define PPI_CHG0_CH11_Excluded  PPI_CHG_CH11_Excluded
#endif
#ifndef PPI_CHG0_CH11_Included
    #define PPI_CHG0_CH11_Included  PPI_CHG_CH11_Included
#endif

#ifndef PPI_CHG0_CH10_Pos
    #define PPI_CHG0_CH10_Pos       PPI_CHG_CH10_Pos
#endif
#ifndef PPI_CHG0_CH10_Msk
    #define PPI_CHG0_CH10_Msk       PPI_CHG_CH10_Msk
#endif
#ifndef PPI_CHG0_CH10_Excluded
    #define PPI_CHG0_CH10_Excluded  PPI_CHG_CH10_Excluded
#endif
#ifndef PPI_CHG0_CH10_Included
    #define PPI_CHG0_CH10_Included  PPI_CHG_CH10_Included
#endif

#ifndef PPI_CHG0_CH9_Pos
    #define PPI_CHG0_CH9_Pos        PPI_CHG_CH9_Pos
#endif
#ifndef PPI_CHG0_CH9_Msk
    #define PPI_CHG0_CH9_Msk        PPI_CHG_CH9_Msk
#endif
#ifndef PPI_CHG0_CH9_Excluded
    #define PPI_CHG0_CH9_Excluded   PPI_CHG_CH9_Excluded
#endif
#ifndef PPI_CHG0_CH9_Included
    #define PPI_CHG0_CH9_Included   PPI_CHG_CH9_Included
#endif

#ifndef PPI_CHG0_CH8_Pos
    #define PPI_CHG0_CH8_Pos        PPI_CHG_CH8_Pos
#endif
#ifndef PPI_CHG0_CH8_Msk
    #define PPI_CHG0_CH8_Msk        PPI_CHG_CH8_Msk
#endif
#ifndef PPI_CHG0_CH8_Excluded
    #define PPI_CHG0_CH8_Excluded   PPI_CHG_CH8_Excluded
#endif
#ifndef PPI_CHG0_CH8_Included
    #define PPI_CHG0_CH8_Included   PPI_CHG_CH8_Included
#endif

#ifndef PPI_CHG0_CH7_Pos
    #define PPI_CHG0_CH7_Pos        PPI_CHG_CH7_Pos
#endif
#ifndef PPI_CHG0_CH7_Msk
    #define PPI_CHG0_CH7_Msk        PPI_CHG_CH7_Msk
#endif
#ifndef PPI_CHG0_CH7_Excluded
    #define PPI_CHG0_CH7_Excluded   PPI_CHG_CH7_Excluded
#endif
#ifndef PPI_CHG0_CH7_Included
    #define PPI_CHG0_CH7_Included   PPI_CHG_CH7_Included
#endif

#ifndef PPI_CHG0_CH6_Pos
    #define PPI_CHG0_CH6_Pos        PPI_CHG_CH6_Pos
#endif
#ifndef PPI_CHG0_CH6_Msk
    #define PPI_CHG0_CH6_Msk        PPI_CHG_CH6_Msk
#endif
#ifndef PPI_CHG0_CH6_Excluded
    #define PPI_CHG0_CH6_Excluded   PPI_CHG_CH6_Excluded
#endif
#ifndef PPI_CHG0_CH6_Included
    #define PPI_CHG0_CH6_Included   PPI_CHG_CH6_Included
#endif

#ifndef PPI_CHG0_CH5_Pos
    #define PPI_CHG0_CH5_Pos        PPI_CHG_CH5_Pos
#endif
#ifndef PPI_CHG0_CH5_Msk
    #define PPI_CHG0_CH5_Msk        PPI_CHG_CH5_Msk
#endif
#ifndef PPI_CHG0_CH5_Excluded
    #define PPI_CHG0_CH5_Excluded   PPI_CHG_CH5_Excluded
#endif
#ifndef PPI_CHG0_CH5_Included
    #define PPI_CHG0_CH5_Included   PPI_CHG_CH5_Included
#endif

#ifndef PPI_CHG0_CH4_Pos
    #define PPI_CHG0_CH4_Pos        PPI_CHG_CH4_Pos
#endif
#ifndef PPI_CHG0_CH4_Msk
    #define PPI_CHG0_CH4_Msk        PPI_CHG_CH4_Msk
#endif
#ifndef PPI_CHG0_CH4_Excluded
    #define PPI_CHG0_CH4_Excluded   PPI_CHG_CH4_Excluded
#endif
#ifndef PPI_CHG0_CH4_Included
    #define PPI_CHG0_CH4_Included   PPI_CHG_CH4_Included
#endif

#ifndef PPI_CHG0_CH3_Pos
    #define PPI_CHG0_CH3_Pos        PPI_CHG_CH3_Pos
#endif
#ifndef PPI_CHG0_CH3_Msk
    #define PPI_CHG0_CH3_Msk        PPI_CHG_CH3_Msk
#endif
#ifndef PPI_CHG0_CH3_Excluded
    #define PPI_CHG0_CH3_Excluded   PPI_CHG_CH3_Excluded
#endif
#ifndef PPI_CHG0_CH3_Included
    #define PPI_CHG0_CH3_Included   PPI_CHG_CH3_Included
#endif

#ifndef PPI_CHG0_CH2_Pos
    #define PPI_CHG0_CH2_Pos        PPI_CHG_CH2_Pos
#endif
#ifndef PPI_CHG0_CH2_Msk
    #define PPI_CHG0_CH2_Msk        PPI_CHG_CH2_Msk
#endif
#ifndef PPI_CHG0_CH2_Excluded
    #define PPI_CHG0_CH2_Excluded   PPI_CHG_CH2_Excluded
#endif
#ifndef PPI_CHG0_CH2_Included
    #define PPI_CHG0_CH2_Included   PPI_CHG_CH2_Included
#endif

#ifndef PPI_CHG0_CH1_Pos
    #define PPI_CHG0_CH1_Pos        PPI_CHG_CH1_Pos
#endif
#ifndef PPI_CHG0_CH1_Msk
    #define PPI_CHG0_CH1_Msk        PPI_CHG_CH1_Msk
#endif
#ifndef PPI_CHG0_CH1_Excluded
    #define PPI_CHG0_CH1_Excluded   PPI_CHG_CH1_Excluded
#endif
#ifndef PPI_CHG0_CH1_Included
    #define PPI_CHG0_CH1_Included   PPI_CHG_CH1_Included
#endif

#ifndef PPI_CHG0_CH0_Pos
    #define PPI_CHG0_CH0_Pos        PPI_CHG_CH0_Pos
#endif
#ifndef PPI_CHG0_CH0_Msk
    #define PPI_CHG0_CH0_Msk        PPI_CHG_CH0_Msk
#endif
#ifndef PPI_CHG0_CH0_Excluded
    #define PPI_CHG0_CH0_Excluded   PPI_CHG_CH0_Excluded
#endif
#ifndef PPI_CHG0_CH0_Included
    #define PPI_CHG0_CH0_Included   PPI_CHG_CH0_Included
#endif

#ifndef PPI_CHG1_CH15_Pos
    #define PPI_CHG1_CH15_Pos       PPI_CHG_CH15_Pos
#endif
#ifndef PPI_CHG1_CH15_Msk
    #define PPI_CHG1_CH15_Msk       PPI_CHG_CH15_Msk
#endif
#ifndef PPI_CHG1_CH15_Excluded
    #define PPI_CHG1_CH15_Excluded  PPI_CHG_CH15_Excluded
#endif
#ifndef PPI_CHG1_CH15_Included
    #define PPI_CHG1_CH15_Included  PPI_CHG_CH15_Included
#endif

#ifndef PPI_CHG1_CH14_Pos
    #define PPI_CHG1_CH14_Pos       PPI_CHG_CH14_Pos
#endif
#ifndef PPI_CHG1_CH14_Msk
    #define PPI_CHG1_CH14_Msk       PPI_CHG_CH14_Msk
#endif
#ifndef PPI_CHG1_CH14_Excluded
    #define PPI_CHG1_CH14_Excluded  PPI_CHG_CH14_Excluded
#endif
#ifndef PPI_CHG1_CH14_Included
    #define PPI_CHG1_CH14_Included  PPI_CHG_CH14_Included
#endif

#ifndef PPI_CHG1_CH13_Pos
    #define PPI_CHG1_CH13_Pos       PPI_CHG_CH13_Pos
#endif
#ifndef PPI_CHG1_CH13_Msk
    #define PPI_CHG1_CH13_Msk       PPI_CHG_CH13_Msk
#endif
#ifndef PPI_CHG1_CH13_Excluded
    #define PPI_CHG1_CH13_Excluded  PPI_CHG_CH13_Excluded
#endif
#ifndef PPI_CHG1_CH13_Included
    #define PPI_CHG1_CH13_Included  PPI_CHG_CH13_Included
#endif

#ifndef PPI_CHG1_CH12_Pos
    #define PPI_CHG1_CH12_Pos       PPI_CHG_CH12_Pos
#endif
#ifndef PPI_CHG1_CH12_Msk
    #define PPI_CHG1_CH12_Msk       PPI_CHG_CH12_Msk
#endif
#ifndef PPI_CHG1_CH12_Excluded
    #define PPI_CHG1_CH12_Excluded  PPI_CHG_CH12_Excluded
#endif
#ifndef PPI_CHG1_CH12_Included
    #define PPI_CHG1_CH12_Included  PPI_CHG_CH12_Included
#endif

#ifndef PPI_CHG1_CH11_Pos
    #define PPI_CHG1_CH11_Pos       PPI_CHG_CH11_Pos
#endif
#ifndef PPI_CHG1_CH11_Msk
    #define PPI_CHG1_CH11_Msk       PPI_CHG_CH11_Msk
#endif
#ifndef PPI_CHG1_CH11_Excluded
    #define PPI_CHG1_CH11_Excluded  PPI_CHG_CH11_Excluded
#endif
#ifndef PPI_CHG1_CH11_Included
    #define PPI_CHG1_CH11_Included  PPI_CHG_CH11_Included
#endif

#ifndef PPI_CHG1_CH10_Pos
    #define PPI_CHG1_CH10_Pos       PPI_CHG_CH10_Pos
#endif
#ifndef PPI_CHG1_CH10_Msk
    #define PPI_CHG1_CH10_Msk       PPI_CHG_CH10_Msk
#endif
#ifndef PPI_CHG1_CH10_Excluded
    #define PPI_CHG1_CH10_Excluded  PPI_CHG_CH10_Excluded
#endif
#ifndef PPI_CHG1_CH10_Included
    #define PPI_CHG1_CH10_Included  PPI_CHG_CH10_Included
#endif

#ifndef PPI_CHG1_CH9_Pos
    #define PPI_CHG1_CH9_Pos        PPI_CHG_CH9_Pos
#endif
#ifndef PPI_CHG1_CH9_Msk
    #define PPI_CHG1_CH9_Msk        PPI_CHG_CH9_Msk
#endif
#ifndef PPI_CHG1_CH9_Excluded
    #define PPI_CHG1_CH9_Excluded   PPI_CHG_CH9_Excluded
#endif
#ifndef PPI_CHG1_CH9_Included
    #define PPI_CHG1_CH9_Included   PPI_CHG_CH9_Included
#endif

#ifndef PPI_CHG1_CH8_Pos
    #define PPI_CHG1_CH8_Pos        PPI_CHG_CH8_Pos
#endif
#ifndef PPI_CHG1_CH8_Msk 
    #define PPI_CHG1_CH8_Msk        PPI_CHG_CH8_Msk
#endif
#ifndef PPI_CHG1_CH8_Excluded
    #define PPI_CHG1_CH8_Excluded   PPI_CHG_CH8_Excluded
#endif
#ifndef PPI_CHG1_CH8_Included 
    #define PPI_CHG1_CH8_Included   PPI_CHG_CH8_Included
#endif

#ifndef PPI_CHG1_CH7_Pos
    #define PPI_CHG1_CH7_Pos        PPI_CHG_CH7_Pos
#endif
#ifndef PPI_CHG1_CH7_Msk
    #define PPI_CHG1_CH7_Msk        PPI_CHG_CH7_Msk
#endif
#ifndef PPI_CHG1_CH7_Excluded
    #define PPI_CHG1_CH7_Excluded   PPI_CHG_CH7_Excluded
#endif
#ifndef PPI_CHG1_CH7_Included
    #define PPI_CHG1_CH7_Included   PPI_CHG_CH7_Included
#endif

#ifndef PPI_CHG1_CH6_Pos
    #define PPI_CHG1_CH6_Pos        PPI_CHG_CH6_Pos
#endif
#ifndef PPI_CHG1_CH6_Msk
    #define PPI_CHG1_CH6_Msk        PPI_CHG_CH6_Msk
#endif
#ifndef PPI_CHG1_CH6_Excluded
    #define PPI_CHG1_CH6_Excluded   PPI_CHG_CH6_Excluded
#endif
#ifndef PPI_CHG1_CH6_Included
    #define PPI_CHG1_CH6_Included   PPI_CHG_CH6_Included
#endif

#ifndef PPI_CHG1_CH5_Pos
    #define PPI_CHG1_CH5_Pos        PPI_CHG_CH5_Pos
#endif
#ifndef PPI_CHG1_CH5_Msk
    #define PPI_CHG1_CH5_Msk        PPI_CHG_CH5_Msk
#endif
#ifndef PPI_CHG1_CH5_Excluded
    #define PPI_CHG1_CH5_Excluded   PPI_CHG_CH5_Excluded
#endif
#ifndef PPI_CHG1_CH5_Included
    #define PPI_CHG1_CH5_Included   PPI_CHG_CH5_Included
#endif

#ifndef PPI_CHG1_CH4_Pos
    #define PPI_CHG1_CH4_Pos        PPI_CHG_CH4_Pos
#endif
#ifndef PPI_CHG1_CH4_Msk
    #define PPI_CHG1_CH4_Msk        PPI_CHG_CH4_Msk
#endif
#ifndef PPI_CHG1_CH4_Excluded
    #define PPI_CHG1_CH4_Excluded   PPI_CHG_CH4_Excluded
#endif
#ifndef PPI_CHG1_CH4_Included
    #define PPI_CHG1_CH4_Included   PPI_CHG_CH4_Included
#endif

#ifndef PPI_CHG1_CH3_Pos
    #define PPI_CHG1_CH3_Pos        PPI_CHG_CH3_Pos
#endif
#ifndef PPI_CHG1_CH3_Msk
    #define PPI_CHG1_CH3_Msk        PPI_CHG_CH3_Msk
#endif
#ifndef PPI_CHG1_CH3_Excluded
    #define PPI_CHG1_CH3_Excluded   PPI_CHG_CH3_Excluded
#endif
#ifndef PPI_CHG1_CH3_Included
    #define PPI_CHG1_CH3_Included   PPI_CHG_CH3_Included
#endif

#ifndef PPI_CHG1_CH2_Pos
    #define PPI_CHG1_CH2_Pos        PPI_CHG_CH2_Pos
#endif
#ifndef PPI_CHG1_CH2_Msk
    #define PPI_CHG1_CH2_Msk        PPI_CHG_CH2_Msk
#endif
#ifndef PPI_CHG1_CH2_Excluded
    #define PPI_CHG1_CH2_Excluded   PPI_CHG_CH2_Excluded
#endif
#ifndef PPI_CHG1_CH2_Included
    #define PPI_CHG1_CH2_Included   PPI_CHG_CH2_Included
#endif

#ifndef PPI_CHG1_CH1_Pos
    #define PPI_CHG1_CH1_Pos        PPI_CHG_CH1_Pos
#endif
#ifndef PPI_CHG1_CH1_Msk
    #define PPI_CHG1_CH1_Msk        PPI_CHG_CH1_Msk
#endif
#ifndef PPI_CHG1_CH1_Excluded
    #define PPI_CHG1_CH1_Excluded   PPI_CHG_CH1_Excluded
#endif
#ifndef PPI_CHG1_CH1_Included
    #define PPI_CHG1_CH1_Included   PPI_CHG_CH1_Included
#endif

#ifndef PPI_CHG1_CH0_Pos
    #define PPI_CHG1_CH0_Pos        PPI_CHG_CH0_Pos
#endif
#ifndef PPI_CHG1_CH0_Msk
    #define PPI_CHG1_CH0_Msk        PPI_CHG_CH0_Msk
#endif
#ifndef PPI_CHG1_CH0_Excluded
    #define PPI_CHG1_CH0_Excluded   PPI_CHG_CH0_Excluded
#endif
#ifndef PPI_CHG1_CH0_Included
    #define PPI_CHG1_CH0_Included   PPI_CHG_CH0_Included
#endif

#ifndef PPI_CHG2_CH15_Pos
    #define PPI_CHG2_CH15_Pos       PPI_CHG_CH15_Pos
#endif
#ifndef PPI_CHG2_CH15_Msk
    #define PPI_CHG2_CH15_Msk       PPI_CHG_CH15_Msk
#endif
#ifndef PPI_CHG2_CH15_Excluded
    #define PPI_CHG2_CH15_Excluded  PPI_CHG_CH15_Excluded
#endif
#ifndef PPI_CHG2_CH15_Included
    #define PPI_CHG2_CH15_Included  PPI_CHG_CH15_Included
#endif

#ifndef PPI_CHG2_CH14_Pos
    #define PPI_CHG2_CH14_Pos       PPI_CHG_CH14_Pos
#endif
#ifndef PPI_CHG2_CH14_Msk
    #define PPI_CHG2_CH14_Msk       PPI_CHG_CH14_Msk
#endif
#ifndef PPI_CHG2_CH14_Excluded
    #define PPI_CHG2_CH14_Excluded  PPI_CHG_CH14_Excluded
#endif
#ifndef PPI_CHG2_CH14_Included
    #define PPI_CHG2_CH14_Included  PPI_CHG_CH14_Included
#endif

#ifndef PPI_CHG2_CH13_Pos
    #define PPI_CHG2_CH13_Pos       PPI_CHG_CH13_Pos
#endif
#ifndef PPI_CHG2_CH13_Msk
    #define PPI_CHG2_CH13_Msk       PPI_CHG_CH13_Msk
#endif
#ifndef PPI_CHG2_CH13_Excluded
    #define PPI_CHG2_CH13_Excluded  PPI_CHG_CH13_Excluded
#endif
#ifndef PPI_CHG2_CH13_Included
    #define PPI_CHG2_CH13_Included  PPI_CHG_CH13_Included
#endif

#ifndef PPI_CHG2_CH12_Pos
    #define PPI_CHG2_CH12_Pos       PPI_CHG_CH12_Pos
#endif
#ifndef PPI_CHG2_CH12_Msk
    #define PPI_CHG2_CH12_Msk       PPI_CHG_CH12_Msk
#endif
#ifndef PPI_CHG2_CH12_Excluded
    #define PPI_CHG2_CH12_Excluded  PPI_CHG_CH12_Excluded
#endif
#ifndef PPI_CHG2_CH12_Included
    #define PPI_CHG2_CH12_Included  PPI_CHG_CH12_Included
#endif

#ifndef PPI_CHG2_CH11_Pos
    #define PPI_CHG2_CH11_Pos       PPI_CHG_CH11_Pos
#endif
#ifndef PPI_CHG2_CH11_Msk
    #define PPI_CHG2_CH11_Msk       PPI_CHG_CH11_Msk
#endif
#ifndef PPI_CHG2_CH11_Excluded
    #define PPI_CHG2_CH11_Excluded  PPI_CHG_CH11_Excluded
#endif
#ifndef PPI_CHG2_CH11_Included
    #define PPI_CHG2_CH11_Included  PPI_CHG_CH11_Included
#endif

#ifndef PPI_CHG2_CH10_Pos
    #define PPI_CHG2_CH10_Pos       PPI_CHG_CH10_Pos
#endif
#ifndef PPI_CHG2_CH10_Msk
    #define PPI_CHG2_CH10_Msk       PPI_CHG_CH10_Msk
#endif
#ifndef PPI_CHG2_CH10_Excluded
    #define PPI_CHG2_CH10_Excluded  PPI_CHG_CH10_Excluded
#endif
#ifndef PPI_CHG2_CH10_Included
    #define PPI_CHG2_CH10_Included  PPI_CHG_CH10_Included
#endif

#ifndef PPI_CHG2_CH9_Pos
    #define PPI_CHG2_CH9_Pos        PPI_CHG_CH9_Pos
#endif
#ifndef PPI_CHG2_CH9_Msk
    #define PPI_CHG2_CH9_Msk        PPI_CHG_CH9_Msk
#endif
#ifndef PPI_CHG2_CH9_Excluded
    #define PPI_CHG2_CH9_Excluded   PPI_CHG_CH9_Excluded
#endif
#ifndef PPI_CHG2_CH9_Included
    #define PPI_CHG2_CH9_Included   PPI_CHG_CH9_Included
#endif

#ifndef PPI_CHG2_CH8_Pos
    #define PPI_CHG2_CH8_Pos        PPI_CHG_CH8_Pos
#endif
#ifndef PPI_CHG2_CH8_Msk
    #define PPI_CHG2_CH8_Msk        PPI_CHG_CH8_Msk
#endif
#ifndef PPI_CHG2_CH8_Excluded
    #define PPI_CHG2_CH8_Excluded   PPI_CHG_CH8_Excluded
#endif
#ifndef PPI_CHG2_CH8_Included
    #define PPI_CHG2_CH8_Included   PPI_CHG_CH8_Included
#endif

#ifndef PPI_CHG2_CH7_Pos
    #define PPI_CHG2_CH7_Pos        PPI_CHG_CH7_Pos
#endif
#ifndef PPI_CHG2_CH7_Msk
    #define PPI_CHG2_CH7_Msk        PPI_CHG_CH7_Msk
#endif
#ifndef PPI_CHG2_CH7_Excluded
    #define PPI_CHG2_CH7_Excluded   PPI_CHG_CH7_Excluded
#endif
#ifndef PPI_CHG2_CH7_Included
    #define PPI_CHG2_CH7_Included   PPI_CHG_CH7_Included
#endif

#ifndef PPI_CHG2_CH6_Pos
    #define PPI_CHG2_CH6_Pos        PPI_CHG_CH6_Pos
#endif
#ifndef PPI_CHG2_CH6_Msk
    #define PPI_CHG2_CH6_Msk        PPI_CHG_CH6_Msk
#endif
#ifndef PPI_CHG2_CH6_Excluded
    #define PPI_CHG2_CH6_Excluded   PPI_CHG_CH6_Excluded
#endif
#ifndef PPI_CHG2_CH6_Included
    #define PPI_CHG2_CH6_Included   PPI_CHG_CH6_Included
#endif

#ifndef PPI_CHG2_CH5_Pos
    #define PPI_CHG2_CH5_Pos        PPI_CHG_CH5_Pos
#endif
#ifndef PPI_CHG2_CH5_Msk
    #define PPI_CHG2_CH5_Msk        PPI_CHG_CH5_Msk
#endif
#ifndef PPI_CHG2_CH5_Excluded
    #define PPI_CHG2_CH5_Excluded   PPI_CHG_CH5_Excluded
#endif
#ifndef PPI_CHG2_CH5_Included
    #define PPI_CHG2_CH5_Included   PPI_CHG_CH5_Included
#endif

#ifndef PPI_CHG2_CH4_Pos
    #define PPI_CHG2_CH4_Pos        PPI_CHG_CH4_Pos
#endif
#ifndef PPI_CHG2_CH4_Msk
    #define PPI_CHG2_CH4_Msk        PPI_CHG_CH4_Msk
#endif
#ifndef PPI_CHG2_CH4_Excluded
    #define PPI_CHG2_CH4_Excluded   PPI_CHG_CH4_Excluded
#endif
#ifndef PPI_CHG2_CH4_Included
    #define PPI_CHG2_CH4_Included   PPI_CHG_CH4_Included
#endif

#ifndef PPI_CHG2_CH3_Pos
    #define PPI_CHG2_CH3_Pos        PPI_CHG_CH3_Pos
#endif
#ifndef PPI_CHG2_CH3_Msk
    #define PPI_CHG2_CH3_Msk        PPI_CHG_CH3_Msk
#endif
#ifndef PPI_CHG2_CH3_Excluded
    #define PPI_CHG2_CH3_Excluded   PPI_CHG_CH3_Excluded
#endif
#ifndef PPI_CHG2_CH3_Included
    #define PPI_CHG2_CH3_Included   PPI_CHG_CH3_Included
#endif

#ifndef PPI_CHG2_CH2_Pos
    #define PPI_CHG2_CH2_Pos        PPI_CHG_CH2_Pos
#endif
#ifndef PPI_CHG2_CH2_Msk
    #define PPI_CHG2_CH2_Msk        PPI_CHG_CH2_Msk
#endif
#ifndef PPI_CHG2_CH2_Excluded
    #define PPI_CHG2_CH2_Excluded   PPI_CHG_CH2_Excluded
#endif 
#ifndef PPI_CHG2_CH2_Included
    #define PPI_CHG2_CH2_Included   PPI_CHG_CH2_Included
#endif

#ifndef PPI_CHG2_CH1_Pos
    #define PPI_CHG2_CH1_Pos        PPI_CHG_CH1_Pos
#endif
#ifndef PPI_CHG2_CH1_Msk
    #define PPI_CHG2_CH1_Msk        PPI_CHG_CH1_Msk
#endif
#ifndef PPI_CHG2_CH1_Excluded
    #define PPI_CHG2_CH1_Excluded   PPI_CHG_CH1_Excluded
#endif
#ifndef PPI_CHG2_CH1_Included
    #define PPI_CHG2_CH1_Included   PPI_CHG_CH1_Included
#endif

#ifndef PPI_CHG2_CH0_Pos
    #define PPI_CHG2_CH0_Pos        PPI_CHG_CH0_Pos
#endif
#ifndef PPI_CHG2_CH0_Msk
    #define PPI_CHG2_CH0_Msk        PPI_CHG_CH0_Msk
#endif
#ifndef PPI_CHG2_CH0_Excluded
    #define PPI_CHG2_CH0_Excluded   PPI_CHG_CH0_Excluded
#endif
#ifndef PPI_CHG2_CH0_Included
    #define PPI_CHG2_CH0_Included   PPI_CHG_CH0_Included
#endif

#ifndef PPI_CHG3_CH15_Pos
    #define PPI_CHG3_CH15_Pos       PPI_CHG_CH15_Pos
#endif
#ifndef PPI_CHG3_CH15_Msk
    #define PPI_CHG3_CH15_Msk       PPI_CHG_CH15_Msk
#endif
#ifndef PPI_CHG3_CH15_Excluded
    #define PPI_CHG3_CH15_Excluded  PPI_CHG_CH15_Excluded
#endif
#ifndef PPI_CHG3_CH15_Included
    #define PPI_CHG3_CH15_Included  PPI_CHG_CH15_Included
#endif

#ifndef PPI_CHG3_CH14_Pos
    #define PPI_CHG3_CH14_Pos       PPI_CHG_CH14_Pos
#endif
#ifndef PPI_CHG3_CH14_Msk
    #define PPI_CHG3_CH14_Msk       PPI_CHG_CH14_Msk
#endif
#ifndef PPI_CHG3_CH14_Excluded
    #define PPI_CHG3_CH14_Excluded  PPI_CHG_CH14_Excluded
#endif
#ifndef PPI_CHG3_CH14_Included
    #define PPI_CHG3_CH14_Included  PPI_CHG_CH14_Included
#endif

#ifndef PPI_CHG3_CH13_Pos
    #define PPI_CHG3_CH13_Pos       PPI_CHG_CH13_Pos
#endif
#ifndef PPI_CHG3_CH13_Msk
    #define PPI_CHG3_CH13_Msk       PPI_CHG_CH13_Msk
#endif
#ifndef PPI_CHG3_CH13_Excluded
    #define PPI_CHG3_CH13_Excluded  PPI_CHG_CH13_Excluded
#endif
#ifndef PPI_CHG3_CH13_Included
    #define PPI_CHG3_CH13_Included  PPI_CHG_CH13_Included
#endif

#ifndef PPI_CHG3_CH12_Pos
    #define PPI_CHG3_CH12_Pos       PPI_CHG_CH12_Pos
#endif
#ifndef PPI_CHG3_CH12_Msk
    #define PPI_CHG3_CH12_Msk       PPI_CHG_CH12_Msk
#endif
#ifndef PPI_CHG3_CH12_Excluded
    #define PPI_CHG3_CH12_Excluded  PPI_CHG_CH12_Excluded
#endif
#ifndef PPI_CHG3_CH12_Included
    #define PPI_CHG3_CH12_Included  PPI_CHG_CH12_Included
#endif

#ifndef PPI_CHG3_CH11_Pos
    #define PPI_CHG3_CH11_Pos       PPI_CHG_CH11_Pos
#endif
#ifndef PPI_CHG3_CH11_Msk
    #define PPI_CHG3_CH11_Msk       PPI_CHG_CH11_Msk
#endif
#ifndef PPI_CHG3_CH11_Excluded
    #define PPI_CHG3_CH11_Excluded  PPI_CHG_CH11_Excluded
#endif
#ifndef PPI_CHG3_CH11_Included
    #define PPI_CHG3_CH11_Included  PPI_CHG_CH11_Included
#endif

#ifndef PPI_CHG3_CH10_Pos
    #define PPI_CHG3_CH10_Pos       PPI_CHG_CH10_Pos
#endif
#ifndef PPI_CHG3_CH10_Msk
    #define PPI_CHG3_CH10_Msk       PPI_CHG_CH10_Msk
#endif
#ifndef PPI_CHG3_CH10_Excluded
    #define PPI_CHG3_CH10_Excluded  PPI_CHG_CH10_Excluded
#endif
#ifndef PPI_CHG3_CH10_Included
    #define PPI_CHG3_CH10_Included  PPI_CHG_CH10_Included
#endif

#ifndef PPI_CHG3_CH9_Pos
    #define PPI_CHG3_CH9_Pos        PPI_CHG_CH9_Pos
#endif
#ifndef PPI_CHG3_CH9_Msk
    #define PPI_CHG3_CH9_Msk        PPI_CHG_CH9_Msk
#endif
#ifndef PPI_CHG3_CH9_Excluded
    #define PPI_CHG3_CH9_Excluded   PPI_CHG_CH9_Excluded
#endif
#ifndef PPI_CHG3_CH9_Included
    #define PPI_CHG3_CH9_Included   PPI_CHG_CH9_Included
#endif

#ifndef PPI_CHG3_CH8_Pos
    #define PPI_CHG3_CH8_Pos        PPI_CHG_CH8_Pos
#endif
#ifndef PPI_CHG3_CH8_Msk
    #define PPI_CHG3_CH8_Msk        PPI_CHG_CH8_Msk
#endif
#ifndef PPI_CHG3_CH8_Excluded
    #define PPI_CHG3_CH8_Excluded   PPI_CHG_CH8_Excluded
#endif
#ifndef PPI_CHG3_CH8_Included
    #define PPI_CHG3_CH8_Included   PPI_CHG_CH8_Included
#endif

#ifndef PPI_CHG3_CH7_Pos
    #define PPI_CHG3_CH7_Pos        PPI_CHG_CH7_Pos
#endif
#ifndef PPI_CHG3_CH7_Msk
    #define PPI_CHG3_CH7_Msk        PPI_CHG_CH7_Msk
#endif
#ifndef PPI_CHG3_CH7_Excluded
    #define PPI_CHG3_CH7_Excluded   PPI_CHG_CH7_Excluded
#endif
#ifndef PPI_CHG3_CH7_Included
    #define PPI_CHG3_CH7_Included   PPI_CHG_CH7_Included
#endif

#ifndef PPI_CHG3_CH6_Pos
    #define PPI_CHG3_CH6_Pos        PPI_CHG_CH6_Pos
#endif
#ifndef PPI_CHG3_CH6_Msk
    #define PPI_CHG3_CH6_Msk        PPI_CHG_CH6_Msk
#endif
#ifndef PPI_CHG3_CH6_Excluded
    #define PPI_CHG3_CH6_Excluded   PPI_CHG_CH6_Excluded
#endif
#ifndef PPI_CHG3_CH6_Included
    #define PPI_CHG3_CH6_Included   PPI_CHG_CH6_Included
#endif

#ifndef PPI_CHG3_CH5_Pos
    #define PPI_CHG3_CH5_Pos        PPI_CHG_CH5_Pos
#endif
#ifndef PPI_CHG3_CH5_Msk
    #define PPI_CHG3_CH5_Msk        PPI_CHG_CH5_Msk
#endif
#ifndef PPI_CHG3_CH5_Excluded
    #define PPI_CHG3_CH5_Excluded   PPI_CHG_CH5_Excluded
#endif
#ifndef PPI_CHG3_CH5_Included
    #define PPI_CHG3_CH5_Included   PPI_CHG_CH5_Included
#endif

#ifndef PPI_CHG3_CH4_Pos
    #define PPI_CHG3_CH4_Pos        PPI_CHG_CH4_Pos
#endif
#ifndef PPI_CHG3_CH4_Msk
    #define PPI_CHG3_CH4_Msk        PPI_CHG_CH4_Msk
#endif
#ifndef PPI_CHG3_CH4_Excluded
    #define PPI_CHG3_CH4_Excluded   PPI_CHG_CH4_Excluded
#endif
#ifndef PPI_CHG3_CH4_Included
    #define PPI_CHG3_CH4_Included   PPI_CHG_CH4_Included
#endif

#ifndef PPI_CHG3_CH3_Pos
    #define PPI_CHG3_CH3_Pos        PPI_CHG_CH3_Pos
#endif
#ifndef PPI_CHG3_CH3_Msk
    #define PPI_CHG3_CH3_Msk        PPI_CHG_CH3_Msk
#endif
#ifndef PPI_CHG3_CH3_Excluded
    #define PPI_CHG3_CH3_Excluded   PPI_CHG_CH3_Excluded
#endif
#ifndef PPI_CHG3_CH3_Included
    #define PPI_CHG3_CH3_Included   PPI_CHG_CH3_Included
#endif

#ifndef PPI_CHG3_CH2_Pos
    #define PPI_CHG3_CH2_Pos        PPI_CHG_CH2_Pos
#endif
#ifndef PPI_CHG3_CH2_Msk
    #define PPI_CHG3_CH2_Msk        PPI_CHG_CH2_Msk
#endif
#ifndef PPI_CHG3_CH2_Excluded
    #define PPI_CHG3_CH2_Excluded   PPI_CHG_CH2_Excluded
#endif
#ifndef PPI_CHG3_CH2_Included
    #define PPI_CHG3_CH2_Included   PPI_CHG_CH2_Included
#endif

#ifndef PPI_CHG3_CH1_Pos
    #define PPI_CHG3_CH1_Pos        PPI_CHG_CH1_Pos
#endif
#ifndef PPI_CHG3_CH1_Msk
    #define PPI_CHG3_CH1_Msk        PPI_CHG_CH1_Msk
#endif
#ifndef PPI_CHG3_CH1_Excluded
    #define PPI_CHG3_CH1_Excluded   PPI_CHG_CH1_Excluded
#endif
#ifndef PPI_CHG3_CH1_Included
    #define PPI_CHG3_CH1_Included   PPI_CHG_CH1_Included
#endif

#ifndef PPI_CHG3_CH0_Pos
    #define PPI_CHG3_CH0_Pos        PPI_CHG_CH0_Pos
#endif
#ifndef PPI_CHG3_CH0_Msk
    #define PPI_CHG3_CH0_Msk        PPI_CHG_CH0_Msk
#endif
#ifndef PPI_CHG3_CH0_Excluded
    #define PPI_CHG3_CH0_Excluded   PPI_CHG_CH0_Excluded
#endif
#ifndef PPI_CHG3_CH0_Included
    #define PPI_CHG3_CH0_Included   PPI_CHG_CH0_Included
#endif




/*lint --flb "Leave library region" */

#endif /* NRF51_TO_NRF52810_H */

