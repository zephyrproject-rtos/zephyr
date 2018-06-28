/***************************************************************************//**
* \file cy_flash.c
* \version 3.20
*
* \brief
* Provides the public functions for the API for the PSoC 6 Flash Driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/
#include "cy_flash.h"
#include "cy_sysclk.h"
#include "cy_sysint.h"
#include "cy_ipc_drv.h"
#include "cy_ipc_sema.h"
#include "cy_ipc_pipe.h"
#include "cy_device.h"


/***************************************
* Data Structure definitions
***************************************/

/* Flash driver context */
typedef struct
{
    uint32_t opcode;      /**< Specifies the code of flash operation */
    uint32_t arg1;        /**< Specifies the configuration of flash operation */
    uint32_t arg2;        /**< Specifies the configuration of flash operation */
    uint32_t arg3;        /**< Specifies the configuration of flash operation */
} cy_stc_flash_context_t;


/***************************************
* Macro definitions
***************************************/

/** \cond INTERNAL */
/** Set SROM API in blocking mode */
#define CY_FLASH_BLOCKING_MODE             ((0x01UL) << 8UL)
/** Set SROM API in non blocking mode */
#define CY_FLASH_NON_BLOCKING_MODE         (0UL)

/** SROM API flash region ID shift for flash row information */
#define CY_FLASH_REGION_ID_SHIFT           (16U)
#define CY_FLASH_REGION_ID_MASK            (3U)
#define CY_FLASH_ROW_ID_MASK               (0xFFFFU)
/** SROM API flash region IDs */
#define CY_FLASH_REGION_ID_MAIN            (0UL)
#define CY_FLASH_REGION_ID_EM_EEPROM       (1UL)
#define CY_FLASH_REGION_ID_SFLASH          (2UL)

/** SROM API opcode mask */
#define CY_FLASH_OPCODE_Msk                ((0xFFUL) << 24UL)
/** SROM API opcode for flash write operation */
#define CY_FLASH_OPCODE_WRITE_ROW          ((0x05UL) << 24UL)
/** SROM API opcode for flash program operation */
#define CY_FLASH_OPCODE_PROGRAM_ROW        ((0x06UL) << 24UL)
/** SROM API opcode for row erase operation */
#define CY_FLASH_OPCODE_ERASE_ROW          ((0x1CUL) << 24UL)
/** SROM API opcode for flash checksum operation */
#define CY_FLASH_OPCODE_CHECKSUM           ((0x0BUL) << 24UL)
/** SROM API opcode for flash hash operation */
#define CY_FLASH_OPCODE_HASH               ((0x0DUL) << 24UL)
/** SROM API flash row shift for flash checksum operation */
#define CY_FLASH_OPCODE_CHECKSUM_ROW_SHIFT (8UL)
/** SROM API flash row shift for flash checksum operation */
#define CY_FLASH_OPCODE_CHECKSUM_REGION_SHIFT (22UL)
/** SROM API flash data size parameter for flash write operation */
#define CY_FLASH_CONFIG_DATASIZE           (CPUSS_FLASHC_PA_SIZE_LOG2 - 1UL)
/** Data to be programmed to flash is located in SRAM memory region */
#define CY_FLASH_DATA_LOC_SRAM             (0x100UL)
/** SROM API flash verification option for flash write operation */
#define CY_FLASH_CONFIG_VERIFICATION_EN    ((0x01UL) << 16u)

/** Command completed with no errors */
#define CY_FLASH_ROMCODE_SUCCESS                   (0xA0000000UL)
/** Invalid device protection state */
#define CY_FLASH_ROMCODE_INVALID_PROTECTION        (0xF0000001UL)
/** Invalid flash page latch address */
#define CY_FLASH_ROMCODE_INVALID_FM_PL             (0xF0000003UL)
/** Invalid flash address */
#define CY_FLASH_ROMCODE_INVALID_FLASH_ADDR        (0xF0000004UL)
/** Row is write protected */
#define CY_FLASH_ROMCODE_ROW_PROTECTED             (0xF0000005UL)
/** Comparison between Page Latches and FM row failed */
#define CY_FLASH_ROMCODE_PL_ROW_COMP_FA            (0xF0000022UL)
/** Command in progress; no error */
#define CY_FLASH_ROMCODE_IN_PROGRESS_NO_ERROR      (0xA0000009UL)
/** Flash operation is successfully initiated */
#define CY_FLASH_IS_OPERATION_STARTED              (0x00000010UL)
/** Flash is under operation */
#define CY_FLASH_IS_BUSY                           (0x00000040UL)
/** IPC structure is already locked by another process */
#define CY_FLASH_IS_IPC_BUSY                       (0x00000080UL)
/** Input parameters passed to Flash API are not valid */
#define CY_FLASH_IS_INVALID_INPUT_PARAMETERS       (0x00000100UL)

/** Result mask */
#define CY_FLASH_RESULT_MASK                       (0x0FFFFFFFUL)
/** Error shift */
#define CY_FLASH_ERROR_SHIFT                       (28UL)
/** No error */
#define CY_FLASH_ERROR_NO_ERROR                    (0xAUL)

/** CM4 Flash Proxy address */
#define CY_FLASH_CM4_FLASH_PROXY_ADDR              (*(Cy_Flash_Proxy *)(0x00000D1CUL))
typedef cy_en_flashdrv_status_t (*Cy_Flash_Proxy)(cy_stc_flash_context_t *context);

/** IPC notify bit for IPC_STRUCT0 (dedicated to flash operation) */
#define CY_FLASH_IPC_NOTIFY_STRUCT0                (0x1UL << CY_IPC_INTR_SYSCALL1)

/** Disable delay */
#define CY_FLASH_NO_DELAY                          (0U)

