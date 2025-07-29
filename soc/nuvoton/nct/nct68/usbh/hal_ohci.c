/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT   nuvoton_nct_usbh

#include <errno.h>

#include <zephyr/types.h>
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbh_nct, LOG_LEVEL_NONE);

extern void USBH_IRQHandler(void);


/* Device configuration */
struct usbh_nct_config {
    struct usbh_reg *const base;    /* USBH controller base address */
    uint8_t irq;                    /* USBH controller irq */
};

/* Get default value in dts */
static const struct usbh_nct_config usbh_cfg = {
    .base = (struct usbh_reg *)DT_INST_REG_ADDR(0),
    .irq = DT_INST_IRQN(0),
};

#define USBH    (usbh_cfg.base)
#define IRQN    (usbh_cfg.irq)

/* HcControl */
void halUSBH_Set_HcControl(uint32_t val)
{
    USBH->HcControl = val;
}
uint32_t halUSBH_Get_HcControl(void)
{
    return USBH->HcControl;
}

/* HcCommandStatus */
void halUSBH_Set_HcCommandStatus(uint32_t val)
{
    USBH->HcCommandStatus = val;
}
uint32_t halUSBH_Get_HcCommandStatus(void)
{
    return USBH->HcCommandStatus;
}

/* HcInterruptStatus */
void halUSBH_Set_HcInterruptStatus(uint32_t mask)
{
    USBH->HcInterruptStatus = mask;
}
uint32_t halUSBH_Get_HcInterruptStatus(void)
{
    return USBH->HcInterruptStatus;
}

/* HcInterruptEnable */
void halUSBH_Set_HcInterruptEnable(uint32_t mask)
{
    USBH->HcInterruptEnable = mask;
}

/* HcInterruptDisable */
void halUSBH_Set_HcInterruptDisable(uint32_t mask)
{
    USBH->HcInterruptDisable = mask;
}

/* HcHCCA */
void halUSBH_Set_HcHCCA(uint32_t hcca)
{
    USBH->HcHCCA = hcca;
}

/* HcControlHeadED */
void halUSBH_Set_HcControlHeadED(uint32_t ed)
{
    USBH->HcControlHeadED = ed;
}

/* HcControlCurrentED */
void halUSBH_Set_HcControlCurrentED(uint32_t ed)
{
    USBH->HcControlCurrentED = ed;
}

/* HcBulkHeadED */
void halUSBH_Set_HcBulkHeadED(uint32_t ed)
{
    USBH->HcBulkHeadED = ed;
}

/* HcBulkCurrentED */
void halUSBH_Set_HcBulkCurrentED(uint32_t ed)
{
    USBH->HcBulkCurrentED = ed;
}

/* HcFmInterval */
void halUSBH_Set_HcFmInterval(uint32_t val)
{
    USBH->HcFmInterval = val;
}

/* HcFmNumber */
uint32_t halUSBH_Get_HcFmNumber(void)
{
    return USBH->HcFmNumber;
}

/* HcPeriodicStart */
void halUSBH_Set_HcPeriodicStart(uint32_t val)
{
    USBH->HcPeriodicStart = val;
}

/* HcLSThreshold */
void halUSBH_Set_HcLSThreshold(uint32_t val)
{
    USBH->HcLSThreshold = val;
}

/* HcRhDescriptorA */
uint32_t halUSBH_Get_HcRhDescriptorA(void)
{
    return USBH->HcRhDescriptorA;
}

uint32_t halUSBH_Get_PowerOnToPowerGoodTime(void)
{
    return (USBH->HcRhDescriptorA >> 24);
}

/* HcRhDescriptorB */
uint32_t halUSBH_Get_HcRhDescriptorB(void)
{
    return USBH->HcRhDescriptorB;
}

/* HcRhStatus */
void halUSBH_Set_HcRhStatus(uint32_t val)
{
    USBH->HcRhStatus = val;
}
uint32_t halUSBH_Get_HcRhStatus(void)
{
    return USBH->HcRhStatus;
}

