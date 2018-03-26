/*

Copyright (c) 2009-2018 ARM Limited. All rights reserved.

    SPDX-License-Identifier: Apache-2.0

Licensed under the Apache License, Version 2.0 (the License); you may
not use this file except in compliance with the License.
You may obtain a copy of the License at

    www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an AS IS BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

NOTICE: This file has been modified by Nordic Semiconductor ASA.

*/

/* NOTE: Template files (including this one) are application specific and therefore expected to
   be copied into the application project folder prior to its use! */

#include <stdint.h>
#include <stdbool.h>
#include "nrf.h"
#include "system_nrf52810.h"

/*lint ++flb "Enter library region" */

#define __SYSTEM_CLOCK_64M      (64000000UL)

static bool errata_31(void);
static bool errata_36(void);
static bool errata_66(void);
static bool errata_103(void);
static bool errata_136(void);

/* Helper functions for Errata workarounds in nRF52832 */
#if defined (DEVELOP_IN_NRF52832)
static bool errata_12(void);
static bool errata_16(void);
static bool errata_32(void);
static bool errata_37(void);
static bool errata_57(void);
static bool errata_108(void);
static bool errata_182(void);
#endif

#if defined ( __CC_ARM )
    uint32_t SystemCoreClock __attribute__((used)) = __SYSTEM_CLOCK_64M;
#elif defined ( __ICCARM__ )
    __root uint32_t SystemCoreClock = __SYSTEM_CLOCK_64M;
#elif defined ( __GNUC__ )
    uint32_t SystemCoreClock __attribute__((used)) = __SYSTEM_CLOCK_64M;
#endif

void SystemCoreClockUpdate(void)
{
    SystemCoreClock = __SYSTEM_CLOCK_64M;
}

