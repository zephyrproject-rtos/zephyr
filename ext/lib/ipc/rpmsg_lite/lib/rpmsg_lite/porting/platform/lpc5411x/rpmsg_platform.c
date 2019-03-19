/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <string.h>

#include "rpmsg_platform.h"
#include "rpmsg_env.h"

#include "fsl_device_registers.h"
#include "fsl_mailbox.h"

#if defined(RL_USE_MCMGR_IPC_ISR_HANDLER) && (RL_USE_MCMGR_IPC_ISR_HANDLER == 1)
#include "mcmgr.h"
#endif

#if defined(RL_USE_ENVIRONMENT_CONTEXT) && (RL_USE_ENVIRONMENT_CONTEXT == 1)
#error "This RPMsg-Lite port requires RL_USE_ENVIRONMENT_CONTEXT set to 0"
#endif

static int isr_counter = 0;
static int disable_counter = 0;
static void *platform_lock;

#if defined(RL_USE_MCMGR_IPC_ISR_HANDLER) && (RL_USE_MCMGR_IPC_ISR_HANDLER == 1)
static void mcmgr_event_handler(uint16_t vring_idx, void *context)
{
    env_isr(vring_idx);
}
#else
void MAILBOX_IRQHandler(void)
{
    mailbox_cpu_id_t cpu_id;
#if defined(FSL_FEATURE_MAILBOX_SIDE_A)
    cpu_id = kMAILBOX_CM4;
#else
    cpu_id = kMAILBOX_CM0Plus;
#endif

    uint32_t value = MAILBOX_GetValue(MAILBOX, cpu_id);

    if (value & 0x01)
    {
        MAILBOX_ClearValueBits(MAILBOX, cpu_id, 0x01);
        env_isr(0);
    }
    if (value & 0x02)
    {
        MAILBOX_ClearValueBits(MAILBOX, cpu_id, 0x02);
        env_isr(1);
    }

    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
      exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}
#endif

void platform_global_isr_disable(void)
{
    __asm volatile("cpsid i");
}


void platform_global_isr_enable(void)
{
    __asm volatile("cpsie i");
}

int platform_init_interrupt(unsigned int vector_id, void *isr_data)
{
    /* Register ISR to environment layer */
    env_register_isr(vector_id, isr_data);

    env_lock_mutex(platform_lock);

    RL_ASSERT(0 <= isr_counter);
    if (!isr_counter)
    {
#if defined(FSL_FEATURE_MAILBOX_SIDE_A)
        NVIC_SetPriority(MAILBOX_IRQn, 5);
#else
        NVIC_SetPriority(MAILBOX_IRQn, 2);
#endif
    }
    isr_counter++;

    env_unlock_mutex(platform_lock);

    return 0;
}

int platform_deinit_interrupt(unsigned int vector_id)
{
    /* Prepare the MU Hardware */
    env_lock_mutex(platform_lock);

    RL_ASSERT(0 < isr_counter);
    isr_counter--;
    if (!isr_counter)
    {
        NVIC_DisableIRQ(MAILBOX_IRQn);
    }

    /* Unregister ISR from environment layer */
    env_unregister_isr(vector_id);

    env_unlock_mutex(platform_lock);

    return 0;
}

void platform_notify(unsigned int vector_id)
{
#if defined(RL_USE_MCMGR_IPC_ISR_HANDLER) && (RL_USE_MCMGR_IPC_ISR_HANDLER == 1)
    env_lock_mutex(platform_lock);
    MCMGR_TriggerEventForce(kMCMGR_RemoteRPMsgEvent, RL_GET_Q_ID(vector_id));
    env_unlock_mutex(platform_lock);
#else
/* Only single RPMsg-Lite instance (LINK_ID) is defined for this dual core device. Extend
   this statement in case multiple instances of RPMsg-Lite are needed. */
    switch (RL_GET_LINK_ID(vector_id))
    {
        case RL_PLATFORM_LPC5411x_M4_M0_LINK_ID:
            env_lock_mutex(platform_lock);
/* Write directly into the Mailbox register, no need to wait until the content is cleared
   (consumed by the receiver side) because the same walue of the virtqueu ID is written
   into this register when trigerring the ISR for the receiver side. The whole queue of
   received buffers for associated virtqueue is handled in the ISR then. */
#if defined(FSL_FEATURE_MAILBOX_SIDE_A)
            MAILBOX_SetValueBits(MAILBOX, kMAILBOX_CM0Plus, (1 << RL_GET_Q_ID(vector_id)));
#else
            MAILBOX_SetValueBits(MAILBOX, kMAILBOX_CM4, (1 << RL_GET_Q_ID(vector_id)));
#endif
            env_unlock_mutex(platform_lock);
            return;

        default:
            return;
    }
#endif
}

