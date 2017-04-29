/*
 *   Copyright (C) 2015 Texas Instruments Incorporated
 *
 *   All rights reserved. Property of Texas Instruments Incorporated.
 *   Restricted rights to use, duplicate or disclose this code are
 *   granted through contract.
 *
 *   The program may not be used without the written permission of
 *   Texas Instruments Incorporated or against the terms and conditions
 *   stipulated in the agreement under which this program has been supplied,
 *   and under no circumstances can it be used with non-TI connectivity device.
 *
 */
/******************************************************************************
* 	cc_pal.c
*
*   SimpleLink Wi-Fi abstraction file for CC3220
******************************************************************************/

/* Board includes */
#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/porting/cc_pal.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/SPI.h>

#include <ti/devices/cc32xx/inc/hw_ints.h>
#include <ti/devices/cc32xx/inc/hw_udma.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_mcspi.h>
#include <ti/devices/cc32xx/inc/hw_common_reg.h>
#include <ti/devices/cc32xx/inc/hw_ocp_shared.h>
#include <ti/devices/cc32xx/inc/hw_apps_rcm.h>
#include <ti/devices/cc32xx/inc/hw_gprcm.h>
#include <ti/devices/cc32xx/inc/hw_hib1p2.h>
#include "ti/devices/cc32xx/driverlib/rom.h"
#include "ti/devices/cc32xx/driverlib/rom_map.h"
#include <ti/devices/cc32xx/driverlib/interrupt.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/devices/cc32xx/driverlib/timer.h>
#include <ti/drivers/net/wifi/source/driver.h>

/* NWP_SPARE_REG_5 - (OCP_SHARED_BASE + OCP_SHARED_O_SPARE_REG_5)
	- Bits 31:02 - Reserved
	- Bits 01	 - SLSTOP1 - NWP in Reset, Power Domain Down
	- Bits 00    - Reserved
*/
#define NWP_SPARE_REG_5	                (OCP_SHARED_BASE + OCP_SHARED_O_SPARE_REG_5)
#define NWP_SPARE_REG_5_SLSTOP          (0x00000002)

/* ANA_DCDC_PARAMS0 - (HIB1P2_BASE + HIB1P2_O_ANA_DCDC_PARAMETERS0)
	- Bits 31:28 - Reserved
	- Bits 27    - Override PWM mode (==> PFM)
	- Bits 26:00 - Reserved
*/
#define ANA_DCDC_PARAMS0                (HIB1P2_BASE + HIB1P2_O_ANA_DCDC_PARAMETERS0)
#define ANA_DCDC_PARAMS0_PWMOVERRIDE    (0x08000000)

/* WAKENWP - (ARCM_BASE + APPS_RCM_O_APPS_TO_NWP_WAKE_REQUEST)
	- Bits 31:01 - Reserved
	- Bits 00    - Wake Request to NWP
*/
#define WAKENWP                         (ARCM_BASE + APPS_RCM_O_APPS_TO_NWP_WAKE_REQUEST)
#define WAKENWP_WAKEREQ                 (APPS_RCM_APPS_TO_NWP_WAKE_REQUEST_APPS_TO_NWP_WAKEUP_REQUEST)

/* NWP_PWR_STATE - (GPRCM_BASE + GPRCM_O_NWP_PWR_STATE)
	- Bits 31:12 - Reserved
	- Bits 11:08 - Active (0x3)
	- Bits 07:00 - Reserved
*/
#define NWP_PWR_STATE                   (GPRCM_BASE + GPRCM_O_NWP_PWR_STATE)
#define NWP_PWR_STATE_PWRMASK           (0x00000F00)
#define NWP_PWR_STATE_PWRACTIVE         (0x00000300)

/* NWP_LPDS_WAKEUPCFG - (GPRCM_BASE + GPRCM_O_NWP_LPDS_WAKEUP_CFG)
	- Bits 31:08 - Reserved
	- Bits 07:00 - WakeUp Config AppsToNwp Wake (0x20) - reset condition
*/
#define NWP_LPDS_WAKEUPCFG              (GPRCM_BASE + GPRCM_O_NWP_LPDS_WAKEUP_CFG)
#define NWP_LPDS_WAKEUPCFG_APPS2NWP     (0x00000020)
#define NWP_LPDS_WAKEUPCFG_TIMEOUT_MSEC (600)