void SystemInit(void)
{
    /* Enable SWO trace functionality. If ENABLE_SWO is not defined, SWO pin will be used as GPIO (see Product
       Specification to see which one). Only available if the developing environment is an nRF52832 device. */
    #if defined (DEVELOP_IN_NRF52832) && defined (ENABLE_SWO)
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        NRF_CLOCK->TRACECONFIG |= CLOCK_TRACECONFIG_TRACEMUX_Serial << CLOCK_TRACECONFIG_TRACEMUX_Pos;
        NRF_P0->PIN_CNF[18] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
    #endif
    
    /* Enable Trace functionality. If ENABLE_TRACE is not defined, TRACE pins will be used as GPIOs (see Product
       Specification to see which ones). Only available if the developing environment is an nRF52832 device. */
    #if defined (DEVELOP_IN_NRF52832) && defined (ENABLE_TRACE)
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        NRF_CLOCK->TRACECONFIG |= CLOCK_TRACECONFIG_TRACEMUX_Parallel << CLOCK_TRACECONFIG_TRACEMUX_Pos;
        NRF_P0->PIN_CNF[14] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
        NRF_P0->PIN_CNF[15] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
        NRF_P0->PIN_CNF[16] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
        NRF_P0->PIN_CNF[18] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
        NRF_P0->PIN_CNF[20] = (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos) | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) | (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
    #endif
    
    #if defined (DEVELOP_IN_NRF52832)
    /* Workaround for Errata 12 "COMP: Reference ladder not correctly calibrated" found at the Errata document
       for nRF52832 device located at https://infocenter.nordicsemi.com/ */
    if (errata_12()){
        *(volatile uint32_t *)0x40013540 = (*(uint32_t *)0x10000324 & 0x00001F00) >> 8;
    }
    #endif
    
    #if defined (DEVELOP_IN_NRF52832)
    /* Workaround for Errata 16 "System: RAM may be corrupt on wakeup from CPU IDLE" found at the Errata document
       for nRF52832 device located at https://infocenter.nordicsemi.com/ */
    if (errata_16()){
        *(volatile uint32_t *)0x4007C074 = 3131961357ul;
    }
    #endif
    
    /* Workaround for Errata 31 "CLOCK: Calibration values are not correctly loaded from FICR at reset" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/ */
    if (errata_31()){
        *(volatile uint32_t *)0x4000053C = ((*(volatile uint32_t *)0x10000244) & 0x0000E000) >> 13;
    }

    #if defined (DEVELOP_IN_NRF52832)
    /* Workaround for Errata 32 "DIF: Debug session automatically enables TracePort pins" found at the Errata document
       for nRF52832 device located at https://infocenter.nordicsemi.com/ */
    if (errata_32()){
        CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
    }
    #endif
    
    /* Workaround for Errata 36 "CLOCK: Some registers are not reset when expected" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/  */
    if (errata_36()){
        NRF_CLOCK->EVENTS_DONE = 0;
        NRF_CLOCK->EVENTS_CTTO = 0;
        NRF_CLOCK->CTIV = 0;
    }
    
    #if defined (DEVELOP_IN_NRF52832)
    /* Workaround for Errata 37 "RADIO: Encryption engine is slow by default" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/  */
    if (errata_37()){
        *(volatile uint32_t *)0x400005A0 = 0x3;
    }
    #endif

    #if defined (DEVELOP_IN_NRF52832)
    /* Workaround for Errata 57 "NFCT: NFC Modulation amplitude" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/  */
    if (errata_57()){
        *(volatile uint32_t *)0x40005610 = 0x00000005;
        *(volatile uint32_t *)0x40005688 = 0x00000001;
        *(volatile uint32_t *)0x40005618 = 0x00000000;
        *(volatile uint32_t *)0x40005614 = 0x0000003F;
    }
    #endif
    
    /* Workaround for Errata 66 "TEMP: Linearity specification not met with default settings" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/  */
    if (errata_66()){
        NRF_TEMP->A0 = NRF_FICR->TEMP.A0;
        NRF_TEMP->A1 = NRF_FICR->TEMP.A1;
        NRF_TEMP->A2 = NRF_FICR->TEMP.A2;
        NRF_TEMP->A3 = NRF_FICR->TEMP.A3;
        NRF_TEMP->A4 = NRF_FICR->TEMP.A4;
        NRF_TEMP->A5 = NRF_FICR->TEMP.A5;
        NRF_TEMP->B0 = NRF_FICR->TEMP.B0;
        NRF_TEMP->B1 = NRF_FICR->TEMP.B1;
        NRF_TEMP->B2 = NRF_FICR->TEMP.B2;
        NRF_TEMP->B3 = NRF_FICR->TEMP.B3;
        NRF_TEMP->B4 = NRF_FICR->TEMP.B4;
        NRF_TEMP->B5 = NRF_FICR->TEMP.B5;
        NRF_TEMP->T0 = NRF_FICR->TEMP.T0;
        NRF_TEMP->T1 = NRF_FICR->TEMP.T1;
        NRF_TEMP->T2 = NRF_FICR->TEMP.T2;
        NRF_TEMP->T3 = NRF_FICR->TEMP.T3;
        NRF_TEMP->T4 = NRF_FICR->TEMP.T4;
    }
    
    /* Workaround for Errata 103 "CCM: Wrong reset value of CCM MAXPACKETSIZE" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/  */
    if (errata_103()){
        NRF_CCM->MAXPACKETSIZE = 0xFBul;
    }
    
    #if defined (DEVELOP_IN_NRF52832)
    /* Workaround for Errata 108 "RAM: RAM content cannot be trusted upon waking up from System ON Idle or System OFF mode" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/  */
    if (errata_108()){
        *(volatile uint32_t *)0x40000EE4 = *(volatile uint32_t *)0x10000258 & 0x0000004F;
    }
    #endif
    
    /* Workaround for Errata 136 "System: Bits in RESETREAS are set when they should not be" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/  */
    if (errata_136()){
        if (NRF_POWER->RESETREAS & POWER_RESETREAS_RESETPIN_Msk){
            NRF_POWER->RESETREAS =  ~POWER_RESETREAS_RESETPIN_Msk;
        }
    }
    
    #if defined (DEVELOP_IN_NRF52832)
    /* Workaround for Errata 182 "RADIO: Fixes for anomalies #102, #106, and #107 do not take effect" found at the Errata document
       for your device located at https://infocenter.nordicsemi.com/  */
    if (errata_182()){
        *(volatile uint32_t *) 0x4000173C |= (0x1 << 10);
    }
    #endif

    /* Configure GPIO pads as pPin Reset pin if Pin Reset capabilities desired. If CONFIG_GPIO_AS_PINRESET is not
      defined, pin reset will not be available. One GPIO (see Product Specification to see which one) will then be
      reserved for PinReset and not available as normal GPIO. */
    #if defined (CONFIG_GPIO_AS_PINRESET)
        if (((NRF_UICR->PSELRESET[0] & UICR_PSELRESET_CONNECT_Msk) != (UICR_PSELRESET_CONNECT_Connected << UICR_PSELRESET_CONNECT_Pos)) ||
            ((NRF_UICR->PSELRESET[1] & UICR_PSELRESET_CONNECT_Msk) != (UICR_PSELRESET_CONNECT_Connected << UICR_PSELRESET_CONNECT_Pos))){
            NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
            while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
            NRF_UICR->PSELRESET[0] = 21;
            while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
            NRF_UICR->PSELRESET[1] = 21;
            while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
            NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
            while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
            NVIC_SystemReset();
        }
    #endif

    SystemCoreClockUpdate();
}

#if defined (DEVELOP_IN_NRF52832)
static bool errata_12(void)
{
    if ((((*(uint32_t *)0xF0000FE0) & 0x000000FF) == 0x6) && (((*(uint32_t *)0xF0000FE4) & 0x0000000F) == 0x0)){
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x30){
            return true;
        }
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x40){
            return true;
        }
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x50){
            return true;
        }
    }

    return false;
}
#endif