/* HcRhPortStatus */
void halUSBH_Set_HcRhPortStatus(uint16_t wIndex, uint32_t val)
{
    USBH->HcRhPortStatus[wIndex] = val;
}
uint32_t halUSBH_Get_HcRhPortStatus(uint16_t wIndex)
{
    return USBH->HcRhPortStatus[wIndex];
}

/*-----------------------------------------------------------------------------------
 *
 *-----------------------------------------------------------------------------------*/
void halUSBH_Open(void)
{
    USBH->HcConfig4 |= (BIT(NCT_HcConfig4_DMPULLDOWN) | BIT(NCT_HcConfig4_DPPULLDOWN));
    irq_enable(IRQN);
}

void halUSBH_Close(void)
{
    irq_disable(IRQN);
    USBH->HcRhStatus = BIT(NCT_HcRhStatus_LPS);
}


void halUSBH_SuspendAllRhPort(void)
{
    if (USBH->HcRhPortStatus[0] & 0x1) {
        USBH->HcRhPortStatus[0] = 0x4;
    }
    if (USBH->HcRhPortStatus[1] & 0x1) {
        USBH->HcRhPortStatus[1] = 0x4;
    }
}

void halUSBH_SuspendHostControl(void)
{
    SET_FIELD(USBH->HcControl, NCT_HcControl_HCFS_FIELD, 3);
}

void halUSBH_ResumeAllRhPort(void)
{
    if (USBH->HcRhPortStatus[0] & 0x4) {
        USBH->HcRhPortStatus[0] = 0x8;
    }
    if (USBH->HcRhPortStatus[1] & 0x4) {
        USBH->HcRhPortStatus[1] = 0x8;
    }
}

void halUSBH_ResumeHostControl(void)
{
    SET_FIELD(USBH->HcControl, NCT_HcControl_HCFS_FIELD, 1);
    SET_FIELD(USBH->HcControl, NCT_HcControl_HCFS_FIELD, 2);
}

void halUSBH_RemoteWkup_EN()
{
    USBH->HcRhStatus |= BIT(NCT_HcRhStatus_DRWE);
    USBH->HcInterruptEnable = BIT(NCT_HcInterruptEnable_RHSC) | BIT(NCT_HcInterruptEnable_RD);
}

void halUSBH_InitialSetting(void)
{
    uint32_t fminterval;
    fminterval = 0x2edf;    /* 11,999 */
    /* periodic start 90% of frame interval */
    halUSBH_Set_HcPeriodicStart((fminterval * 9) / 10);
    /* set FSLargestDataPacket, 10,104 for 0x2edf frame interval */
    fminterval |= ((((fminterval - 210) * 6) / 7) << 16);
    halUSBH_Set_HcFmInterval(fminterval);
    halUSBH_Set_HcLSThreshold(0x628);
}

void halUSBH_RootHubInit(void)
{
    USBH->HcRhDescriptorA = (USBH->HcRhDescriptorA | BIT(NCT_HcRhDescriptorA_NPS)) & ~BIT(NCT_HcRhDescriptorA_PSM);
    USBH->HcRhStatus = BIT(NCT_HcRhStatus_LPSC);
}

static int usbh_nct_init(const struct device *dev)
{
    struct scfg_reg *inst_scfg = (struct scfg_reg *)DT_REG_ADDR_BY_NAME(DT_NODELABEL(scfg), scfg);

    LOG_DBG("Device name: %s", dev->name);

    /* usb host power on */
    inst_scfg->DEV_CTL4 |= BIT(NCT_DEV_CTL4_USB_PWRDN);

    /* irq */
    IRQ_CONNECT(DT_INST_IRQN(0),
            DT_INST_IRQ(0, priority),
            USBH_IRQHandler, NULL, 0);

    return 0;
}

#define NCT_USBH_INIT(inst)                        \
                                                    \
    DEVICE_DT_INST_DEFINE(inst,                     \
                  usbh_nct_init,                   \
                  NULL,                             \
                  NULL, &usbh_cfg,                  \
                  PRE_KERNEL_2,                     \
                  CONFIG_USBH_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NCT_USBH_INIT)