/**
 * platform_time_delay
 *
 * @param num_msec Delay time in ms.
 *
 * This is not an accurate delay, it ensures at least num_msec passed when return.
 */
void platform_time_delay(int num_msec)
{
    uint32_t loop;

    /* Recalculate the CPU frequency */
    SystemCoreClockUpdate();

    /* Calculate the CPU loops to delay, each loop has 3 cycles */
    loop = SystemCoreClock / 3 / 1000 * num_msec;

    /* There's some difference among toolchains, 3 or 4 cycles each loop */
    while (loop)
    {
        __NOP();
        loop--;
    }
}

/**
 * platform_in_isr
 *
 * Return whether CPU is processing IRQ
 *
 * @return True for IRQ, false otherwise.
 *
 */
int platform_in_isr(void)
{
    return ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0);
}

/**
 * platform_interrupt_enable
 *
 * Enable peripheral-related interrupt
 *
 * @param vector_id Virtual vector ID that needs to be converted to IRQ number
 *
 * @return vector_id Return value is never checked.
 *
 */
int platform_interrupt_enable(unsigned int vector_id)
{
    RL_ASSERT(0 < disable_counter);

    platform_global_isr_disable();
    disable_counter--;

    if (!disable_counter)
    {
        NVIC_EnableIRQ(MAILBOX_IRQn);
    }
    platform_global_isr_enable();
    return (vector_id);
}

/**
 * platform_interrupt_disable
 *
 * Disable peripheral-related interrupt.
 *
 * @param vector_id Virtual vector ID that needs to be converted to IRQ number
 *
 * @return vector_id Return value is never checked.
 *
 */
int platform_interrupt_disable(unsigned int vector_id)
{
    RL_ASSERT(0 <= disable_counter);

    platform_global_isr_disable();
    /* virtqueues use the same NVIC vector
       if counter is set - the interrupts are disabled */
    if (!disable_counter)
    {
        NVIC_DisableIRQ(MAILBOX_IRQn);
    }
    disable_counter++;
    platform_global_isr_enable();
    return (vector_id);
}

/**
 * platform_map_mem_region
 *
 * Dummy implementation
 *
 */
void platform_map_mem_region(unsigned int vrt_addr, unsigned int phy_addr, unsigned int size, unsigned int flags)
{
}

/**
 * platform_cache_all_flush_invalidate
 *
 * Dummy implementation
 *
 */
void platform_cache_all_flush_invalidate(void)
{
}

/**
 * platform_cache_disable
 *
 * Dummy implementation
 *
 */
void platform_cache_disable(void)
{
}

/**
 * platform_vatopa
 *
 * Dummy implementation
 *
 */
unsigned long platform_vatopa(void *addr)
{
    return ((unsigned long)addr);
}

/**
 * platform_patova
 *
 * Dummy implementation
 *
 */
void *platform_patova(unsigned long addr)
{
    return ((void *)addr);
}

/**
 * platform_init
 *
 * platform/environment init
 */
int platform_init(void)
{
#if defined(RL_USE_MCMGR_IPC_ISR_HANDLER) && (RL_USE_MCMGR_IPC_ISR_HANDLER == 1)
    mcmgr_status_t retval;
    retval = MCMGR_RegisterEvent(kMCMGR_RemoteRPMsgEvent, mcmgr_event_handler, NULL);
    if(kStatus_MCMGR_Success != retval)
    {
        return -1;
    }
#else
    MAILBOX_Init(MAILBOX);
#endif

    /* Create lock used in multi-instanced RPMsg */
    if(0 != env_create_mutex(&platform_lock, 1))
    {
        return -1;
    }

    return 0;
}

/**
 * platform_deinit
 *
 * platform/environment deinit process
 */
int platform_deinit(void)
{
/* Important for LPC5411x - do not deinit mailbox, if there
   is a pending ISR on the other core! */
#if defined(FSL_FEATURE_MAILBOX_SIDE_A)
    while (0 != MAILBOX_GetValue(MAILBOX, kMAILBOX_CM0Plus))
    {
    }
#else
    while (0 != MAILBOX_GetValue(MAILBOX, kMAILBOX_CM4))
    {
    }
#endif

    MAILBOX_Deinit(MAILBOX);

    /* Delete lock used in multi-instanced RPMsg */
    env_delete_mutex(platform_lock);
    platform_lock = NULL;
    return 0;
}