#if !defined(CY_FLASH_RWW_DRV_SUPPORT_DISABLED)
    /** Number of ticks to wait 1 uS */
    #define CY_FLASH_TICKS_FOR_1US                     (8U)
    /** Slow control register */
    #define CY_FLASH_TST_DDFT_SLOW_CTL_REG             (*(reg32 *) 0x40260108U)
    /** Slow control register */
    #define CY_FLASH_TST_DDFT_FAST_CTL_REG             (*(reg32 *) 0x40260104U)
    /** Define to set the IMO to perform a delay after the flash operation started */
    #define CY_FLASH_TST_DDFT_SLOW_CTL_MASK            (0x00001F1EUL)
    /** Fast control register */
    #define CY_FLASH_TST_DDFT_FAST_CTL_MASK            (62U)
    /** Slow output register - output disabled */
    #define CY_FLASH_CLK_OUTPUT_DISABLED               (0U)

    /* The default delay time value */
    #define CY_FLASH_DEFAULT_DELAY                     (150UL)
    /* Calculates the time in microseconds to wait for the number of the CM0P ticks */
    #define CY_FLASH_DELAY_CORRECTIVE(ticks)           ((((uint32)Cy_SysClk_ClkPeriGetDivider() + 1UL) * \
                                                       (Cy_SysClk_ClkSlowGetDivider() + 1UL) * (ticks) * 1000UL)\
                                                       / ((uint32_t)cy_Hfclk0FreqHz / 1000UL))

    /* Number of the CM0P ticks for StartProgram function delay corrective time */
    #define CY_FLASH_START_PROGRAM_DELAY_TICKS         (6000UL)
    /* Delay time for StartProgram function in us */
    #define CY_FLASH_START_PROGRAM_DELAY_TIME          (900UL + CY_FLASH_DELAY_CORRECTIVE(CY_FLASH_START_PROGRAM_DELAY_TICKS))
    /* Number of the CM0P ticks for StartErase function delay corrective time */
    #define CY_FLASH_START_ERASE_DELAY_TICKS           (9500UL)
    /* Delay time for StartErase function in us */
    #define CY_FLASH_START_ERASE_DELAY_TIME            (2200UL + CY_FLASH_DELAY_CORRECTIVE(CY_FLASH_START_ERASE_DELAY_TICKS))
    /* Number of the CM0P ticks for StartWrite function delay corrective time */
    #define CY_FLASH_START_WRITE_DELAY_TICKS           (19000UL)
    /* Delay time for StartWrite function in us */
    #define CY_FLASH_START_WRITE_DELAY_TIME            (9800UL + CY_FLASH_DELAY_CORRECTIVE(CY_FLASH_START_WRITE_DELAY_TICKS))

    /** Delay time for Start Write function in us with corrective time */
    #define CY_FLASH_START_WRITE_DELAY                 (CY_FLASH_START_WRITE_DELAY_TIME)
    /** Delay time for Start Program function in us with corrective time */
    #define CY_FLASH_START_PROGRAM_DELAY               (CY_FLASH_START_PROGRAM_DELAY_TIME)
    /** Delay time for Start Erase function in uS with corrective time */
    #define CY_FLASH_START_ERASE_DELAY                 (CY_FLASH_START_ERASE_DELAY_TIME)

    #define CY_FLASH_ENTER_WAIT_LOOP                   (0xFFU)
    #define CY_FLASH_IPC_CLIENT_ID                     (2U)

    /** Semaphore number reserved for flash driver */
    #define CY_FLASH_WAIT_SEMA                         (0UL)
    /* Semaphore check timeout (in tries) */
    #define CY_FLASH_SEMA_WAIT_MAX_TRIES               (150000UL)

    typedef struct
    {
        uint8_t  clientID;
        uint8_t  pktType;
        uint16_t intrRelMask;
    } cy_flash_notify_t;

    static void Cy_Flash_RAMDelay(uint32_t microseconds);

    #if (CY_CPU_CORTEX_M0P)
        #define IS_CY_PIPE_FREE(...)       (!Cy_IPC_Drv_IsLockAcquired(Cy_IPC_Drv_GetIpcBaseAddress(CY_IPC_CHAN_CYPIPE_EP1)))
        #define NOTIFY_PEER_CORE(a)         Cy_IPC_Pipe_SendMessage(CY_IPC_EP_CYPIPE_CM4_ADDR, CY_IPC_EP_CYPIPE_CM0_ADDR, (a), NULL)
    #else
        #define IS_CY_PIPE_FREE(...)       (!Cy_IPC_Drv_IsLockAcquired(Cy_IPC_Drv_GetIpcBaseAddress(CY_IPC_CHAN_CYPIPE_EP0)))
        #define NOTIFY_PEER_CORE(a)         Cy_IPC_Pipe_SendMessage(CY_IPC_EP_CYPIPE_CM0_ADDR, CY_IPC_EP_CYPIPE_CM4_ADDR, (a), NULL)
    #endif

    static void Cy_Flash_NotifyHandler(uint32_t * msgPtr);
#endif /* !defined(CY_FLASH_RWW_DRV_SUPPORT_DISABLED) */
/** \endcond */

/* Static functions */
static bool Cy_Flash_BoundsCheck(uint32_t flashAddr);
static uint32_t Cy_Flash_GetRowNum(uint32_t flashAddr);
static cy_en_flashdrv_status_t Cy_Flash_ProcessOpcode(uint32_t opcode);
static cy_en_flashdrv_status_t Cy_Flash_OperationStatus(void);
static cy_en_flashdrv_status_t Cy_Flash_SendCmd(uint32_t mode, uint32_t microseconds);

static volatile cy_stc_flash_context_t flashContext;


#if !defined(CY_FLASH_RWW_DRV_SUPPORT_DISABLED)
    /*******************************************************************************
    * Function Name: Cy_Flash_NotifyHandler
    ****************************************************************************//**
    *
    * This is the interrupt service routine for the pipe notifications.
    *
    *******************************************************************************/
    typedef struct
    {
        uint32_t maxSema;      /* Maximum semaphores in system */
        uint32_t *arrayPtr;    /* Pointer to semaphores array  */
    } cy_stc_ipc_sema_t;

    #if defined (__ICCARM__)
        #pragma diag_suppress=Ta023
        __ramfunc
    #else
        CY_SECTION(".cy_ramfunc")
    #endif
    static void Cy_Flash_NotifyHandler(uint32_t * msgPtr)
    {
        uint32_t intr;
        static uint32_t semaIndex;
        static uint32_t semaMask;
        static volatile uint32_t *semaPtr;
        static cy_stc_ipc_sema_t *semaStruct;

        cy_flash_notify_t *ipcMsgPtr = (cy_flash_notify_t *)msgPtr;

        if (CY_FLASH_ENTER_WAIT_LOOP == ipcMsgPtr->pktType)
        {
            intr = Cy_SysLib_EnterCriticalSection();

            /* Get pointer to structure */
            semaStruct = (cy_stc_ipc_sema_t *)Cy_IPC_Drv_ReadDataValue(Cy_IPC_Drv_GetIpcBaseAddress(CY_IPC_CHAN_SEMA));

            /* Get the index into the semaphore array and calculate the mask */
            semaIndex = CY_FLASH_WAIT_SEMA / CY_IPC_SEMA_PER_WORD;
            semaMask = (uint32_t)(1ul << (CY_FLASH_WAIT_SEMA - (semaIndex * CY_IPC_SEMA_PER_WORD) ));
            semaPtr = &semaStruct->arrayPtr[semaIndex];

            /* Notification to the Flash driver to start the current operation */
            *semaPtr |= semaMask;

            /* Check a notification from other core to end of waiting */
            while (((*semaPtr) & semaMask) != 0ul)
            {
            }

            Cy_SysLib_ExitCriticalSection(intr);
        }
    }
    #if defined (__ICCARM__)
        #pragma diag_default=Ta023
    #endif
#endif /* !defined(CY_FLASH_RWW_DRV_SUPPORT_DISABLED) */


/*******************************************************************************
* Function Name: Cy_Flash_Init
****************************************************************************//**
*
* Initiates all needed prerequisites to support flash erase/write.
* Should be called from each core.
*
* Requires a call to Cy_IPC_SystemSemaInit() and Cy_IPC_SystemPipeInit() functions
* before use.
*
* This function is called in the SystemInit() function, for proper flash write
* and erase operations. If the default startup file is not used, or the function
* SystemInit() is not called in your project, call the following three functions
* prior to executing any flash or EmEEPROM write or erase operations:
* -# Cy_IPC_SystemSemaInit()
* -# Cy_IPC_SystemPipeInit()
* -# Cy_Flash_Init()
*
*******************************************************************************/
void Cy_Flash_Init(void)
{
    #if !defined(CY_FLASH_RWW_DRV_SUPPORT_DISABLED)    
        if (cy_device->flashRwwSupport)
        {
            #if (CY_CPU_CORTEX_M4)
                cy_stc_sysint_t flashIntConfig =
                {
                    cpuss_interrupt_fm_IRQn,        /* .intrSrc */
                    0                               /* .intrPriority */
                };

                (void)Cy_SysInt_Init(&flashIntConfig, &Cy_Flash_ResumeIrqHandler);
                NVIC_EnableIRQ(flashIntConfig.intrSrc);
            #endif

            #if (CY_IPC_CYPIPE_ENABLE)
                if (cy_device->flashPipeEnable)
                {
                    (void)Cy_IPC_Pipe_RegisterCallback(CY_IPC_EP_CYPIPE_ADDR, &Cy_Flash_NotifyHandler,
                                                      (uint32_t)CY_FLASH_IPC_CLIENT_ID);
                }
            #endif /* (CY_IPC_CYPIPE_ENABLE) */
        }
    #endif /* !defined(CY_FLASH_RWW_DRV_SUPPORT_DISABLED) */
}