/* N2A_INT_MASK_SET -                   (COMMON_REG_BASE + COMMON_REG_O_NW_INT_MASK_SET)  */
#define N2A_INT_MASK_SET                (COMMON_REG_BASE + COMMON_REG_O_NW_INT_MASK_SET)
/* N2A_INT_MASK_CLR -                   (COMMON_REG_BASE + COMMON_REG_O_NW_INT_MASK_CLR)  */
#define N2A_INT_MASK_CLR                (COMMON_REG_BASE + COMMON_REG_O_NW_INT_MASK_CLR)
/* N2A_INT_ACK -                        (COMMON_REG_BASE + COMMON_REG_O_NW_INT_ACK)       */
#define N2A_INT_ACK                     (COMMON_REG_BASE + COMMON_REG_O_NW_INT_ACK)
#define NWP_N2A_INT_ACK_TIMEOUT_MSEC    (3000)

/* A2N_INT_STS_CLR -                    (COMMON_REG_BASE + COMMON_REG_O_APPS_INT_STS_CLR)  */
#define A2N_INT_STS_CLR                 (COMMON_REG_BASE + COMMON_REG_O_APPS_INT_STS_CLR)
/* A2N_INT_TRIG -                       (COMMON_REG_BASE + COMMON_REG_O_APPS_INT_TRIG)     */
#define A2N_INT_TRIG                    (COMMON_REG_BASE + COMMON_REG_O_APPS_INT_TRIG)
/* A2N_INT_STS_RAW -                    (COMMON_REG_BASE + COMMON_REG_O_APPS_INT_STS_RAW)  */
#define A2N_INT_STS_RAW                 (COMMON_REG_BASE + COMMON_REG_O_APPS_INT_STS_RAW)

#define MAX_DMA_RECV_TRANSACTION_SIZE   (4096)
#define SPI_RATE_20M                    (20000000)

HwiP_Handle g_intHandle = 0;

//****************************************************************************
//                          LOCAL FUNCTIONS
//****************************************************************************

Fd_t spi_Open(char *ifName, unsigned long flags)
{
	void *lspi_hndl;
	unsigned int lspi_index;
	SPI_Params SPI_Config;
	SPI_Params_init(&SPI_Config);

	/* configure the SPI settings */
	SPI_Config.transferMode = SPI_MODE_BLOCKING;
	SPI_Config.mode = SPI_MASTER;
	SPI_Config.bitRate = SPI_RATE_20M;
	SPI_Config.dataSize = 32;
	SPI_Config.frameFormat = SPI_POL0_PHA0;

	/* index of the link SPI initialization configuration in the SPI_Config table */
	lspi_index = 0;
	lspi_hndl = SPI_open(lspi_index, &SPI_Config);
	if(NULL == lspi_hndl)
	{
        return -1;
	}
	else
	{
        return (Fd_t)lspi_hndl;
	}
}


int spi_Close(Fd_t fd)
{
	SPI_close((void *)fd);
    return 0;
}


int spi_Read(Fd_t fd, unsigned char *pBuff, int len)
{
    SPI_Transaction transact_details;
    int read_size = 0;

    /* check if the link SPI has been initialized successfully */
    if(fd < 0)
    {
        return -1;
    }

	transact_details.txBuf = NULL;
	transact_details.arg = NULL;
	while(len > 0)
	{
		/* DMA can transfer upto a maximum of 1024 words in one go. So, if
		   the data to be read is more than 1024 words, it will be done in
		   parts */
		/* length is received in bytes, should be specified in words for the
		 * SPI driver.
		 */
		if(len > MAX_DMA_RECV_TRANSACTION_SIZE)
		{
			transact_details.count = (MAX_DMA_RECV_TRANSACTION_SIZE +3)>>2;
			transact_details.rxBuf = (void*)(pBuff + read_size);
			if(SPI_transfer((SPI_Handle)fd, &transact_details))
			{
				read_size += MAX_DMA_RECV_TRANSACTION_SIZE;
				len = len - MAX_DMA_RECV_TRANSACTION_SIZE;
			}
			else
			{
				return -1;
			}

		}
		else
		{
			transact_details.count = (len+3)>>2;
			transact_details.rxBuf = (void*)(pBuff + read_size);
			if(SPI_transfer((SPI_Handle)fd, &transact_details))
			{
				read_size += len;
				len = 0;
				return read_size;
			}
			else
			{
				 return -1;
			}
		}
	}

    return(read_size);
}