#if defined (DEVELOP_IN_NRF52832)
static bool errata_16(void)
{
    if ((((*(uint32_t *)0xF0000FE0) & 0x000000FF) == 0x6) && (((*(uint32_t *)0xF0000FE4) & 0x0000000F) == 0x0)){
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x30){
            return true;
        }
    }

    return false;
}
#endif

static bool errata_31(void)
{
    if ((*(uint32_t *)0x10000130ul == 0xAul) && (*(uint32_t *)0x10000134ul == 0x0ul)){
        return true;
    }
    
    #if defined (DEVELOP_IN_NRF52832)
    if ((((*(uint32_t *)0xF0000FE0) & 0x000000FF) == 0x6) && (((*(uint32_t *)0xF0000FE4) & 0x0000000F) == 0x0)){
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x30){
            return true;
        }
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x40){
            return true;
        }
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x50){
            return true;
        }
    }
    #endif

    return true;
}

#if defined (DEVELOP_IN_NRF52832)
static bool errata_32(void)
{
    if ((((*(uint32_t *)0xF0000FE0) & 0x000000FF) == 0x6) && (((*(uint32_t *)0xF0000FE4) & 0x0000000F) == 0x0)){
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x30){
            return true;
        }
    }

    return false;
}
#endif

static bool errata_36(void)
{
    if ((*(uint32_t *)0x10000130ul == 0xAul) && (*(uint32_t *)0x10000134ul == 0x0ul)){
        return true;
    }
    
    #if defined (DEVELOP_IN_NRF52832)
    if ((((*(uint32_t *)0xF0000FE0) & 0x000000FF) == 0x6) && (((*(uint32_t *)0xF0000FE4) & 0x0000000F) == 0x0)){
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x30){
            return true;
        }
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x40){
            return true;
        }
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x50){
            return true;
        }
    }
    #endif
    
    return true;
}

#if defined (DEVELOP_IN_NRF52832)
static bool errata_37(void)
{
    if ((((*(uint32_t *)0xF0000FE0) & 0x000000FF) == 0x6) && (((*(uint32_t *)0xF0000FE4) & 0x0000000F) == 0x0)){
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x30){
            return true;
        }
    }

    return false;
}
#endif

#if defined (DEVELOP_IN_NRF52832)
static bool errata_57(void)
{
    if ((((*(uint32_t *)0xF0000FE0) & 0x000000FF) == 0x6) && (((*(uint32_t *)0xF0000FE4) & 0x0000000F) == 0x0)){
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x30){
            return true;
        }
    }

    return false;
}
#endif

static bool errata_66(void)
{
    if ((*(uint32_t *)0x10000130ul == 0xAul) && (*(uint32_t *)0x10000134ul == 0x0ul)){
        return true;
    }
    
    #if defined (DEVELOP_IN_NRF52832)
    if ((((*(uint32_t *)0xF0000FE0) & 0x000000FF) == 0x6) && (((*(uint32_t *)0xF0000FE4) & 0x0000000F) == 0x0)){
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x50){
            return true;
        }
    }
    #endif
    
    return true;
}

static bool errata_103(void)
{
    if ((*(uint32_t *)0x10000130ul == 0xAul) && (*(uint32_t *)0x10000134ul == 0x0ul)){
        return true;
    }
    
    return true;
}

#if defined (DEVELOP_IN_NRF52832)
static bool errata_108(void)
{
    if ((((*(uint32_t *)0xF0000FE0) & 0x000000FF) == 0x6) && (((*(uint32_t *)0xF0000FE4) & 0x0000000F) == 0x0)){
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x30){
            return true;
        }
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x40){
            return true;
        }
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x50){
            return true;
        }
    }

    return false;
}
#endif

static bool errata_136(void)
{
    if ((*(uint32_t *)0x10000130ul == 0xAul) && (*(uint32_t *)0x10000134ul == 0x0ul)){
        return true;
    }
    
    #if defined (DEVELOP_IN_NRF52832)
    if ((((*(uint32_t *)0xF0000FE0) & 0x000000FF) == 0x6) && (((*(uint32_t *)0xF0000FE4) & 0x0000000F) == 0x0)){
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x30){
            return true;
        }
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x40){
            return true;
        }
        if (((*(uint32_t *)0xF0000FE8) & 0x000000F0) == 0x50){
            return true;
        }
    }
    #endif
    
    return true;
}

#if defined (DEVELOP_IN_NRF52832)
static bool errata_182(void)
{
    if (*(uint32_t *)0x10000130ul == 0x6ul){
        if (*(uint32_t *)0x10000134ul == 0x6ul){
            return true;
        }
    }

    return false;
}
#endif


/*lint --flb "Leave library region" */