/*******************************************************************************
* Function Name: Cy_Flash_SendCmd
****************************************************************************//**
*
* Sends a command to the SROM via the IPC channel. The function is placed to the
* SRAM memory to guarantee successful operation. After an IPC message is sent,
* the function waits for a defined time before exiting the function.
*
* \param mode
* Sets the blocking or non-blocking Flash operation.
*
* \param microseconds
* The number of microseconds to wait before exiting the functions
* in range 0-65535 us.
*
* \return Returns the status of the Flash operation,
* see \ref cy_en_flashdrv_status_t.
*
*******************************************************************************/
#if defined (__ICCARM__)
    #pragma diag_suppress=Ta023
    __ramfunc
#else
    CY_SECTION(".cy_ramfunc")
#endif
static cy_en_flashdrv_status_t Cy_Flash_SendCmd(uint32_t mode, uint32_t microseconds)
{
    cy_en_flashdrv_status_t result = CY_FLASH_DRV_IPC_BUSY;
    IPC_STRUCT_Type *ipcBase = Cy_IPC_Drv_GetIpcBaseAddress(CY_IPC_CHAN_SYSCALL);

#if !defined(CY_FLASH_RWW_DRV_SUPPORT_DISABLED)
    uint32_t semaTryCount = 0uL;
    uint32_t intr;

    CY_ALIGN(4) static cy_flash_notify_t ipcWaitMessage =
    {
        /* .clientID    */ CY_FLASH_IPC_CLIENT_ID,
        /* .pktType     */ CY_FLASH_ENTER_WAIT_LOOP,
        /* .intrRelMask */ 0u
    };

    if (cy_device->flashRwwSupport)
    {
        /* Check for active core is CM0+, or CM4 on single core device */
        #if ((CY_CPU_CORTEX_M0P) || !(__CM0P_PRESENT))
            #if (CY_CPU_CORTEX_M0P)
            bool isPeerCoreEnabled = (CY_SYS_CM4_STATUS_ENABLED == Cy_SysGetCM4Status());
            #else
            bool isPeerCoreEnabled = false;
            #endif

            if (!isPeerCoreEnabled)
            {
                result = CY_FLASH_DRV_SUCCESS;
            }
            else
            {
        #endif
                if (IS_CY_PIPE_FREE())
                {
                    if (CY_IPC_SEMA_STATUS_LOCKED != Cy_IPC_Sema_Status(CY_FLASH_WAIT_SEMA))
                    {
                        if (CY_IPC_PIPE_SUCCESS == NOTIFY_PEER_CORE(&ipcWaitMessage))
                        {
                            /* Wait for SEMA lock by peer core */
                            while ((CY_IPC_SEMA_STATUS_LOCKED != Cy_IPC_Sema_Status(CY_FLASH_WAIT_SEMA)) && ((semaTryCount < CY_FLASH_SEMA_WAIT_MAX_TRIES)))
                            {
                                /* check for timeout (as maximum tries count) */
                                ++semaTryCount;
                            }

                            if (semaTryCount < CY_FLASH_SEMA_WAIT_MAX_TRIES)
                            {
                                result = CY_FLASH_DRV_SUCCESS;
                            }
                        }
                    }
                }
        #if ((CY_CPU_CORTEX_M0P) || !(__CM0P_PRESENT))
            }
        #endif

        if (CY_FLASH_DRV_SUCCESS == result)
        {
            /* Notifier is ready, start of the operation */
            intr = Cy_SysLib_EnterCriticalSection();

            if (0UL != _FLD2VAL(SRSS_CLK_CAL_CNT1_CAL_COUNTER_DONE, SRSS->CLK_CAL_CNT1))
            {
               /* Tries to acquire the IPC structure and pass the arguments to SROM API */
                if (Cy_IPC_Drv_SendMsgPtr(ipcBase, CY_FLASH_IPC_NOTIFY_STRUCT0, (void*)&flashContext) == CY_IPC_DRV_SUCCESS)
                {
                    if (cy_device->flashRwwSupport)
                    {
                        Cy_Flash_RAMDelay(microseconds);
                    }

                    if (mode == CY_FLASH_NON_BLOCKING_MODE)
                    {
                        /* The Flash operation is successfully initiated */
                        result = CY_FLASH_DRV_OPERATION_STARTED;
                    }
                    else
                    {
                        while (0u != _FLD2VAL(IPC_STRUCT_ACQUIRE_SUCCESS, REG_IPC_STRUCT_LOCK_STATUS(ipcBase)))
                        {
                            /* Polls whether the IPC is released and the Flash operation is performed */
                        }
                        //Cy_IPC_Drv_IsLockAcquired(Cy_IPC_Drv_GetIpcBaseAddress(CY_IPC_CHAN_SYSCALL))

                        result = Cy_Flash_OperationStatus();
                    }
                }
                else
                {
                    /* The IPC structure is already locked by another process */
                    result = CY_FLASH_DRV_IPC_BUSY;
                }
            }
        #if ((CY_CPU_CORTEX_M0P) || !(__CM0P_PRESENT))
            if (isPeerCoreEnabled)
            {
        #endif
                while (CY_IPC_SEMA_SUCCESS != Cy_IPC_Sema_Clear(CY_FLASH_WAIT_SEMA, true))
                {
                    /* Clear SEMA lock */
                }
        #if ((CY_CPU_CORTEX_M0P) || !(__CM0P_PRESENT))
            }
        #endif

            Cy_SysLib_ExitCriticalSection(intr);
            /* End of the flash operation */
        }
    }
    else
#endif /* !defined(CY_FLASH_RWW_DRV_SUPPORT_DISABLED) */
    {
       /* Tries to acquire the IPC structure and pass the arguments to SROM API */
        if (Cy_IPC_Drv_SendMsgPtr(ipcBase, CY_FLASH_IPC_NOTIFY_STRUCT0, (void*)&flashContext) == CY_IPC_DRV_SUCCESS)
        {
            if (mode == CY_FLASH_NON_BLOCKING_MODE)
            {
                /* The Flash operation is successfully initiated */
                result = CY_FLASH_DRV_OPERATION_STARTED;
            }
            else
            {
                while (0u != _FLD2VAL(IPC_STRUCT_ACQUIRE_SUCCESS, REG_IPC_STRUCT_LOCK_STATUS(ipcBase)))
                {
                    /* Polls whether the IPC is released and the Flash operation is performed */
                }

                result = Cy_Flash_OperationStatus();
            }
        }
        else
        {
            /* The IPC structure is already locked by another process */
            result = CY_FLASH_DRV_IPC_BUSY;
        }
    }

    return (result);
}
#if defined (__ICCARM__)
    #pragma diag_default=Ta023