int spi_Write(Fd_t fd, unsigned char *pBuff, int len)
{
    SPI_Transaction transact_details;
    int write_size = 0;

    /* check if the link SPI has been initialized successfully */
    if(fd < 0)
    {
	return -1;
    }

	transact_details.rxBuf = NULL;
	transact_details.arg = NULL;
	while(len > 0)
	{
		/* configure the transaction details.
		 * length is received in bytes, should be specified in words for the SPI
		 * driver.
		 */
		if(len > MAX_DMA_RECV_TRANSACTION_SIZE)
		{
			transact_details.count = (MAX_DMA_RECV_TRANSACTION_SIZE +3)>>2;
			transact_details.txBuf = (void*)(pBuff + write_size);
			if(SPI_transfer((SPI_Handle)fd, &transact_details))
			{
				write_size += MAX_DMA_RECV_TRANSACTION_SIZE;
				len = len - MAX_DMA_RECV_TRANSACTION_SIZE;
			}
			else
			{
				return -1;
			}
		}
		else
		{
			transact_details.count = (len+3)>>2;
			transact_details.txBuf = (void*)(pBuff + write_size);
			if(SPI_transfer((SPI_Handle)fd, &transact_details))
			{
				write_size += len;
				len = 0;
				return write_size;
			}
			else
			{
				 return -1;
			}
		}
	}

    return(write_size);
}


int NwpRegisterInterruptHandler(P_EVENT_HANDLER InterruptHdl , void* pValue)
{

	HwiP_Params nwp_iParams;

	HwiP_clearInterrupt(INT_NWPIC);

	if(!InterruptHdl)
	{
		return(HwiP_delete(g_intHandle));
	}
	else
	{
		nwp_iParams.priority = INT_PRIORITY_LVL_1 ;
		nwp_iParams.name = "host_irq" ;

	}
		g_intHandle = HwiP_create(INT_NWPIC , (HwiP_Fxn)(InterruptHdl) , &nwp_iParams);

	if(!g_intHandle)
	{
		return -1;
	}
	else
	{
		return OS_OK ;
	}
}


void NwpMaskInterrupt()
{
	(*(unsigned long *)N2A_INT_MASK_SET) = 0x1;
}


void NwpUnMaskInterrupt()
{
	(*(unsigned long *)N2A_INT_MASK_CLR) = 0x1;
}


void NwpPowerOn(void)
{
    /* bring the 1.32 eco out of reset */
    HWREG(NWP_SPARE_REG_5) &= ~NWP_SPARE_REG_5_SLSTOP;

    /* Clear host IRQ indication */
    HWREG(N2A_INT_ACK) = 1;

    /* NWP Wake-up */
    HWREG(WAKENWP) = WAKENWP_WAKEREQ;

    //UnMask Host Interrupt
    NwpUnMaskInterrupt();
}


