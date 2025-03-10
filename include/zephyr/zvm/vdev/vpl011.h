/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Yuhao Hu, Qingqiao Wang and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZVM_VDEV_VPL011_H_
#define ZEPHYR_INCLUDE_ZVM_VDEV_VPL011_H_

#include <zephyr/arch/arm64/sys_io.h>
#include <zephyr/drivers/uart.h>

#define VSERIAL_REG_BASE DT_REG_ADDR(DT_INST(0, arm_pl011))
#define VSERIAL_REG_SIZE DT_REG_SIZE(DT_INST(0, arm_pl011))
#define VSERIAL_HIRQ_NUM DT_IRQN(DT_INST(0, arm_pl011))

#define ARM_PL011_ID {0x11, 0x10, 0x14, 0x00, 0x0d, 0xf0, 0x05, 0xb1}
#define PL011_INT_TX			0x20
#define PL011_INT_RX			0x10
#define PL011_FLAG_TXFE			0x80
#define PL011_FLAG_RXFF			0x40
#define PL011_FLAG_TXFF			0x20
#define PL011_FLAG_RXFE			0x10

/* Regs Write/Read 32/64 Op*/
#define vserial_sysreg_read32(base, offset)	\
	sys_read32((unsigned long)(base+offset / 4))
#define vserial_sysreg_write32(data, base, offset)	\
	sys_write32(data, (unsigned long)(base+(offset / 4)))
#define vserial_sysreg_read64(base, offset)	\
	sys_read64(((unsigned long))(base+(offset / 4)))
#define vserial_sysreg_write64(data, base, offset)	\
	sys_write64(data, (unsigned long)(base+(offset / 4)))


/*
 * VUART PL011 register map structure
 */
struct vpl011_regs_ctxt {
	uint32_t dr;			/* base + 0x00 , 0*/
	union {					/* base + 0x04 , 1*/
		uint32_t rsr;
		uint32_t ecr;
	};
	uint32_t reserved_0[4];	/* base + 0x08 , 2 ~ 5*/
	uint32_t fr;			/* base + 0x18 , 6*/
	uint32_t reserved_1;	/* base + 0x1c , 7*/
	uint32_t ilpr;			/* base + 0x20 , 8*/
	uint32_t ibrd;			/* base + 0x24 , 9*/
	uint32_t fbrd;			/* base + 0x28 , 10*/
	uint32_t lcr_h;			/* base + 0x2c , 11*/
	uint32_t cr;			/* base + 0x30 , 12*/
	uint32_t ifls;			/* base + 0x34 , 13*/
	uint32_t imsc;			/* base + 0x38 , 14*/
	uint32_t ris;			/* base + 0x3c , 15*/
	uint32_t mis;			/* base + 0x40 , 16*/
	uint32_t icr;			/* base + 0x44 , 17*/
	uint32_t dmacr;			/* base + 0x48 , 18*/
	uint8_t id[8];			/* base + 0xfe0 */
};

#define FIFO_SIZE 16

#define VPL011_BIT_MASK(x, y) (((2 << x) - 1) << y)
/* VPL011 Uart Flags Register */
#define VPL011_FR_CTS		BIT(0)	/* clear to send - inverted */
#define VPL011_FR_DSR		BIT(1)	/* data set ready - inverted */
#define VPL011_FR_DCD		BIT(2)	/* data carrier detect - inverted */
#define VPL011_FR_BUSY		BIT(3)	/* busy transmitting data */
#define VPL011_FR_RXFE		BIT(4)	/* receive FIFO empty */
#define VPL011_FR_TXFF		BIT(5)	/* transmit FIFO full */
#define VPL011_FR_RXFF		BIT(6)	/* receive FIFO full */
#define VPL011_FR_TXFE		BIT(7)	/* transmit FIFO empty */
#define VPL011_FR_RI		BIT(8)	/* ring indicator - inverted */

#define VPL011_INT_RX			0x10
#define VPL011_INT_TX			0x20
/* VPL011 Interrupt Mask Set/Clear Register */
#define VPL011_IMSC_RIMIM	BIT(0)	/* RTR modem interrupt mask */
#define VPL011_IMSC_CTSMIM	BIT(1)	/* CTS modem interrupt mask */
#define VPL011_IMSC_DCDMIM	BIT(2)	/* DCD modem interrupt mask */
#define VPL011_IMSC_DSRMIM	BIT(3)	/* DSR modem interrupt mask */
#define VPL011_IMSC_RXIM		BIT(4)	/* receive interrupt mask */
#define VPL011_IMSC_TXIM		BIT(5)	/* transmit interrupt mask */
#define VPL011_IMSC_RTIM		BIT(6)	/* receive timeout interrupt mask */
#define VPL011_IMSC_FEIM		BIT(7)	/* framine error interrupt mask */
#define VPL011_IMSC_PEIM		BIT(8)	/* parity error interrupt mask */
#define VPL011_IMSC_BEIM		BIT(9)	/* break error interrupt mask */
#define VPL011_IMSC_OEIM		BIT(10)	/* overrun error interrutpt mask */

#define VPL011_PRIV(vdev) \
	((struct virt_pl011 *)(vdev)->priv_vdev)
#define VDEV_REGS(vdev) \
	((volatile struct vpl011_regs_ctxt  *)(VPL011_PRIV(vdev))->vserial_reg_base)
#define VPL011_REGS(vpl011) \
	((volatile struct vpl011_regs_ctxt  *)(vpl011)->vserial_reg_base)

struct virt_pl011 {

	/* virtual serial for emulating device for vm. */
	uint32_t *vserial_reg_base;

	/**
	 * serial address base and size which
	 * are used to locate vdev access from
	 * vm.
	 */
	uint32_t vserial_base;
	uint32_t vserial_size;

	struct z_vm *vm;
	struct k_fifo rx_fifo;
	struct k_spinlock vserial_lock;

	uint32_t irq;
	uint32_t enabled;
	uint32_t level;
	uint32_t set_irq;
	uint32_t count;

	bool connecting;
	void *vserial;
};

#endif /* ZEPHYR_INCLUDE_ZVM_VDEV_VPL011_H_ */