#endif


#if !defined(CY_FLASH_RWW_DRV_SUPPORT_DISABLED)
    /*******************************************************************************
    * Function Name: Cy_Flash_RAMDelay
    ****************************************************************************//**
    *
    * Wait for a defined time in the SRAM memory region.
    *
    * \param microseconds
    * Delay time in microseconds in range 0-65535 us.
    *
    *******************************************************************************/
    #if defined (__ICCARM__)
        #pragma diag_suppress=Ta023
        __ramfunc
    #else
        CY_SECTION(".cy_ramfunc")
    #endif
    static void Cy_Flash_RAMDelay(uint32_t microseconds)
    {
        uint32_t ticks = (microseconds & 0xFFFFUL) * CY_FLASH_TICKS_FOR_1US;
        if (ticks != CY_FLASH_NO_DELAY)
        {
            CY_FLASH_TST_DDFT_FAST_CTL_REG  = CY_FLASH_TST_DDFT_FAST_CTL_MASK;
            CY_FLASH_TST_DDFT_SLOW_CTL_REG  = CY_FLASH_TST_DDFT_SLOW_CTL_MASK;

            SRSS->CLK_OUTPUT_SLOW = _VAL2FLD(SRSS_CLK_OUTPUT_SLOW_SLOW_SEL0, CY_SYSCLK_MEAS_CLK_IMO) |
                                    _VAL2FLD(SRSS_CLK_OUTPUT_SLOW_SLOW_SEL1, CY_FLASH_CLK_OUTPUT_DISABLED);

            /* Load the down-counter without status bit value */
            SRSS->CLK_CAL_CNT1 = _VAL2FLD(SRSS_CLK_CAL_CNT1_CAL_COUNTER1, ticks);

            /* Make sure that the counter is started */
            ticks = _FLD2VAL(SRSS_CLK_CAL_CNT1_CAL_COUNTER_DONE, SRSS->CLK_CAL_CNT1);

            while (0UL == _FLD2VAL(SRSS_CLK_CAL_CNT1_CAL_COUNTER_DONE, SRSS->CLK_CAL_CNT1))
            {
                /* Wait until the counter stops counting */
            }
        }
    }
    #if defined (__ICCARM__)
        #pragma diag_default=Ta023
    #endif

    #if (CY_CPU_CORTEX_M4)

        /* Based on bookmark codes of mxs40srompsoc BROS,002-03298 */
        #define CY_FLASH_PROGRAM_ROW_BOOKMARK        (0x00000001UL)
        #define CY_FLASH_ERASE_ROW_BOOKMARK          (0x00000002UL)
        #define CY_FLASH_WRITE_ROW_ERASE_BOOKMARK    (0x00000003UL)
        #define CY_FLASH_WRITE_ROW_PROGRAM_BOOKMARK  (0x00000004UL)

        /* Number of the CM0P ticks for function delay corrective time at final stage */
        #define CY_FLASH_START_PROGRAM_FINAL_DELAY_TICKS     (1000UL)
        #define CY_FLASH_PROGRAM_ROW_DELAY           (130UL + CY_FLASH_DELAY_CORRECTIVE(CY_FLASH_START_PROGRAM_FINAL_DELAY_TICKS))
        #define CY_FLASH_ERASE_ROW_DELAY             (130UL + CY_FLASH_DELAY_CORRECTIVE(CY_FLASH_START_PROGRAM_FINAL_DELAY_TICKS))
        #define CY_FLASH_WRITE_ROW_ERASE_DELAY       (130UL + CY_FLASH_DELAY_CORRECTIVE(CY_FLASH_START_PROGRAM_FINAL_DELAY_TICKS))
        #define CY_FLASH_WRITE_ROW_PROGRAM_DELAY     (130UL + CY_FLASH_DELAY_CORRECTIVE(CY_FLASH_START_PROGRAM_FINAL_DELAY_TICKS))


        /*******************************************************************************
        * Function Name: Cy_Flash_ResumeIrqHandler
        ****************************************************************************//**
        *
        * This is the interrupt service routine to make additional processing of the
        * flash operations resume phase.
        *
        *******************************************************************************/
        #if defined (__ICCARM__)
            #pragma diag_suppress=Ta023
            __ramfunc
        #else
            CY_SECTION(".cy_ramfunc")
        #endif
        void Cy_Flash_ResumeIrqHandler(void)
        {
            IPC_STRUCT_Type *ipcBase = Cy_IPC_Drv_GetIpcBaseAddress(CY_IPC_CHAN_CYPIPE_EP0);

            uint32_t bookmark;
            bookmark = FLASHC->FM_CTL.BOOKMARK & 0xffffUL;

            uint32_t intr = Cy_SysLib_EnterCriticalSection();

            uint32_t cm0s = CPUSS->CM0_STATUS;

            switch (bookmark)
            {
            case CY_FLASH_PROGRAM_ROW_BOOKMARK:
                if ((cm0s == (CPUSS_CM0_STATUS_SLEEPING_Msk | CPUSS_CM0_STATUS_SLEEPDEEP_Msk)) && ((bool)(CY_IPC_CYPIPE_ENABLE)))
                {
                    REG_IPC_STRUCT_NOTIFY(ipcBase) = _VAL2FLD(IPC_STRUCT_NOTIFY_INTR_NOTIFY, (1UL << CY_IPC_INTR_CYPIPE_EP0));
                }
                Cy_Flash_RAMDelay(CY_FLASH_PROGRAM_ROW_DELAY);
                break;
            case CY_FLASH_ERASE_ROW_BOOKMARK:
                if ((cm0s == (CPUSS_CM0_STATUS_SLEEPING_Msk | CPUSS_CM0_STATUS_SLEEPDEEP_Msk)) && ((bool)(CY_IPC_CYPIPE_ENABLE)))
                {
                    REG_IPC_STRUCT_NOTIFY(ipcBase) = _VAL2FLD(IPC_STRUCT_NOTIFY_INTR_NOTIFY, (1UL << CY_IPC_INTR_CYPIPE_EP0));
                }
                Cy_Flash_RAMDelay(CY_FLASH_ERASE_ROW_DELAY);               /* Delay when erase row is finished */
                break;
            case CY_FLASH_WRITE_ROW_ERASE_BOOKMARK:
                if ((cm0s == (CPUSS_CM0_STATUS_SLEEPING_Msk | CPUSS_CM0_STATUS_SLEEPDEEP_Msk)) && ((bool)(CY_IPC_CYPIPE_ENABLE)))
                {
                    REG_IPC_STRUCT_NOTIFY(ipcBase) = _VAL2FLD(IPC_STRUCT_NOTIFY_INTR_NOTIFY, (1UL << CY_IPC_INTR_CYPIPE_EP0));
                }
                Cy_Flash_RAMDelay(CY_FLASH_WRITE_ROW_ERASE_DELAY);         /* Delay when erase phase for row is finished */
                break;
            case CY_FLASH_WRITE_ROW_PROGRAM_BOOKMARK:
                if ((cm0s == (CPUSS_CM0_STATUS_SLEEPING_Msk | CPUSS_CM0_STATUS_SLEEPDEEP_Msk)) && ((bool)(CY_IPC_CYPIPE_ENABLE)))
                {
                    REG_IPC_STRUCT_NOTIFY(ipcBase) = _VAL2FLD(IPC_STRUCT_NOTIFY_INTR_NOTIFY, (1UL << CY_IPC_INTR_CYPIPE_EP0));
                }
                Cy_Flash_RAMDelay(CY_FLASH_WRITE_ROW_PROGRAM_DELAY);
                break;
            default:
                break;
            }

            Cy_SysLib_ExitCriticalSection(intr);
        }
        #if defined (__ICCARM__)
            #pragma diag_default=Ta023
        #endif
    #endif /* (CY_CPU_CORTEX_M4) */