void NwpPowerOff(void)
{

    volatile unsigned long apps_int_sts_raw;
    volatile unsigned long sl_stop_ind       = HWREG(NWP_SPARE_REG_5);
    volatile unsigned long nwp_lpds_wake_cfg = HWREG(NWP_LPDS_WAKEUPCFG);
    _SlTimeoutParams_t SlTimeoutInfo         = {0};

    if((nwp_lpds_wake_cfg != NWP_LPDS_WAKEUPCFG_APPS2NWP) &&     /* Check for NWP POR condition - APPS2NWP is reset condition */
                !(sl_stop_ind & NWP_SPARE_REG_5_SLSTOP))         /* Check if sl_stop was executed */
    {
	HWREG(0xE000E104) = 0x200;               /* Enable the out of band interrupt, this is not a wake-up source*/
	HWREG(A2N_INT_TRIG) = 0x1;               /* Trigger out of band interrupt  */
	HWREG(WAKENWP) = WAKENWP_WAKEREQ;        /* Wake-up the NWP */

	    _SlDrvStartMeasureTimeout(&SlTimeoutInfo, NWP_N2A_INT_ACK_TIMEOUT_MSEC);

	   /* Wait for the A2N_INT_TRIG to be cleared by the NWP to indicate it's awake and ready for shutdown.
		* poll until APPs->NWP interrupt is cleared or timeout :
		* for service pack 3.1.99.1 or higher, this condition is fulfilled in less than 1 mSec.
		* Otherwise, in some cases it may require up to 3000 mSec of waiting.  */

	apps_int_sts_raw = HWREG(A2N_INT_STS_RAW);
	while(!(apps_int_sts_raw & 0x1))
	{
		if(_SlDrvIsTimeoutExpired(&SlTimeoutInfo))
		{
			break;
		}
		apps_int_sts_raw = HWREG(A2N_INT_STS_RAW);
	}

	WAIT_NWP_SHUTDOWN_READY;
   }

   /* Clear Out of band interrupt, Acked by the NWP */
   HWREG(A2N_INT_STS_CLR) = 0x1;

   /* Mask Host Interrupt */
   NwpMaskInterrupt();

   /* Switch to PFM Mode */
   HWREG(ANA_DCDC_PARAMS0) &= ~ANA_DCDC_PARAMS0_PWMOVERRIDE;

   /* sl_stop ECO for PG1.32 devices */
   HWREG(NWP_SPARE_REG_5) |= NWP_SPARE_REG_5_SLSTOP;

   ClockP_usleep(1000);
}


int Semaphore_create_handle(SemaphoreP_Handle* pSemHandle)
{
    SemaphoreP_Params params;

    SemaphoreP_Params_init(&params);
#ifndef SL_PLATFORM_MULTI_THREADED
    params.callback = tiDriverSpawnCallback;
#endif
	(*(pSemHandle)) = SemaphoreP_create(0, &params);

	if(!(*(pSemHandle)))
	{
		return SemaphoreP_FAILURE ;
	}

	return SemaphoreP_OK;
}


int Mutex_create_handle(MutexP_Handle* pMutexHandle)
{
    MutexP_Params params;

    MutexP_Params_init(&params);
#ifndef SL_PLATFORM_MULTI_THREADED
    params.callback = tiDriverSpawnCallback;
#endif

	(*(pMutexHandle)) = MutexP_create(&params);

	if(!(*(pMutexHandle)))
	{
		return MutexP_FAILURE ;
	}

	return MutexP_OK;
}


int Mutex_unlock(MutexP_Handle pMutexHandle)
{
	MutexP_unlock(pMutexHandle, 0);
	return(MutexP_OK);
}


int Mutex_lock(MutexP_Handle pMutexHandle)
{
	MutexP_lock(pMutexHandle);
	return(MutexP_OK);
}


unsigned long TimerGetCurrentTimestamp()
{
    return (ClockP_getSystemTicks());
}


void NwpWaitForShutDownInd()
{
	volatile unsigned long nwp_wakup_ind = HWREG(NWP_LPDS_WAKEUPCFG);
	_SlTimeoutParams_t SlTimeoutInfo = {0};

	_SlDrvStartMeasureTimeout(&SlTimeoutInfo, NWP_LPDS_WAKEUPCFG_TIMEOUT_MSEC);

    while(nwp_wakup_ind != NWP_LPDS_WAKEUPCFG_APPS2NWP)
    {
	if(_SlDrvIsTimeoutExpired(&SlTimeoutInfo))
	{
		return;
	}
	nwp_wakup_ind = HWREG(NWP_LPDS_WAKEUPCFG);
    }

    return ;
}