#endif /* !defined(CY_FLASH_RWW_DRV_SUPPORT_DISABLED) */


/*******************************************************************************
* Function Name: Cy_Flash_EraseRow
****************************************************************************//**
*
* This function erases a single row of flash. Reports success or
* a reason for failure. Does not return until the Write operation is
* complete. Returns immediately and reports a \ref CY_FLASH_DRV_IPC_BUSY error in
* the case when another process is writing to flash or erasing the row.
* User firmware should not enter the Hibernate or Deep-Sleep mode until flash Erase
* is complete. The Flash operation is allowed in Sleep mode.
* During the Flash operation, the device should not be reset, including the
* XRES pin, a software reset, and watchdog reset sources. Also, low-voltage
* detect circuits should be configured to generate an interrupt instead of a
* reset. Otherwise, portions of flash may undergo unexpected changes.
*
* \param rowAddr Address of the flash row number. The number of the flash rows
* is defined by the \ref CY_FLASH_NUMBER_ROWS macro for the selected device.
* The Read-while-Write violation occurs when the flash read operation is
* initiated in the same flash sector where the flash write operation is
* performing. Refer to the device datasheet for the details.
* Address must match row start address.
*
* \return Returns the status of the Flash operation,
* see \ref cy_en_flashdrv_status_t.
*
*******************************************************************************/
cy_en_flashdrv_status_t Cy_Flash_EraseRow(uint32_t rowAddr)
{
    cy_en_flashdrv_status_t result = CY_FLASH_DRV_INVALID_INPUT_PARAMETERS;

    /* Prepares arguments to be passed to SROM API */
    if (Cy_Flash_BoundsCheck(rowAddr) != false)
    {
        SystemCoreClockUpdate();

        flashContext.opcode = CY_FLASH_OPCODE_ERASE_ROW | CY_FLASH_BLOCKING_MODE;
        flashContext.arg1 = rowAddr;
        flashContext.arg2 = 0UL;
        flashContext.arg3 = 0UL;

        if (cy_device->flashEraseDelay)
        {
            result = Cy_Flash_SendCmd(CY_FLASH_BLOCKING_MODE, CY_FLASH_START_ERASE_DELAY);
        }
        else
        {
            result = Cy_Flash_SendCmd(CY_FLASH_BLOCKING_MODE, CY_FLASH_NO_DELAY);
        }
    }

    return (result);
}


/*******************************************************************************
* Function Name: Cy_Flash_ProgramRow
****************************************************************************//**
*
* This function writes an array of data to a single row of flash. Reports
* success or a reason for failure. Does not return until the Program operation
* is complete.
* Returns immediately and reports a \ref CY_FLASH_DRV_IPC_BUSY error in the case
* when another process is writing to flash. User firmware should not enter the
* Hibernate or Deep-sleep mode until flash Write is complete. The Flash operation
* is allowed in Sleep mode. During the Flash operation, the device should not be
* reset, including the XRES pin, a software reset, and watchdog reset sources.
* Also, low-voltage detect circuits should be configured to generate an interrupt
* instead of a reset. Otherwise, portions of flash may undergo unexpected
* changes.\n
* Before calling this function, the target flash region must be erased by
* the StartErase/EraseRow function.\n
* Data to be programmed must be located in the SRAM memory region.
* \note Before reading data from previously programmed/erased flash rows, the
* user must clear the flash cache with the Cy_SysLib_ClearFlashCacheAndBuffer()
* function.
*
* \param rowAddr Address of the flash row number. The number of the flash rows
* is defined by the \ref CY_FLASH_NUMBER_ROWS macro for the selected device.
* The Read-while-Write violation occurs when the flash read operation is
* initiated in the same flash sector where the flash write operation is
* performing. Refer to the device datasheet for the details.
* Address must match row start address.
*
* \param data The pointer to the data which has to be written to flash. The size
* of the data array must be equal to the flash row size. The flash row size for
* the selected device is defined by the \ref CY_FLASH_SIZEOF_ROW macro. Refer to
* the device datasheet for the details.
*
* \return Returns the status of the Flash operation,
* see \ref cy_en_flashdrv_status_t.
*
*******************************************************************************/
cy_en_flashdrv_status_t Cy_Flash_ProgramRow(uint32_t rowAddr, const uint32_t* data)
{
    cy_en_flashdrv_status_t result = CY_FLASH_DRV_INVALID_INPUT_PARAMETERS;

    /* Checks whether the input parameters are valid */
    if ((Cy_Flash_BoundsCheck(rowAddr) != false) && (NULL != data))
    {
        SystemCoreClockUpdate();

        /* Prepares arguments to be passed to SROM API */
        flashContext.opcode = CY_FLASH_OPCODE_PROGRAM_ROW | CY_FLASH_BLOCKING_MODE;
        flashContext.arg1   = CY_FLASH_CONFIG_DATASIZE | CY_FLASH_DATA_LOC_SRAM;
        flashContext.arg2   = rowAddr;
        flashContext.arg3   = (uint32_t)data;

        if (cy_device->flashProgramDelay)
        {
            result = Cy_Flash_SendCmd(CY_FLASH_BLOCKING_MODE, CY_FLASH_START_PROGRAM_DELAY);
        }
        else
        {
            result = Cy_Flash_SendCmd(CY_FLASH_BLOCKING_MODE, CY_FLASH_NO_DELAY);
        }
    }

    return (result);
}


/*******************************************************************************
* Function Name: Cy_Flash_WriteRow
****************************************************************************//**
*
* This function writes an array of data to a single row of flash. This is done
* in three steps - pre-program, erase and then program flash row with the input
* data. Reports success or a reason for failure. Does not return until the Write
* operation is complete.
* Returns immediately and reports a \ref CY_FLASH_DRV_IPC_BUSY error in the case
* when another process is writing to flash. User firmware should not enter the
* Hibernate or Deep-sleep mode until flash Write is complete. The Flash operation
* is allowed in Sleep mode. During the Flash operation, the
* device should not be reset, including the XRES pin, a software
* reset, and watchdog reset sources. Also, low-voltage detect
* circuits should be configured to generate an interrupt
* instead of a reset. Otherwise, portions of flash may undergo
* unexpected changes.
*
* \param rowAddr Address of the flash row number. The number of the flash rows
* is defined by the \ref CY_FLASH_NUMBER_ROWS macro for the selected device.
* The Read-while-Write violation occurs when the flash read operation is
* initiated in the same flash sector where the flash write operation is
* performing. Refer to the device datasheet for the details.
* Address must match row start address.
*
* \param data The pointer to the data which has to be written to flash. The size
* of the data array must be equal to the flash row size. The flash row size for
* the selected device is defined by the \ref CY_FLASH_SIZEOF_ROW macro. Refer to
* the device datasheet for the details.
*
* \return Returns the status of the Flash operation,
* see \ref cy_en_flashdrv_status_t.
*
*******************************************************************************/
cy_en_flashdrv_status_t Cy_Flash_WriteRow(uint32_t rowAddr, const uint32_t* data)
{
    cy_en_flashdrv_status_t result = CY_FLASH_DRV_INVALID_INPUT_PARAMETERS;

    /* Checks whether the input parameters are valid */
    if ((Cy_Flash_BoundsCheck(rowAddr) != false) && (NULL != data))
    {
        SystemCoreClockUpdate();

        /* Prepares arguments to be passed to SROM API */
        flashContext.opcode = CY_FLASH_OPCODE_WRITE_ROW | CY_FLASH_BLOCKING_MODE;
        flashContext.arg1   = 0UL;
        flashContext.arg2   = rowAddr;
        flashContext.arg3   = (uint32_t)data;

        if (cy_device->flashWriteDelay)
        {
            result = Cy_Flash_SendCmd(CY_FLASH_BLOCKING_MODE, CY_FLASH_START_WRITE_DELAY);
        }
        else
        {
            result = Cy_Flash_SendCmd(CY_FLASH_BLOCKING_MODE, CY_FLASH_NO_DELAY);
        }
    }

    return (result);
}


/*******************************************************************************
* Function Name: Cy_Flash_StartWrite
****************************************************************************//**
*
* Performs pre-program, erase and then starts programming the flash row with
* the input data. Returns immediately and reports a successful start
* or reason for failure. Reports a \ref CY_FLASH_DRV_IPC_BUSY error
* in the case when another process is writing to flash. User
* firmware should not enter the Hibernate or Deep-Sleep mode until
* flash Write is complete. The Flash operation is allowed in Sleep mode.
* During the flash operation, the device should not be reset, including the
* XRES pin, a software reset, and watchdog reset sources. Also, the low-voltage
* detect circuits should be configured to generate an interrupt instead of a reset.
* Otherwise, portions of flash may undergo unexpected changes.
* \note Before reading data from previously programmed/erased flash rows, the
* user must clear the flash cache with the Cy_SysLib_ClearFlashCacheAndBuffer()
* function.
*
* \param rowAddr Address of the flash row number. The number of the flash rows
* is defined by the \ref CY_FLASH_NUMBER_ROWS macro for the selected device.
* The Read-while-Write violation occurs when the flash read operation is
* initiated in the same flash sector where the flash write operation is
* performing. Refer to the device datasheet for the details.
* Address must match row start address.
*
* \param data The pointer to the data to be written to flash. The size
* of the data array must be equal to the flash row size. The flash row size for
* the selected device is defined by the \ref CY_FLASH_SIZEOF_ROW macro. Refer to
* the device datasheet for the details.
*
* \return Returns the status of the Flash operation,
* see \ref cy_en_flashdrv_status_t.
*
*******************************************************************************/
cy_en_flashdrv_status_t Cy_Flash_StartWrite(uint32_t rowAddr, const uint32_t* data)
{
    cy_en_flashdrv_status_t result = CY_FLASH_DRV_INVALID_INPUT_PARAMETERS;

    /* Checks whether the input parameters are valid */
    if ((Cy_Flash_BoundsCheck(rowAddr) != false) && (NULL != data))
    {
        result = Cy_Flash_StartErase(rowAddr);

        if (CY_FLASH_DRV_OPERATION_STARTED == result)
        {
            /* Polls whether the IPC is released and the Flash operation is performed */
            do
            {
                result = Cy_Flash_OperationStatus();
            }
            while (result == CY_FLASH_DRV_OPCODE_BUSY);

            if (CY_FLASH_DRV_SUCCESS == result)
            {
                result = Cy_Flash_StartProgram(rowAddr, data);
            }
        }
    }

    return (result);
}


/*******************************************************************************
* Function Name: Cy_Flash_IsOperationComplete
****************************************************************************//**
*
* Reports a successful operation result, reason of failure or busy status
* ( \ref CY_FLASH_DRV_OPCODE_BUSY ).
*
* \return Returns the status of the Flash operation (see \ref cy_en_flashdrv_status_t).
*
*******************************************************************************/
cy_en_flashdrv_status_t Cy_Flash_IsOperationComplete(void)
{
    return (Cy_Flash_OperationStatus());
}


/*******************************************************************************
* Function Name: Cy_Flash_StartErase
****************************************************************************//**
*
* Starts erasing a single row of flash. Returns immediately
* and reports a successful start or reason for failure.
* Reports a \ref CY_FLASH_DRV_IPC_BUSY error in the case when IPC structure is locked
* by another process. User firmware should not enter the Hibernate or Deep-Sleep mode until
* flash Erase is complete. The Flash operation is allowed in Sleep mode.
* During the flash operation, the device should not be reset, including the
* XRES pin, a software reset, and watchdog reset sources. Also, the low-voltage
* detect circuits should be configured to generate an interrupt instead of a reset.
* Otherwise, portions of flash may undergo unexpected changes.
* \note Before reading data from previously programmed/erased flash rows, the
* user must clear the flash cache with the Cy_SysLib_ClearFlashCacheAndBuffer()
* function.
*
* \param rowAddr Address of the flash row number. The number of the flash rows
* is defined by the \ref CY_FLASH_NUMBER_ROWS macro for the selected device.
* The Read-while-Write violation occurs when the flash read operation is
* initiated in the same flash sector where the flash erase operation is
* performing. Refer to the device datasheet for the details.
* Address must match row start address.
*
* \return Returns the status of the Flash operation,
* see \ref cy_en_flashdrv_status_t.
*
*******************************************************************************/
cy_en_flashdrv_status_t Cy_Flash_StartErase(uint32_t rowAddr)
{
    cy_en_flashdrv_status_t result = CY_FLASH_DRV_INVALID_INPUT_PARAMETERS;

    if (Cy_Flash_BoundsCheck(rowAddr) != false)
    {
        SystemCoreClockUpdate();

        /* Prepares arguments to be passed to SROM API */
        flashContext.opcode = CY_FLASH_OPCODE_ERASE_ROW;
#if (!(__CM0P_PRESENT))
        flashContext.opcode |= CY_FLASH_BLOCKING_MODE;
#endif
        flashContext.arg1 = rowAddr;
        flashContext.arg2 = 0UL;
        flashContext.arg3 = 0UL;

        if (cy_device->flashEraseDelay)
        {
            result = Cy_Flash_SendCmd(CY_FLASH_NON_BLOCKING_MODE, CY_FLASH_START_ERASE_DELAY);
        }
        else
        {
            result = Cy_Flash_SendCmd(CY_FLASH_NON_BLOCKING_MODE, CY_FLASH_NO_DELAY);
        }
    }

    return (result);
}


/*******************************************************************************
* Function Name: Cy_Flash_StartProgram
****************************************************************************//**
*
* Starts writing an array of data to a single row of flash. Returns immediately
* and reports a successful start or reason for failure.
* Reports a \ref CY_FLASH_DRV_IPC_BUSY error if another process is writing
* to flash. The user firmware should not enter Hibernate or Deep-Sleep mode until flash
* Program is complete. The Flash operation is allowed in Sleep mode.
* During the Flash operation, the device should not be reset, including the
* XRES pin, a software reset, and watchdog reset sources. Also, the low-voltage
* detect circuits should be configured to generate an interrupt instead of a reset.
* Otherwise, portions of flash may undergo unexpected changes.\n
* Before calling this function, the target flash region must be erased by
* the StartErase/EraseRow function.\n
* Data to be programmed must be located in the SRAM memory region.
* \note Before reading data from previously programmed/erased flash rows, the
* user must clear the flash cache with the Cy_SysLib_ClearFlashCacheAndBuffer()
* function.
*
* \param rowAddr The address of the flash row number. The number of the flash rows
* is defined by the \ref CY_FLASH_NUMBER_ROWS macro for the selected device.
* The Read-while-Write violation occurs when the Flash Write operation is
* performing. Refer to the device datasheet for the details.
* The address must match the row start address.
*
* \param data The pointer to the data to be written to flash. The size
* of the data array must be equal to the flash row size. The flash row size for
* the selected device is defined by the \ref CY_FLASH_SIZEOF_ROW macro. Refer to
* the device datasheet for the details.
*
* \return Returns the status of the Flash operation,
* see \ref cy_en_flashdrv_status_t.
*
*******************************************************************************/
cy_en_flashdrv_status_t Cy_Flash_StartProgram(uint32_t rowAddr, const uint32_t* data)
{
    cy_en_flashdrv_status_t result = CY_FLASH_DRV_INVALID_INPUT_PARAMETERS;

    if ((Cy_Flash_BoundsCheck(rowAddr) != false) && (NULL != data))
    {
        SystemCoreClockUpdate();

        /* Prepares arguments to be passed to SROM API */
        flashContext.opcode = CY_FLASH_OPCODE_PROGRAM_ROW;
#if (!(__CM0P_PRESENT))
        flashContext.opcode |= CY_FLASH_BLOCKING_MODE;
#endif
        flashContext.arg1   = CY_FLASH_CONFIG_DATASIZE | CY_FLASH_DATA_LOC_SRAM;
        flashContext.arg2   = rowAddr;
        flashContext.arg3   = (uint32_t)data;

        if (cy_device->flashProgramDelay)
        {
            result = Cy_Flash_SendCmd(CY_FLASH_NON_BLOCKING_MODE, CY_FLASH_START_PROGRAM_DELAY);
        }
        else
        {
            result = Cy_Flash_SendCmd(CY_FLASH_NON_BLOCKING_MODE, CY_FLASH_NO_DELAY);
        }
    }

    return (result);
}


/*******************************************************************************
* Function Name: Cy_Flash_RowChecksum
****************************************************************************//**
*
* Returns a checksum value of the specified flash row.
*
* \note Now Cy_Flash_RowChecksum() requires the row <b>address</b> (rowAddr)
*       as a parameter. In previous versions of the driver, this function used
*       the row <b>number</b> (rowNum) for this parameter.
*
* \param rowAddr The address of the flash row.
*
* \param checksumPtr The pointer to the address where checksum is to be stored
*
* \return Returns the status of the Flash operation.
*
*******************************************************************************/
cy_en_flashdrv_status_t Cy_Flash_RowChecksum (uint32_t rowAddr, uint32_t* checksumPtr)
{
    cy_en_flashdrv_status_t result = CY_FLASH_DRV_INVALID_INPUT_PARAMETERS;
    uint32_t resTmp;
    uint32_t rowID;

    /* Checks whether the input parameters are valid */
    if ((Cy_Flash_BoundsCheck(rowAddr)) && (NULL != checksumPtr))
    {
        rowID = Cy_Flash_GetRowNum(rowAddr);

        /* Prepares arguments to be passed to SROM API */
        flashContext.opcode = CY_FLASH_OPCODE_CHECKSUM |
                              (((rowID >> CY_FLASH_REGION_ID_SHIFT) & CY_FLASH_REGION_ID_MASK) << CY_FLASH_OPCODE_CHECKSUM_REGION_SHIFT) |
                              ((rowID & CY_FLASH_ROW_ID_MASK) << CY_FLASH_OPCODE_CHECKSUM_ROW_SHIFT);

        /* Tries to acquire the IPC structure and pass the arguments to SROM API */
        if (Cy_IPC_Drv_SendMsgPtr(Cy_IPC_Drv_GetIpcBaseAddress(CY_IPC_CHAN_SYSCALL), CY_FLASH_IPC_NOTIFY_STRUCT0,
                                  (void*)&flashContext) == CY_IPC_DRV_SUCCESS)
        {
            /* Polls whether IPC is released and the Flash operation is performed */
            while (Cy_IPC_Drv_IsLockAcquired(Cy_IPC_Drv_GetIpcBaseAddress(CY_IPC_CHAN_SYSCALL)) != false)
            {
                /* Wait till IPC is released */
            }

            resTmp = flashContext.opcode;

            if ((resTmp >> CY_FLASH_ERROR_SHIFT) == CY_FLASH_ERROR_NO_ERROR)
            {
                result = CY_FLASH_DRV_SUCCESS;
                *checksumPtr = flashContext.opcode & CY_FLASH_RESULT_MASK;
            }
            else
            {
                result = Cy_Flash_ProcessOpcode(flashContext.opcode);
            }
        }
        else
        {
            /* The IPC structure is already locked by another process */
            result = CY_FLASH_DRV_IPC_BUSY;
        }
    }

    return (result);
}


/*******************************************************************************
* Function Name: Cy_Flash_CalculateHash
****************************************************************************//**
*
* Returns a hash value of the specified region of flash.
*
* \param data Start the data address.
*
* \param numberOfBytes The hash value is calculated for the number of bytes after the
* start data address (0 - 1 byte, 1- 2 bytes etc).
*
* \param hashPtr The pointer to the address where hash is to be stored
*
* \return Returns the status of the Flash operation.
*
*******************************************************************************/
cy_en_flashdrv_status_t Cy_Flash_CalculateHash (const uint32_t* data, uint32_t numberOfBytes,  uint32_t* hashPtr)
{
    cy_en_flashdrv_status_t result = CY_FLASH_DRV_INVALID_INPUT_PARAMETERS;
    volatile uint32_t resTmp;

    /* Checks whether the input parameters are valid */
    if ((data != NULL) && (0ul != numberOfBytes))
    {
        /* Prepares arguments to be passed to SROM API */
        flashContext.opcode = CY_FLASH_OPCODE_HASH;
        flashContext.arg1 = (uint32_t)data;
        flashContext.arg2 = numberOfBytes;

        /* Tries to acquire the IPC structure and pass the arguments to SROM API */
        if (Cy_IPC_Drv_SendMsgPtr(Cy_IPC_Drv_GetIpcBaseAddress(CY_IPC_CHAN_SYSCALL), CY_FLASH_IPC_NOTIFY_STRUCT0,
                                  (void*)&flashContext) == CY_IPC_DRV_SUCCESS)
        {
            /* Polls whether IPC is released and the Flash operation is performed */
            while (Cy_IPC_Drv_IsLockAcquired(Cy_IPC_Drv_GetIpcBaseAddress(CY_IPC_CHAN_SYSCALL)) != false)
            {
                /* Wait till IPC is released */
            }

            resTmp = flashContext.opcode;

            if ((resTmp >> CY_FLASH_ERROR_SHIFT) == CY_FLASH_ERROR_NO_ERROR)
            {
                result = CY_FLASH_DRV_SUCCESS;
                *hashPtr = flashContext.opcode & CY_FLASH_RESULT_MASK;
            }
            else
            {
                result = Cy_Flash_ProcessOpcode(flashContext.opcode);
            }
        }
        else
        {
            /* The IPC structure is already locked by another process */
            result = CY_FLASH_DRV_IPC_BUSY;
        }
    }

    return (result);
}


/*******************************************************************************
* Function Name: Cy_Flash_GetRegion
****************************************************************************//**
*
* Returns false if Flash address is out of boundary, otherwise returns true.
*
* \param flashAddr Address to be checked
*
* \return
*   The valid return value is encoded as follows (or 0xFFFFFFFFUL for invalid address)
*   <table>
*   <tr><th>Field            <th>Value
*   <tr><td>Flash row number <td>[15:0]  bits
*   <tr><td>Flash region ID  <td>[31:16] bits
*   </table>
*
*******************************************************************************/
static uint32_t Cy_Flash_GetRowNum(uint32_t flashAddr)
{
    uint32_t result;

    if ((flashAddr >= CY_FLASH_BASE) && (flashAddr < (CY_FLASH_BASE + cy_device->flashcSize)))
    {
        result = (CY_FLASH_REGION_ID_MAIN << CY_FLASH_REGION_ID_SHIFT) |
                 ((flashAddr - CY_FLASH_BASE) / CY_FLASH_SIZEOF_ROW);
    }
    else
    if ((flashAddr >= CY_EM_EEPROM_BASE) && (flashAddr < (CY_EM_EEPROM_BASE + CY_EM_EEPROM_SIZE)))
    {
        result = (CY_FLASH_REGION_ID_EM_EEPROM << CY_FLASH_REGION_ID_SHIFT) |
                 ((flashAddr - CY_EM_EEPROM_BASE) / CY_FLASH_SIZEOF_ROW);
    }
    else
    if ((flashAddr >= SFLASH_BASE) && (flashAddr < (SFLASH_BASE + SFLASH_SECTION_SIZE)))
    {
        result = (CY_FLASH_REGION_ID_SFLASH << CY_FLASH_REGION_ID_SHIFT) |
                 ((flashAddr - SFLASH_BASE) / CY_FLASH_SIZEOF_ROW);
    }
    else
    {
        result = 0xFFFFFFFFUL;
    }

    return (result);
}


/*******************************************************************************
* Function Name: Cy_Flash_BoundsCheck
****************************************************************************//**
*
* The function checks the following conditions:
*  - if Flash address is in valid flash rows range
*  - if Flash address is equal to start address of the row
*
* \param flashAddr Address to be checked
*
* \return false - out of bound, true - in flash bounds
*
*******************************************************************************/
static bool Cy_Flash_BoundsCheck(uint32_t flashAddr)
{
    return ((Cy_Flash_GetRowNum(flashAddr) != 0xFFFFFFFFUL) && ((flashAddr % CY_FLASH_SIZEOF_ROW) == 0UL));
}


/*******************************************************************************
* Function Name: Cy_Flash_ProcessOpcode
****************************************************************************//**
*
* Converts System Call returns to the Flash driver return defines.
*
* \param opcode The value returned by the System Call.
*
* \return Flash driver return.
*
*******************************************************************************/
static cy_en_flashdrv_status_t Cy_Flash_ProcessOpcode(uint32_t opcode)
{
    cy_en_flashdrv_status_t result;

    switch (opcode)
    {
        case 0UL:
        {
            result = CY_FLASH_DRV_SUCCESS;
            break;
        }
        case CY_FLASH_ROMCODE_SUCCESS:
        {
            result = CY_FLASH_DRV_SUCCESS;
            break;
        }
        case CY_FLASH_ROMCODE_INVALID_PROTECTION:
        {
            result = CY_FLASH_DRV_INV_PROT;
            break;
        }
        case CY_FLASH_ROMCODE_INVALID_FM_PL:
        {
            result = CY_FLASH_DRV_INVALID_FM_PL;
            break;
        }
        case CY_FLASH_ROMCODE_INVALID_FLASH_ADDR:
        {
            result = CY_FLASH_DRV_INVALID_FLASH_ADDR;
            break;
        }
        case CY_FLASH_ROMCODE_ROW_PROTECTED:
        {
            result = CY_FLASH_DRV_ROW_PROTECTED;
            break;
        }
        case CY_FLASH_ROMCODE_IN_PROGRESS_NO_ERROR:
        {
            result = CY_FLASH_DRV_PROGRESS_NO_ERROR;
            break;
        }
        case (uint32_t)CY_FLASH_DRV_INVALID_INPUT_PARAMETERS:
        {
            result = CY_FLASH_DRV_INVALID_INPUT_PARAMETERS;
            break;
        }
        case CY_FLASH_IS_OPERATION_STARTED :
        {
            result = CY_FLASH_DRV_OPERATION_STARTED;
            break;
        }
        case CY_FLASH_IS_BUSY :
        {
            result = CY_FLASH_DRV_OPCODE_BUSY;
            break;
        }
        case CY_FLASH_IS_IPC_BUSY :
        {
            result = CY_FLASH_DRV_IPC_BUSY;
            break;
        }
        case CY_FLASH_IS_INVALID_INPUT_PARAMETERS :
        {
            result = CY_FLASH_DRV_INVALID_INPUT_PARAMETERS;
            break;
        }
        default:
        {
            result = CY_FLASH_DRV_ERR_UNC;
            break;
        }
    }

    return (result);
}


/*******************************************************************************
* Function Name: Cy_Flash_OperationStatus
****************************************************************************//**
*
* Checks the status of the Flash Operation, and returns it.
*
* \return Returns the status of the Flash operation
* (see \ref cy_en_flashdrv_status_t).
*
*******************************************************************************/
static cy_en_flashdrv_status_t Cy_Flash_OperationStatus(void)
{
    cy_en_flashdrv_status_t result = CY_FLASH_DRV_OPCODE_BUSY;

    /* Checks if the IPC structure is not locked */
    if (Cy_IPC_Drv_IsLockAcquired(Cy_IPC_Drv_GetIpcBaseAddress(CY_IPC_CHAN_SYSCALL)) == false)
    {
        /* The result of SROM API calling is returned to the driver context */
        result = Cy_Flash_ProcessOpcode(flashContext.opcode);

        /* Clear pre-fetch cache after flash operation */
        FLASHC->FLASH_CMD = FLASHC_FLASH_CMD_INV_Msk;

        while (FLASHC->FLASH_CMD != 0U)
        {
        }
    }

    return (result);
}


/*******************************************************************************
* Function Name: Cy_Flash_GetExternalStatus
****************************************************************************//**
*
* This function handles the case where a module such as security image captures
* a system call from this driver and reports its own status or error code,
* for example protection violation. In that case, a function from this
* driver returns an unknown error (see \ref cy_en_flashdrv_status_t). After receipt
* of an unknown error, the user may call this function to get the status
* of the capturing module.
*
* The user is responsible for parsing the content of the returned value
* and casting it to the appropriate enumeration.
*
* \return
* The error code that was stored in the opcode variable.
*
*******************************************************************************/
uint32_t Cy_Flash_GetExternalStatus(void)
{
    return (flashContext.opcode);
}


/* [] END OF FILE */
