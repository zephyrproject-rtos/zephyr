/*
 * Copyright (c) 2024 Microchip Technology, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <soc.h>
#include <mec_qspi_api.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app);

#include "crc8.h"
#include "espi_hc_emu.h"

/* #define ESPI_EMU_DEBUG_QSPI */
#define ESPI_EMU_DEBUG_QSPI_WITH_PIN

#define ESPI_CFG_DEV_ID			0x4
#define ESPI_CFG_CAP			0x8
#define ESPI_CFG_CH0_CAP		0x10
#define ESPI_CFG_CH1_CAP		0x20
#define ESPI_CFG_CH2_CAP		0x30
#define ESPI_CFG_CH3_CAP		0x40
#define ESPI_CFG_PLT_0			0x800

/* eSPI Cycle types */
#define ESPI_CYCLE_PC_MEM_RD32_NP	0u /* Fig. 36 */
#define ESPI_CYCLE_PC_MEM_WR32		0x01u /* Fig. 34 */
#define ESPI_CYCLE_PC_MEM_RD64_NP	0x02u /* Fig. 36 */
#define ESPI_CYCLE_PC_MEM_WR64		0x03u /* Fig. 34 */
#define ESPI_CYCLE_PC_MSG_REQ		0x10u /* b[3:1] = r2:r1:r0 Fig. 38 */
#define ESPI_CYCLE_PC_MSG_REQ_WITH_DATA	0x11u /* b[3:1] = r2:r1:r0 Fig. 38 */
#define ESPI_CYCLE_PC_COMPL		0x06u /* Fig. 39 */
#define ESPI_CYCLE_PC_COMPL_WITH_DATA	0x09u /* b[2:1] = P1:P0 Fig. 39 */
#define ESPI_CYCLE_PC_NO_COMPL		0x08u /* b[2:1] = P1:P0 Fig. 39 */
/* OOB */
#define ESPI_CYCLE_OOB_MSG		0x20 /* Fig. 45 */
/* Flash */
#define ESPI_CYCLE_FC_RD		0x00 /* Fig. 48 */

/* eSPI command opcodes
 * put a posted or completion header and optional data
 * put a non-posted header and optional data
 * get posted or completion header and optional data
 * get non-posted header and optional data
 * put a short 1 byte non-posted I/O Read packet
 * put a short 2 byte non-posted I/O Read packet
 * put a short 4 byte non-posted I/O Read packet
 * put a short 1 byte non-posted I/O Write packet
 * put a short 2 byte non-posted I/O Write packet
 * put a short 4 byte non-posted I/O Write packet
 * put a short 1 byte non-posted Memory Read packet
 * put a short 2 byte non-posted Memory Read packet
 * put a short 4 byte non-posted Memory Read packet
 * put a short 1 byte non-posted Memory Write packet
 * put a short 2 byte non-posted Memory Write packet
 * put a short 4 byte non-posted Memory Write packet
 */
#define ESPI_OPCODE_PUT_PC		0x00u
#define ESPI_OPCODE_PUT_NP		0x02u
#define ESPI_OPCODE_GET_PC		0x01u
#define ESPI_OPCODE_GET_NP		0x03u
#define ESPI_OPCODE_PUT_IORD_SHORT_1	0x40u
#define ESPI_OPCODE_PUT_IORD_SHORT_2	0x41u
#define ESPI_OPCODE_PUT_IORD_SHORT_4	0x43u
#define ESPI_OPCODE_PUT_IOWR_SHORT_1	0x44u
#define ESPI_OPCODE_PUT_IOWR_SHORT_2	0x45u
#define ESPI_OPCODE_PUT_IOWR_SHORT_4	0x47u
#define ESPI_OPCODE_PUT_MRD32_SHORT_1	0x48u
#define ESPI_OPCODE_PUT_MRD32_SHORT_2	0x49u
#define ESPI_OPCODE_PUT_MRD32_SHORT_4	0x4bu
#define ESPI_OPCODE_PUT_MWR32_SHORT_1	0x4cu
#define ESPI_OPCODE_PUT_MWR32_SHORT_2	0x4du
#define ESPI_OPCODE_PUT_MWR32_SHORT_4	0x4fu
/* VWire */
#define ESPI_OPCODE_PUT_VWIRE		0x04u /* put a tunneled vw packet. Fig. 41 */
#define ESPI_OPCODE_GET_VWIRE		0x05u /* get a tunneled vw packet. Fig. 42 */
/* OOB */
#define ESPI_OPCODE_PUT_OOB		0x06u /* Put an OOB message. Refer to Table 6 */
#define ESPI_OPCODE_GET_OOB		0x07u /* Get an OOB message. */
/* Flash channel */
#define ESPI_OPCODE_PUT_FLASH_COMPL	0x08u /* Put a flash access completion */
#define ESPI_OPCODE_GET_FLASH_NP	0x09u /* Get non-posted flash access request. Table 6 */
#define ESPI_OPCODE_PUT_FLASH_NP	0x0au /* Put a non-posted flash access request */
#define ESPI_OPCODE_GET_FLASH_COMPL	0x0bu /* Get flash access completion */
/* channel independent */
#define ESPI_OPCODE_GET_STATUS		0x25u /* Read status from Target */
#define ESPI_OPCODE_SET_CONFIG		0x22u /* Set capabilities */
#define ESPI_OPCODE_GET_CONFIG		0x21u /* Get capabilities */
#define ESPI_OPCODE_RESET		0xffu /* in-band RESET command */

/* response opcode byte
 * b[3:0] = response code
 * b[5:4] = reserved = 00b
 * b[7:6] = response modifier
 */
#define ESPI_RESP_CODE_RESP_MSK			0x0fu
#define ESPI_RESP_CODE_RESP_POS			0
#define ESPI_RESP_CODE_RESP_OK			0x08
#define ESPI_RESP_CODE_RESP_DEFER		0x01
#define ESPI_RESP_CODE_RESP_NON_FATAL_ERR	0x02
#define ESPI_RESP_CODE_RESP_FATAL_ERR		0x03
#define ESPI_RESP_CODE_RESP_WAIT		0x0f
/* if all 8-bits of response code are 1 it is interpreted as no target present */
#define ESPI_RESP_CODE_NO_RESP			0xff

#define ESPI_RESP_CODE_MOD_POS  6
#define ESPI_RESP_CODE_MOD_MSK  0xc0u

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

#define ZUSER_GPIO_ESPI_RESET_N_IDX	0
#define ZUSER_GPIO_ESPI_IO1_IDX		1
#define ZUSER_GPIO_ESPI_ALERT_N_IDX	2
#define ZUSER_GPIO_VCC_PWRGD_OUT_IDX	3
#define ZUSER_GPIO_TREADY_N_IDX		4

const struct gpio_dt_spec espi_reset_n_dt =
	GPIO_DT_SPEC_GET_BY_IDX(ZEPHYR_USER_NODE, espi_gpios, 0);

/* eSPI specification allows for Single Controller - Single Target configuration
 * only, IO[1] can be used to signal an Alert event. We configure eSPI
 * for this method of alert instead of using the dedicated alert pin.
 */
const struct gpio_dt_spec espi_io1_dt =
	GPIO_DT_SPEC_GET_BY_IDX(ZEPHYR_USER_NODE, espi_gpios, 1);

const struct gpio_dt_spec trig_gpio_dt =
	GPIO_DT_SPEC_GET_BY_IDX(ZEPHYR_USER_NODE, espi_gpios, 2);

const struct gpio_dt_spec vcc_pwrgd_gpio_dt =
	GPIO_DT_SPEC_GET_BY_IDX(ZEPHYR_USER_NODE, espi_gpios, 3);

const struct gpio_dt_spec target_ready_n_dt =
	GPIO_DT_SPEC_GET_BY_IDX(ZEPHYR_USER_NODE, espi_gpios, 4);

/* Controller-to-Target Virtual Wire initialization table */
const struct espi_hc_vw ctvw_init[] = {
	{ /* Host Index 02h b[0]=nSLP_S3, b[1]=nSLP_S4, b[2]=nSLP_S5, b[3]=rsvd */
		.host_index = 0x02u,
		.mask = 0x07u, /* implemented mask */
		.reset_val = 0u,
		.levels = 0u,
		.valid = 0u,
	},
	{ /* Host Index 03h b[0]=nSUS_STAT, b[1]=nPLTRST, b[2]=OOB_RST_WARN, b[3]=rsvd */
		.host_index = 0x03u,
		.mask = 0x07u,
		.reset_val = 0u,
		.levels = 0u,
		.valid = 0u,
	},
	{ /* Host Index 07h b[0]=HOST_RST_WARN, b[1]=nSMIOUT, b[2]=nSMIOUT, b[3]=rsvd */
		.host_index = 0x7u,
		.mask = 0x07u,
		.reset_val = 0x06u,
		.levels = 0x06u,
		.valid = 0u,
	},
	{ /* Host Index 41h b[0]=nSUS_WARN, b[1]=SUS_PWRDN_ACK, b[2]=rsvd, b[3=]nSLP_A */
		.host_index = 0x41u,
		.mask = 0x0bu,
		.reset_val = 0u,
		.levels = 0u,
		.valid = 0u,
	},
	{ /* Host Index 42h b[0]=nSLP_LAN, b[1]=nSLP_WLAN, b[3:2]=rsvd */
		.host_index = 0x42u,
		.mask = 0x03u,
		.reset_val = 0u,
		.levels = 0u,
		.valid = 0u,
	},
	{ /* Host Index 43h, b[3:0] = Host-to-Target Generic [3:0] */
		.host_index = 0x43u,
		.mask = 0x0fu,
		.reset_val = 0u,
		.levels = 0u,
		.valid = 0u,
	},
	{ /* Host Index 44h, b[3:0] = Host-to-Target Generic [7:4] */
		.host_index = 0x44u,
		.mask = 0x0fu,
		.reset_val = 0u,
		.levels = 0u,
		.valid = 0u,
	},
	{ /* Host Index 47h b[0]=HOST_C10, b[3:1]=rsvd */
		.host_index = 0x47u,
		.mask = 0x01u,
		.reset_val = 0u,
		.levels = 0u,
		.valid = 0u,
	},
	{ /* Host Index 4Ah b[0]=rsvd, b[1]=DNX_WARN, b[3:2]=rsvd */
		.host_index = 0x4au,
		.mask = 0x02u,
		.reset_val = 0u,
		.levels = 0u,
		.valid = 0u,
	},
};

/* Controller-to-Target Virtual Wire initialization table */
const struct espi_hc_vw tcvw_init[] = {
	{ /* Host Index 00h, Serial IRQ */
		.host_index = 0x00u,
		.mask = 0x0fu,
		.reset_val = 0u,
		.levels = 0u, /* b[7]=0(low),1(high), b[6:0]=IRQ 0-127 */
		.valid = 0u,
	},
	{ /* Host Index 01h, Serial IRQ */
		.host_index = 0x01u,
		.mask = 0x0fu,
		.reset_val = 0u,
		.levels = 0u, /* b[7]=0(low),1(high), b[6:0]=IRQ 128-255 */
		.valid = 0u,
	},
	{ /* Host Index 04h b[0]=OOB_RST_ACK, b[1]=rsvd, b[2]=nWAKE, b[3]=nPME */
		.host_index = 0x04u,
		.mask = 0x0du,
		.reset_val = 0x0cu,
		.levels = 0xcu, /* b[3:0]=current vwire(s) state */
		.valid = 0u,
	},
	{ /* Host Index 05h b[0]=TARG_BOOT_LOAD_DONE, b[1]=ERR_FATAL,
	   * b[2]=ERR_NONFATAL, b[3]=TARG_BOOT_LOAD_STATUS
	   */
		.host_index = 0x05u,
		.mask = 0x0fu,
		.reset_val = 0u,
		.levels = 0u,
		.valid = 0u,
	},
	{ /* Host Index 06h b[0]=nSCI, b[1]=nSMI, b[2]=nRCIN, b[3]=HOST_RST_ACK */
		.host_index = 0x6u,
		.mask = 0xfu,
		.reset_val = 0x7u,
		.levels = 0u,
		.valid = 0u,
	},
	{ /* Host Index 40h b[0]=nSUSACK, b[1]=DNX_ACK, b[2]=rsvd, b[3]=rsvd */
		.host_index = 0x40u,
		.mask = 0x03u,
		.reset_val = 0u,
		.levels = 0u,
		.valid = 0u,
	},
	{ /* Host Index 45h b[3:1]=Target-to-Host Generic b[3:1] */
		.host_index = 0x45u,
		.mask = 0x0eu,
		.reset_val = 0u,
		.levels = 0u,
		.valid = 0u,
	},
	{ /* Host Index 46h, b[3:0] = Target-to-Host Generic [7:4] */
		.host_index = 0x46u,
		.mask = 0x0fu,
		.reset_val = 0u,
		.levels = 0u,
		.valid = 0u,
	},
	{ /* Host Index 50h, b[3:0] = Target-GPIO [3:0] */
		.host_index = 0x50u,
		.mask = 0x0fu,
		.reset_val = 0u,
		.levels = 0u,
		.valid = 0u,
	},
	{ /* Host Index 51h, b[3:0] = Target-GPIO [7:4] */
		.host_index = 0x51u,
		.mask = 0x0fu,
		.reset_val = 0u,
		.levels = 0u,
		.valid = 0u,
	},
	{ /* Host Index 52h, b[3:0] = Target-GPIO [11:8] */
		.host_index = 0x52u,
		.mask = 0x0fu,
		.reset_val = 0u,
		.levels = 0u,
		.valid = 0u,
	},
	{ /* Host Index 53h, b[3:0] = Target-GPIO [15:12] */
		.host_index = 0x53u,
		.mask = 0x0fu,
		.reset_val = 0u,
		.levels = 0u,
		.valid = 0u,
	},
	{ /* Host Index 54h, b[3:0] = Target-GPIO [19:16] */
		.host_index = 0x54u,
		.mask = 0x0fu,
		.reset_val = 0u,
		.levels = 0u,
		.valid = 0u,
	},
};

/* eSPI packet PC Memory Write 32
 * byte offset    description
 * 0              cycle type  Begin Header
 * 1              b[7:4]=Tag, b[3:0]=Len[11:8]
 * 2              Len[7:0] Len is 1-based. 0=4KB, Note: complex rules
 * 3              Addr[31:24]
 * 4              Addr[23:16]
 * 5              Addr[15:8]
 * 6              Addr[7:0]   End Header
 * 7              data byte 0
 * 8              data byte 1
 * ...
 * n+7            data byte n
 *
 * PC Short Mem Write 32
 * 0              Addr[31:24]
 * 1              Addr[28:16]
 * 2              Addr[15:8]
 * 3              Addr[7:0]
 * 4              data byte 0
 * 5              data byte 1
 * ...
 * n+4            data byte n (n = 0, 1, 3)
 *
 * Link Layer adds command opcode and CRC-8 to packet
 * poly: x^8 + x^2 + x + 1
 * poly coeff: 0x07
 * seed value: 0x00
 * Example PC Memory Read 32
 * byte offset    description
 * 0              command opcode
 * 1              cycle type
 * 2              b[7:4]=Tag, b[3:0]=Len[11:8]
 * 3              Len[7:0]
 * 4              Addr[31:24]
 * 5              Addr[24:16]
 * 6              Addr[15:8]
 * 7              Addr[7:0]
 * 8              CRC-8
 * TAR 2 clocks with I/O tri-stated
 * 0              response opcode
 * 1              cycle type
 * 2              b[7:4]=Tag, b[3:0]=Len[11:8]
 * 3              Len[7:0]
 * 4              data byte 0
 * 5              data byte 1
 * 6              data byte 2
 * 7              data byte 3
 * 8              Status[7:0]
 * 9              Status[15:8]
 * 10             CRC
 */

/* QSPI0 used to emulate eSPI link protocol.
 * Issue 1: Boot-ROM
 * We do not want the Boot-ROM to load from eSPI MAF because it will
 * configure eSPI and enter an infinite wait loop for the eSPI Host
 * to configure the system. We set the BSS_STRAP to force Boot-ROM to
 * load from Shared SPI. The Boot-ROM will configure Shared SPI and
 * send commands over it. If we fly-wire SHD_SPI pins to ESPI pins there
 * should not be an issue if Boot-ROM does not enable eSPI and its pins.
 * On Boot-ROM load fail, Boot-ROM will put SHD_SPI pins back to default
 * state (disabled). Next, Zephyr application will turn on SHD_SPI pins
 * when the QSPI driver loads.  We can work-around this by disabling
 * the QSPI0 zephyr driver in Device Tree.
 *
 * Issue 2: pin voltages
 * MEC5 ESPI_xxxx pins are on the VTR3 voltage rail. VTR3 is
 * connected to +1.8V on the EVB.
 * MEC5 QSPI0 Shared SPI port pins are on the VTR2 voltage rail.
 * The EVB has jumpers to select +3.3V or +1.8V for VTR2.
 * MEC5 QSPI0 Private SPI port pins are on the VTR1 voltage rail.
 * VTR1 is always +3.3V.
 * Option 1: Use SHD_SPI
 *   Disable Zephyr QSPI0 driver.
 *   Configure EVB jumpers to use +1.8V for VTR2
 *   Fly-wire SHD_SPI pins on EVB J34 to EVB J45 ESPI pins on +1.8V
 *   Connect logic analyzer to level translated ESPI pins on J52 (+3.3V)
 *
 * Option 2: Use PVT_SPI
 *   VTR2 can stay at default jumper settings for +3.3V
 *   Fly-wire PVT_SPI pins (+3.3V) on EVB J34 to EVB J52 (+3.3V)
 *   Connect logic analyzer to +1.8V ESPI pins on J45
 *
 * We will try Option 2 first. Leave VTR2 at +3.3V because there are
 * many other pins on VTR2: I2C01, I2C07, PWM0, PWM1, etc.
 */
int espi_hc_emu_init(uint32_t freqhz)
{
	struct qspi_regs *qr = (struct qspi_regs *)DT_REG_ADDR(DT_NODELABEL(qspi0));
	int ret;

	if (freqhz > MHZ(48)) {
		return -EINVAL;
	}

	ret = gpio_pin_configure_dt(&trig_gpio_dt, GPIO_OUTPUT_HIGH);
	if (ret) {
		LOG_ERR("Configure debug trigger GPIO and drive high error (%d)", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&espi_reset_n_dt, GPIO_OUTPUT_LOW);
	if (ret) {
		LOG_ERR("Configure eSPI Reset GPIO and drive low error (%d)", ret);
		return ret;
	}

	/* Initialize emulated VCC_PWRGD GPIO to output low */
	ret = gpio_pin_configure_dt(&vcc_pwrgd_gpio_dt, GPIO_OUTPUT_LOW);
	if (ret) {
		LOG_ERR("Configure emulated VCC_PWRGD and drive low error (%d)", ret);
		return ret;
	}

	/* Initialize TARGET_nREADY as GPIO input */
	ret = gpio_pin_configure_dt(&target_ready_n_dt, GPIO_INPUT);
	if (ret) {
		LOG_ERR("Configure TARGET_nREADY GPIO as input error (%d)", ret);
		return ret;
	}

	/* Use QSPI SHD_nCS1 because SHD_nCS0 is also a strap and has a pull-down on the EVB */
	ret = mec_qspi_init(qr, MHZ(4), MEC_SPI_SIGNAL_MODE_0,
			    MEC_QSPI_IO_FULL_DUPLEX, MEC_QSPI_CS_1);
	if (ret != MEC_RET_OK) {
		return -EIO;
	}

	return 0;
}

/* TODO arm eSPI IO[1] pin as alert.
 * Pin interrupt should only be enable when eSPI_nCS is de-asserted.
 * espi_io1_dt
 */
int espi_hc_alert_detection_enable(int enable)
{
	int ret;
	gpio_flags_t flags;

	if (enable) {
		flags = GPIO_INT_EDGE_FALLING; /* OR GPIO_INT_LEVEL_LOW */
	} else {
		flags = GPIO_INT_DISABLE;
	}

	ret = gpio_pin_interrupt_configure_dt(&espi_io1_dt, flags);

	return ret;
}

int espi_hc_emu_espi_reset_n(uint8_t level)
{
	int ret;

	if (level) {
		ret = gpio_pin_set_dt(&espi_reset_n_dt, 1);
	} else {
		ret = gpio_pin_set_dt(&espi_reset_n_dt, 0);
	}

	return ret;
}

int espi_hc_is_alert(void)
{
	int pin_level =	gpio_pin_get_dt(&espi_io1_dt);

	if (pin_level == 0) {
		return 1;
	}

	return 0;
}

int espi_hc_emu_vcc_pwrgd(uint8_t level)
{
	int ret;

	if (level) {
		ret = gpio_pin_set_dt(&vcc_pwrgd_gpio_dt, 1);
	} else {
		ret = gpio_pin_set_dt(&vcc_pwrgd_gpio_dt, 0);
	}

	return ret;
}

int espi_hc_emu_is_target_ready(void)
{
	int pin_level =	gpio_pin_get_dt(&target_ready_n_dt);

	if (pin_level == 0) {
		return 1;
	}

	return 0;
}

#ifdef ESPI_EMU_DEBUG_QSPI
static void pr_qspi_regs(struct qspi_regs *qr)
{
	LOG_INF("QSPI.Mode = 0x%08x", qr->MODE);
	LOG_INF("QSPI.Ctrl = 0x%08x", qr->CTRL);
	LOG_INF("QSPI.Status = 0x%08x", qr->STATUS);
	LOG_INF("QSPI.Intr_en = 0x%08x", qr->INTR_CTRL);
	for (uint32_t n = 0; n < 4u; n++) {
		LOG_INF("QSPI.Descr[%u] = 0x%08x", n, qr->DESCR[n]);
	}
	LOG_INF("QSPI.LDMA_RXEN = 0x%08x", qr->LDMA_RXEN);
	LOG_INF("QSPI.LDMA_TXEN = 0x%08x", qr->LDMA_TXEN);
	LOG_INF("QSPI.RX_LDMA_CHAN[0].CTRL = 0x%08x", qr->RX_LDMA_CHAN[0].CTRL);
	LOG_INF("QSPI.RX_LDMA_CHAN[0].MEM_START = 0x%08x", qr->RX_LDMA_CHAN[0].MEM_START);
	LOG_INF("QSPI,RX_LDMA_CHAN[0].LEN = 0x%08x", qr->RX_LDMA_CHAN[0].LEN);
	LOG_INF("QSPI.TX_LDMA_CHAN[0].CTRL = 0x%08x", qr->TX_LDMA_CHAN[0].CTRL);
	LOG_INF("QSPI.TX_LDMA_CHAN[0].MEM_START = 0x%08x", qr->TX_LDMA_CHAN[0].MEM_START);
	LOG_INF("QSPI,TX_LDMA_CHAN[0].LEN = 0x%08x", qr->TX_LDMA_CHAN[0].LEN);
}
#else
static void pr_qspi_regs(struct qspi_regs *qr) { }
#endif

static int qspi_is_hw_error(uint32_t status)
{
	if (status & (BIT(QSPI_STATUS_TXBERR_Pos)
		      | BIT(QSPI_STATUS_RXBERR_Pos)
		      | BIT(QSPI_STATUS_PROGERR_Pos)
		      | BIT(QSPI_STATUS_LDRXERR_Pos)
		      | BIT(QSPI_STATUS_LDTXERR_Pos))) {
		return -EIO;
	}

	return 0;
}

/* de-assert ESPI_nCS */
int espi_hc_close(void)
{
	struct qspi_regs *qr = (struct qspi_regs *)DT_REG_ADDR(DT_NODELABEL(qspi0));
	uint32_t status = 0;

	status = qr->STATUS;
	if (status & BIT(QSPI_STATUS_ACTIVE_Pos)) {
		/* de-assert QSPI chip select */
		qr->EXE = BIT(QSPI_EXE_STOP_Pos);
		status = qr->STATUS;
		while (status & BIT(QSPI_STATUS_ACTIVE_Pos)) {
			status = qr->STATUS;
		}
		return 0;
	}

	return 0;
}

int espi_hc_fd_tx_fifo(const uint8_t *msg, size_t msglen, uint32_t flags)
{
	struct qspi_regs *qr = (struct qspi_regs *)DT_REG_ADDR(DT_NODELABEL(qspi0));
	const uint8_t *pdata = msg;
	size_t ntx = msglen;
	uint32_t ctrl = 0, status = 0;

	if (!msg || !msglen || (msglen > 0x7fffu)) {
		LOG_ERR("eSPI HC TX: invalid params");
		return -EINVAL;
	}

	status = qr->STATUS;
	if (flags & BIT(0)) {
		if (status & BIT(QSPI_STATUS_ACTIVE_Pos)) {
			LOG_ERR("eSPI HC TX: HW busy error: 0x%08x", status);
			return -EIO;
		}
		qr->INTR_CTRL = 0;
		qr->RX_LDMA_CHAN[0].CTRL = 0;
		qr->TX_LDMA_CHAN[0].CTRL = 0;
		qr->MODE &= ~(BIT(QSPI_MODE_RX_LDMA_Pos) | BIT(QSPI_MODE_TX_LDMA_Pos));
		qr->LDMA_RXEN = 0;
		qr->LDMA_TXEN = 0;
		qr->EXE = BIT(QSPI_EXE_CLRF_Pos);
	} else if (qspi_is_hw_error(status)) {
		LOG_ERR("eSPI HC TX: HW errors: 0x%08x", status);
		return -EIO;
	}

	qr->STATUS = UINT32_MAX;
	ctrl = ((QSPI_CTRL_IFM_FD << QSPI_CTRL_IFM_Pos) | (QSPI_CTRL_TXM_EN << QSPI_CTRL_TXM_Pos)
		| (QSPI_CTRL_QUNITS_1B << QSPI_CTRL_QUNITS_Pos));
	ctrl |= ((msglen & 0x7fffu) << QSPI_CTRL_QNUNITS_Pos);
	if (flags & BIT(1)) {
		ctrl |= BIT(QSPI_CTRL_CLOSE_Pos);
	}
	qr->CTRL = ctrl; /* write all bits at once */

	/* preload TX FIFO up to MEC5_QSPI_FIFO_LEN */
	while (ntx) {
		status = qr->STATUS;
		if (status & BIT(QSPI_STATUS_TXBF_Pos)) {
			break;
		}
		sys_write8(*pdata, (mem_addr_t)&qr->TX_FIFO);
		pdata++;
		ntx--;
	}

	/* Start QSPI */
	qr->EXE = BIT(QSPI_EXE_START_Pos);

	while (ntx) {
		status = qr->STATUS;
		if (!(status & BIT(QSPI_STATUS_TXBF_Pos))) {
			sys_write8(*pdata, (mem_addr_t)&qr->TX_FIFO);
			pdata++;
			ntx--;
		}
	}

#ifdef ESPI_EMU_DEBUG_QSPI_WITH_PIN
	gpio_pin_set_dt(&trig_gpio_dt, 0);
#endif
	/* wait for last byte to go out on the wire(s)
	 * QSPI has no flags for this case. If CLOSE
	 * was set then the DONE bit would indicate CS#
	 * de-asserted. Use hard coded delay.
	 * We need HW status indicating the QSPI TX shift
	 * register has emptied!
	 */
	status = qr->STATUS;
	while (!(status & BIT(QSPI_STATUS_TXBE_Pos))) {
		status = qr->STATUS;
	}

#ifdef ESPI_EMU_DEBUG_QSPI_WITH_PIN
	gpio_pin_set_dt(&trig_gpio_dt, 1);
	gpio_pin_set_dt(&trig_gpio_dt, 0);
#endif

	/* TX FIFO empty, one byte being shifted out. Is there any
	 * status in QSPI we can check?
	 */
	status = qr->STATUS;
	while (!(status & BIT(QSPI_STATUS_DONE_Pos))) {
		status = qr->STATUS;
	}

#ifdef ESPI_EMU_DEBUG_QSPI_WITH_PIN
	gpio_pin_set_dt(&trig_gpio_dt, 1);
#endif

	return 0;
}

/* Generate 2 clocks with I/O lines tri-stated in full-duplex mode */
int espi_hc_fd_tar(void)
{
	struct qspi_regs *qr = (struct qspi_regs *)DT_REG_ADDR(DT_NODELABEL(qspi0));
	uint32_t ctrl = 0, status = 0;

	status = qr->STATUS;
	if (!(status & BIT(QSPI_STATUS_ACTIVE_Pos))) {
		LOG_ERR("eSPI HC TX: QSPI not open! 0x%08x", status);
		return -EIO;
	}

	qr->STATUS = UINT32_MAX;
	ctrl = ((QSPI_CTRL_IFM_FD << QSPI_CTRL_IFM_Pos)
		| (QSPI_CTRL_TXM_DIS << QSPI_CTRL_TXM_Pos)
		| (QSPI_CTRL_QUNITS_BITS << QSPI_CTRL_QUNITS_Pos)
		| (2u << QSPI_CTRL_QNUNITS_Pos));
	qr->CTRL = ctrl; /* write all bits at once */

	/* Start QSPI */
	qr->EXE = BIT(QSPI_EXE_START_Pos);

	status = qr->STATUS;
	while (!(status & BIT(QSPI_STATUS_DONE_Pos))) {
		status = qr->STATUS;
	}

	return qspi_is_hw_error(status);
}

int espi_hc_fd_rx_fifo(uint8_t *msg, size_t msglen, uint32_t flags)
{
	struct qspi_regs *qr = (struct qspi_regs *)DT_REG_ADDR(DT_NODELABEL(qspi0));
	uint8_t *pdata = msg;
	size_t nrx = msglen;
	uint32_t ctrl = 0, status = 0;

	if (!msg || !msglen || (msglen > 0x7fffu)) {
		LOG_ERR("eSPI HC RX: invalid params");
		return -EINVAL;
	}

	status = qr->STATUS;
	if (flags & BIT(0)) {
		if (status & BIT(QSPI_STATUS_ACTIVE_Pos)) {
			LOG_ERR("eSPI HC RX: HW busy error: 0x%08x", status);
			return -EIO;
		}
		qr->INTR_CTRL = 0;
		qr->RX_LDMA_CHAN[0].CTRL = 0;
		qr->TX_LDMA_CHAN[0].CTRL = 0;
		qr->MODE &= ~(BIT(QSPI_MODE_RX_LDMA_Pos) | BIT(QSPI_MODE_TX_LDMA_Pos));
		qr->LDMA_RXEN = 0;
		qr->LDMA_TXEN = 0;
		qr->EXE = BIT(QSPI_EXE_CLRF_Pos);
	} else if (qspi_is_hw_error(status)) {
		LOG_ERR("eSPI HC RX: HW errors: 0x%08x", status);
		return -EIO;
	}

	qr->STATUS = UINT32_MAX;
	ctrl = ((QSPI_CTRL_IFM_FD << QSPI_CTRL_IFM_Pos) | BIT(QSPI_CTRL_RXEN_Pos)
		| (QSPI_CTRL_QUNITS_1B << QSPI_CTRL_QUNITS_Pos));
	ctrl |= ((msglen & 0x7fffu) << QSPI_CTRL_QNUNITS_Pos);
	if (flags & BIT(1)) {
		ctrl |= BIT(QSPI_CTRL_CLOSE_Pos);
	}
	qr->CTRL = ctrl; /* write all bits at once */

	/* Start QSPI */
	qr->EXE = BIT(QSPI_EXE_START_Pos);

	while (nrx) {
		status = qr->STATUS;
		if (status & BIT(QSPI_STATUS_RXBE_Pos)) {
			continue;
		}
		*pdata = sys_read8((mem_addr_t)&qr->RX_FIFO);
		pdata++;
		nrx--;
	}

	status = qr->STATUS;
	while (!(status & BIT(QSPI_STATUS_DONE_Pos))) {
		status = qr->STATUS;
	}

	return 0;
}

/* Transmit message, receive response per eSPI protocol for simple messages.
 * Transmit message full-duplex.
 *   Descriptor TX LDMA mode
 *   TX data enabled
 *   RX disabled
 *   units = bytes
 *   number of units = msglen
 * TAR of 2 clocks with I/O lines tri-stated.
 * !!! TODO: eSPI spec. states first clock of TAR drive line high
 * !!!  second clock of TAR tri-state pins.
 * !!! Will external pull-ups on I/O lines work to pull lines high during
 * !!! first clock?
 *  Descriptor FIFO mode.
 *  TX and RX disabled. Results in I/O lines tri-stated.
 *  QSPI units = bits, number of units = 2 (2 clocks in full duplex)
 *  Do not write data to TX FIFO or read from RX FIFO.
 * Receive resplen bytes and store in response buffer.
 *  Descriptor FIFO mode.
 *  TX disabled
 *  RX enabled
 *  units = bytes
 *  number of units = resplen
 */
int espi_hc_emu_xfr(const uint8_t *msg, size_t msglen, uint8_t *response, size_t resplen)
{
	struct qspi_regs *qr = (struct qspi_regs *)DT_REG_ADDR(DT_NODELABEL(qspi0));
	uint32_t descr = 0, status = 0;

	if (!msg || !response || !msglen || !resplen) {
		LOG_ERR("eSPI HC emuXfr: invalid params");
		return -EINVAL;
	}

	status = qr->STATUS;
	if (status & BIT(QSPI_STATUS_ACTIVE_Pos)) {
		LOG_ERR("eSPI HC emuXfr: HW busy error: 0x%08x", status);
		return -EIO;
	}

	qr->INTR_CTRL = 0;
	qr->RX_LDMA_CHAN[0].CTRL = 0;
	qr->TX_LDMA_CHAN[0].CTRL = 0;
	qr->MODE &= ~(BIT(QSPI_MODE_RX_LDMA_Pos) | BIT(QSPI_MODE_TX_LDMA_Pos));
	qr->LDMA_RXEN = 0;
	qr->LDMA_TXEN = 0;
	qr->CTRL = (BIT(QSPI_CTRL_DESCR_MODE_Pos)
		    | (QSPI_CTRL_IFM_FD << QSPI_CTRL_IFM_Pos)
		    | (QSPI_CTRL_DPTR_DESCR0 << QSPI_CTRL_DPTR_Pos));
	qr->EXE = BIT(QSPI_EXE_CLRF_Pos);
	qr->STATUS = UINT32_MAX;

	/* build message transmit descriptor */
	descr = ((QSPI_DESCR_IFM_FD << QSPI_DESCR_IFM_Pos)
		 | (QSPI_DESCR_TXEN_EN << QSPI_DESCR_TXEN_Pos)
		 | (QSPI_DESCR_TXDMA_1B_LDMA_CH0 << QSPI_DESCR_TXDMA_Pos)
		 | (QSPI_DESCR_QUNITS_1B << QSPI_DESCR_QUNITS_Pos)
		 | (QSPI_DESCR_NEXT_DESCR1 << QSPI_DESCR_NEXT_Pos)
		 | ((msglen << QSPI_DESCR_QNUNITS_Pos) & QSPI_DESCR_QNUNITS_Msk));
	qr->DESCR[0] = descr;

	/* build TAR descriptor. Generate 2 clocks with tri-stated I/O lines */
	descr = ((QSPI_DESCR_IFM_FD << QSPI_DESCR_IFM_Pos)
		 | (QSPI_DESCR_QUNITS_BITS << QSPI_DESCR_QUNITS_Pos)
		 | (QSPI_DESCR_NEXT_DESCR2 << QSPI_DESCR_NEXT_Pos)
		 | ((2u << QSPI_DESCR_QNUNITS_Pos) & QSPI_DESCR_QNUNITS_Msk));
	qr->DESCR[1] = descr;

	/* build receive response descriptor */
	descr = ((QSPI_DESCR_IFM_FD << QSPI_DESCR_IFM_Pos)
		 | BIT(QSPI_DESCR_RXEN_Pos)
		 | (QSPI_DESCR_RXDMA_1B_LDMA_CH0 << QSPI_DESCR_RXDMA_Pos)
		 | (QSPI_DESCR_QUNITS_1B << QSPI_DESCR_QUNITS_Pos)
		 | (QSPI_DESCR_NEXT_DESCR3 << QSPI_DESCR_NEXT_Pos)
		 | ((resplen << QSPI_DESCR_QNUNITS_Pos) & QSPI_DESCR_QNUNITS_Msk)
		 | BIT(QSPI_DESCR_CLOSE_Pos) | BIT(QSPI_DESCR_LAST_Pos));
	qr->DESCR[2] = descr;

	/* Local DMA TX channel 0 to load msg into TX FIFO */
	qr->TX_LDMA_CHAN[0].LEN = msglen;
	qr->TX_LDMA_CHAN[0].MEM_START = (uint32_t)msg;
	qr->TX_LDMA_CHAN[0].CTRL =
		((QSPI_LDMA_CHAN_CTRL_ACCSZ_1B << QSPI_LDMA_CHAN_CTRL_ACCSZ_Pos)
		 | BIT(QSPI_LDMA_CHAN_CTRL_EN_Pos) | BIT(QSPI_LDMA_CHAN_CTRL_INCRA_Pos));

	/* Local DMA RX channel 0 to read response from RX FIFO and store in resp */
	qr->RX_LDMA_CHAN[0].LEN = resplen;
	qr->RX_LDMA_CHAN[0].MEM_START = (uint32_t)response;
	qr->RX_LDMA_CHAN[0].CTRL =
		((QSPI_LDMA_CHAN_CTRL_ACCSZ_1B << QSPI_LDMA_CHAN_CTRL_ACCSZ_Pos)
		 | BIT(QSPI_LDMA_CHAN_CTRL_EN_Pos) | BIT(QSPI_LDMA_CHAN_CTRL_INCRA_Pos));

	/* Set Local DMA TX enable for descr[0] and set Local DMA RX enable for descr[2] */
	qr->LDMA_RXEN = BIT(2);
	qr->LDMA_TXEN = BIT(0);

	/* Enable TX and RX Local DMA in Mode register */
	qr->MODE |= (BIT(QSPI_MODE_RX_LDMA_Pos) | BIT(QSPI_MODE_TX_LDMA_Pos));

	pr_qspi_regs(qr);

	/* Start QSPI */
	qr->EXE = BIT(QSPI_EXE_START_Pos);

	/* Wait for Done */
	status = qr->STATUS;
	while (!(status & BIT(QSPI_STATUS_DONE_Pos))) {
		status = qr->STATUS;
	}

	/* Check for errors */
	if (status & (BIT(QSPI_STATUS_TXBERR_Pos)
		      | BIT(QSPI_STATUS_RXBERR_Pos)
		      | BIT(QSPI_STATUS_PROGERR_Pos)
		      | BIT(QSPI_STATUS_LDRXERR_Pos)
		      | BIT(QSPI_STATUS_LDTXERR_Pos))) {
		LOG_ERR("eSPI HC emuXfr HW error: 0x%08x", status);
		return -EIO;
	}

	pr_qspi_regs(qr);

	return 0;
}

static void init_vw(struct espi_hc_vw *vw, size_t max_vw,
		    const struct espi_hc_vw *init_vw, size_t max_init_vw)
{
	for (size_t n = 0; n < max_init_vw; n++) {
		if (n >= max_vw) {
			break;
		}

		struct espi_hc_vw *v = &vw[n];
		const struct espi_hc_vw *vinit = &init_vw[n];

		v->host_index = vinit->host_index;
		v->mask = vinit->mask;
		v->reset_val = vinit->reset_val;
		v->levels = vinit->levels;
		v->valid = vinit->valid;
	}
}

int espi_hc_ctx_emu_init(struct espi_hc_context *hc)
{
	size_t n;

	if (!hc) {
		return -EINVAL;
	}

	memset(hc, 0, sizeof(struct espi_hc_context));

	for (n = 0; n < ESPI_MAX_CT_VW_GROUPS; n++) {
		hc->c2t_vw[n].host_index = 0xffu;
	}

	for (n = 0; n < ESPI_MAX_TC_VW_GROUPS; n++) {
		hc->t2c_vw[n].host_index = 0xffu;
	}

	init_vw(hc->c2t_vw, ESPI_MAX_CT_VW_GROUPS, ctvw_init, ESPI_MAX_CT_VW_GROUPS);
	init_vw(hc->t2c_vw, ESPI_MAX_TC_VW_GROUPS, tcvw_init, ESPI_MAX_TC_VW_GROUPS);

	return 0;
}

/* Enable eSPI Host controller and link by de-asserting eSPI_RESET */
int espi_hc_ctx_emu_ctrl(struct espi_hc_context *hc, uint8_t enable)
{
	int ret = 0;
	uint16_t config_offset = 0;

	if (!hc) {
		return -EINVAL;
	}

	LOG_INF("eSPI HC CTX EMU Control");

	/* memset(hc, 0, sizeof(struct espi_hc_context)); */

	if (!enable) { /* disable eSPI Target by asserting ESPI_nRESET */
		LOG_INF("eSPI HC emu: Assert ESPI_nRESET");
		ret = espi_hc_emu_espi_reset_n(0);
		if (ret) {
			LOG_ERR("eSPI HC emu: nESPI_RESET drive low error");
			return ret;
		}
		return 0;
	}

	LOG_INF("eSPI HC emu: De-assert ESPI_nRESET");
	ret = espi_hc_emu_espi_reset_n(1);
	if (ret) {
		LOG_ERR("eSPI HC emu: nESPI_RESET drive high error");
		return ret;
	}

	LOG_INF("eSPI HC emu: nESPI_RESET de-asserted");

	config_offset = ESPI_GET_CONFIG_DEV_ID;
	ret = espi_hc_emu_get_config(hc, config_offset, &hc->version_id, &hc->pkt_status);
	if (ret) {
		LOG_ERR("eSPI GET_CONFIG[%u] after release of nESPI_RESET failed (%d)",
			config_offset, ret);
	}

	config_offset = ESPI_GET_CONFIG_GLB_CAP;
	ret = espi_hc_emu_get_config(hc, config_offset, &hc->global_cap_cfg, &hc->pkt_status);
	if (ret) {
		LOG_ERR("eSPI GET_CONFIG[%u] after release of nESPI_RESET failed (%d)",
			config_offset, ret);
	}

	config_offset = ESPI_GET_CONFIG_PC_CAP;
	ret = espi_hc_emu_get_config(hc, config_offset, &hc->pc_cap_cfg, &hc->pkt_status);
	if (ret) {
		LOG_ERR("eSPI GET_CONFIG[%u] after release of nESPI_RESET failed (%d)",
			config_offset, ret);
	}

	config_offset = ESPI_GET_CONFIG_VW_CAP;
	ret = espi_hc_emu_get_config(hc, config_offset, &hc->vw_cap_cfg, &hc->pkt_status);
	if (ret) {
		LOG_ERR("eSPI GET_CONFIG[%u] after release of nESPI_RESET failed (%d)",
			config_offset, ret);
	}

	config_offset = ESPI_GET_CONFIG_OOB_CAP;
	ret = espi_hc_emu_get_config(hc, config_offset, &hc->oob_cap_cfg, &hc->pkt_status);
	if (ret) {
		LOG_ERR("eSPI GET_CONFIG[%u] after release of nESPI_RESET failed (%d)",
			config_offset, ret);
	}

	config_offset = ESPI_GET_CONFIG_FC_CAP;
	ret = espi_hc_emu_get_config(hc, config_offset, &hc->fc_cap_cfg, &hc->pkt_status);
	if (ret) {
		LOG_ERR("eSPI GET_CONFIG[%u] after release of nESPI_RESET failed (%d)",
			config_offset, ret);
	}

	hc->vwg_cnt_max = ((hc->vw_cap_cfg >> 8) & 0x3fu) + 1u;

	return ret;
}

/* eSPI GET_STATUS
 * byte offset    description
 * 0              0x25 get status opcode
 * 1              CRC-8
 * TAR            2 eSPI clocks with I/O tri-stated
 * 0              response byte. b[7:6]=00b no exended response appended.
 *                                      01b PC completion appended
 *                                      10b VW completion appended
 *                                      11b FC completion appended
 *                               b[5:4]=00b reserved
 *                               b[3:0]=response code
 * 1              status b[7:0]
 * 2              status [b15:8]
 * 3              CRC-8
 *
 * NOTE: MCHP eSPI Target does not support GET_STATUS appended response format.
 *       This format appends a channel completion data.
 * Response with VW completion appended
 * 0              response byte. b[7:6]=10b, b[5:4]=00b, b[3:0]=response code
 * 1              VW group count - maximum is 0x40 (64).
 * 2              VW group[0] index
 * 3              VW group[0] data
 * 4              VW group[1] index
 * 5              VW group[1] data
 * ...
 *                VW group[group count - 1] index
 *                VW group[group count - 1] data
 *                status b[7:0]
 *                status b[15:8]
 *                CRC-8
 * maximum response pkt byte length = 1 + 1 + (64 * 2) + 2 + 1 = 133
 *
 * Response with PC completion appended
 * 0 response byte: b[7:6]=01b, b[5:4]=00b, b[3:0]=response code
 * 1 HDR
 * ? Data
 * ? status b[7:0]
 * ? status b[15:8]
 * ? CRC-8
 *
 */
int espi_hc_emu_get_status(struct espi_hc_context *hc, uint16_t *espi_status)
{
	int ret = 0;
	size_t rlen = 4;
	uint8_t msg[4] = {0};
	uint16_t status = 0xffffu;
	uint8_t crc8 = 0, crc8_pkt = 0;

	LOG_INF("eSPI HC EMU GET_STATUS");

	if (!hc || !espi_status) {
		return -EINVAL;
	}

	struct espi_resp_pkt *ersp = &hc->ersp;
	uint8_t *buf = ersp->buf;

	msg[0] = ESPI_OPCODE_GET_STATUS;
	crc8 = crc8_init();
	crc8 = crc8_update(crc8, (const void *)msg, 1u);
	crc8 = crc8_finalize(crc8);

	/* MCHP eSPI Target does not support GET_STATUS appended response packets */
	ret = espi_hc_emu_xfr((const uint8_t *)msg, 2u, buf, rlen);
	if (ret) {
		return ret;
	}

	if (buf[0] & 0xc0) {
		LOG_ERR("GET_STATUS has appended response bits set! 0x%02x", buf[0]);
	}

	crc8 = crc8_init();
	crc8 = crc8_update(crc8, (const void *)buf, rlen - 1u);
	crc8 = crc8_finalize(crc8);

	crc8_pkt = buf[rlen - 1u];
	if (crc8 != crc8_pkt) {
		LOG_ERR("GET_RESP: CRC error: expected=0x%02x calc=0x%02x", crc8, crc8_pkt);
		return -EIO;
	}

	status = buf[rlen - 2u];
	status <<= 8;
	status |= buf[rlen - 3u];
	*espi_status = status;

	LOG_INF("eSPI HC EMU GET_STATUS OK: 0x%04x", status);

	return 0;
}

/* Transmit eSPI GET_CONFIGURATION command packet and receive response packet
 * Message offset    description
 * 0                 0x21 get config
 * 1                 addr[15:8]
 * 2                 addr[7:0]
 * 3                 CRC-8
 * TAR               2 eSPI clocks with I/O lines tri-stated
 * 0                 response code
 * 1                 data[7:0]
 * 2                 data[15:8]
 * 3                 data[23:16]
 * 4                 data[31:24]
 * 5                 status[7:0]
 * 6                 status[15:8]
 * 7                 CRC-8
 */
int espi_hc_emu_get_config(struct espi_hc_context *hc, uint16_t config_offset,
			   uint32_t *config, uint16_t *cmd_status)
{
	uint32_t cfg = 0u;
	int ret = 0;
	uint16_t status = 0u;
	uint8_t rsp_crc8 = 0;

	LOG_INF("eSPI HC emu: GET_CONFIG 0x%04x", config_offset);

	if (!hc || !config || !cmd_status) {
		return -EINVAL;
	}

	struct espi_msg_pkt *emsg = &hc->emsg;
	struct espi_resp_pkt *ersp = &hc->ersp;

	memset(emsg, 0, sizeof(struct espi_msg_pkt));
	memset(ersp, 0, sizeof(struct espi_resp_pkt));

	emsg->buf[0] = ESPI_OPCODE_GET_CONFIG;
	emsg->buf[1] = (uint8_t)((config_offset >> 8) & 0xffu);
	emsg->buf[2] = (uint8_t)(config_offset & 0xffu);

	emsg->crc8 = crc8_init();
	emsg->crc8 = crc8_update(emsg->crc8, (const void *)emsg->buf, 3u);
	emsg->crc8 = crc8_finalize(emsg->crc8);
	emsg->buf[3] = emsg->crc8;

	ret = espi_hc_emu_xfr((const uint8_t *)emsg->buf, 4u, ersp->buf, 8u);
	if (ret) {
		return ret;
	}

	rsp_crc8 = crc8_init();
	rsp_crc8 = crc8_update(rsp_crc8, (const void *)ersp->buf, 7u);
	rsp_crc8 = crc8_finalize(rsp_crc8);

	if (rsp_crc8 != ersp->buf[7]) {
		LOG_ERR("GET_CONFIGURATION: CRC error: expected=0x%02x calc=0x%02x",
			rsp_crc8, ersp->buf[7]);
		*cmd_status = 0xffffu;
		return -EIO;
	}

	status = ((uint16_t)ersp->buf[6] << 8) | ersp->buf[5];
	cfg = (((uint32_t)ersp->buf[4] << 24) | ((uint32_t)ersp->buf[3] << 16)
	       | ((uint32_t)ersp->buf[2] << 8) | (uint32_t)ersp->buf[1]);
	*config = cfg;
	*cmd_status = status;

	if (config_offset == ESPI_GET_CONFIG_VW_CAP) {
		hc->vwg_cnt = (uint8_t)((cfg >> 16) & 0x3fu);
	}

	LOG_INF("eSPI HC EMU GET_CONFIG Status 0x%04x", status);

	return 0;
}

/*
 * byte offset    description
 * 0              0x22u
 * 1              addr[15:8]
 * 2              addr[7:0]
 * 3              data[7:0]
 * 4              data[15:8]
 * 5              data[23:16]
 * 6              data[31:24]
 * 7              CRC-8
 * TAR            2 eSPI clocks with I/O tri-stated
 * 0              response code
 * 1              status b[7:0]
 * 2              status b[15:8]
 * 3              CRC-8
 *
 */
int espi_hc_emu_set_config(struct espi_hc_context *hc, uint16_t config_offset,
			   uint32_t config, uint16_t *cmd_status)
{
	int ret = 0;
	uint16_t status = 0;
	uint8_t msg_crc = 0;
	uint8_t rsp_crc = 0;

	LOG_INF("eSPI HC EMU SET_CONFIG 0x%04x 0x%08x", config_offset, config);

	if (!hc || !cmd_status) {
		return -EINVAL;
	}

	struct espi_resp_pkt *ersp = &hc->ersp;
	struct espi_msg_pkt *emsg = &hc->emsg;
	uint8_t *buf = emsg->buf;
	uint8_t *rbuf = ersp->buf;

	memset(emsg, 0, sizeof(struct espi_msg_pkt));
	memset(ersp, 0, sizeof(struct espi_resp_pkt));

	buf[0] = ESPI_OPCODE_SET_CONFIG;
	buf[1] = (uint8_t)((config_offset >> 8) & 0xffu);
	buf[2] = (uint8_t)(config_offset & 0xffu);
	buf[3] = config & 0xffu;
	buf[4] = (config >> 8) & 0xffu;
	buf[5] = (config >> 16) & 0xffu;
	buf[6] = (config >> 24) & 0xffu;

	msg_crc = crc8_init();
	msg_crc = crc8_update(msg_crc, (const void *)buf, 7u);
	msg_crc = crc8_finalize(msg_crc);
	emsg->crc8 = msg_crc;
	buf[7] = msg_crc;

	ret = espi_hc_emu_xfr((const uint8_t *)buf, 8u, ersp->buf, 4u);
	if (ret) {
		return ret;
	}

	rsp_crc = crc8_init();
	rsp_crc = crc8_update(rsp_crc, (const void *)rbuf, 3u);
	rsp_crc = crc8_finalize(rsp_crc);

	if (rsp_crc != rbuf[3]) {
		LOG_ERR("SET_CONFIGURATION: rsp_crc=0x%02x crc=0x%02x",
			rsp_crc, rbuf[3]);
		*cmd_status = 0xffffu;
		return -EIO;
	}

	status = rbuf[2];
	status <<= 8;
	status |= rbuf[1];
	*cmd_status = status;

	if (config_offset == ESPI_GET_CONFIG_VW_CAP) {
		hc->vwg_cnt = (uint8_t)((config >> 16) & 0x3fu);
	}

	LOG_INF("eSPI HC EMU SET_CONFIG Status 0x%04x", status);

	return 0;
}

int espi_hc_ctx_get_status(struct espi_hc_context *hc)
{
	int ret;

	if (!hc) {
		return -EINVAL;
	}

	ret = espi_hc_emu_get_status(hc, &hc->pkt_status);

	return ret;
}

int espi_hc_ctx_emu_get_config(struct espi_hc_context *hc, uint16_t cfg_offset)
{
	int ret;
	uint32_t config;
	uint16_t estatus;

	if (!hc) {
		return -EINVAL;
	}

	config = 0u;
	estatus = 0u;
	ret = espi_hc_emu_get_config(hc, cfg_offset, &config, &estatus);
	if (ret) {
		return ret;
	}

	hc->pkt_status = estatus;
	switch (cfg_offset) {
	case ESPI_GET_CONFIG_DEV_ID:
		hc->version_id = config;
		break;
	case ESPI_GET_CONFIG_GLB_CAP:
		hc->global_cap_cfg = config;
		break;
	case ESPI_GET_CONFIG_PC_CAP:
		hc->pc_cap_cfg = config;
		break;
	case ESPI_GET_CONFIG_VW_CAP:
		hc->vw_cap_cfg = config;
		break;
	case ESPI_GET_CONFIG_OOB_CAP:
		hc->oob_cap_cfg = config;
		break;
	case ESPI_GET_CONFIG_FC_CAP:
		hc->fc_cap_cfg = config;
		break;
	default:
		hc->other_cfg = config;
		break;
	}

	return 0;
}

int espi_hc_ctx_emu_set_config(struct espi_hc_context *hc,
			       uint16_t cfg_offset, uint32_t new_config)
{
	int ret;
	uint16_t estatus;

	if (!hc) {
		return -EINVAL;
	}

	estatus = 0;
	ret = espi_hc_emu_set_config(hc, cfg_offset, new_config, &estatus);
	if (ret) {
		return ret;
	}

	return 0;
}

/* Transmit VW Put packet and get response
 *
 * message
 * byte offset   description
 * 0             VW put command opcode
 * 1             count of VWire groups contained after this byte
 * 2             group 1 host index
 * 3             group 1 data: b[7:4]=valid bit map, b[3:0]=four VWire states
 * ...
 *
 */
int espi_hc_ctx_emu_put_vws(struct espi_hc_context *hc, struct espi_hc_vw *vws, uint8_t nvws)
{
	int ret;
	uint32_t idx, n;
	uint8_t crc8, data, rsp_crc8;

	LOG_INF("eSPI HC EMU PUT_VW nvw = %u", nvws);

	if (!hc || !vws || !nvws) {
		return -EINVAL;
	}

	if (nvws > hc->vwg_cnt_max) {
		return -EINVAL;
	}

	struct espi_msg_pkt *emsg = &hc->emsg;
	struct espi_resp_pkt *ersp = &hc->ersp;
	uint8_t *msgbuf = emsg->buf;
	uint8_t *rspbuf = emsg->buf;

	memset(emsg, 0, sizeof(struct espi_msg_pkt));
	memset(ersp, 0, sizeof(struct espi_resp_pkt));

	msgbuf[0] = ESPI_OPCODE_PUT_VWIRE;
	msgbuf[1] = nvws - 1u; /* number of groups is 0 based [0, 0x3f] */
	idx = 2u;
	n = nvws;
	while (n--) {
		msgbuf[idx++] = vws->host_index;
		data = vws->valid & vws->mask;
		data <<= 4;
		data |= vws->levels & vws->mask;
		msgbuf[idx++] = data;
		vws++;
	}
	crc8 = crc8_init();
	crc8 = crc8_update(crc8, (const void *)msgbuf, idx);
	crc8 = crc8_finalize(crc8);
	msgbuf[idx] = crc8;

	ret = espi_hc_emu_xfr((const uint8_t *)msgbuf, idx+1, rspbuf, 4u);
	if (ret) {
		return ret;
	}

	rsp_crc8 = crc8_init();
	rsp_crc8 = crc8_update(rsp_crc8, (const void *)rspbuf, 3u);
	rsp_crc8 = crc8_finalize(rsp_crc8);

	if (rsp_crc8 != rspbuf[3]) {
		LOG_ERR("PUT_VW: rsp_crc=0x%02x crc=0x%02x",
			rsp_crc8, rspbuf[3]);
		return -EIO;
	}

	hc->pkt_status = (uint16_t)rspbuf[1] | ((uint16_t)rspbuf[2] << 8);

	LOG_INF("eSPI HC EMU PUT_VW Status = 0x%04x", hc->pkt_status);

	return 0;
}

int espi_hc_ctx_emu_get_vws(struct espi_hc_context *hc, struct espi_hc_vw *vws,
			    uint8_t nvws, uint8_t *nrecv)
{
	int ret = 0;
	size_t rsplen = 0, remlen = 0, n = 0, nvw = 0;
	uint8_t crc8 = 0, rsp_crc8 = 0, resp_code = 0;

	LOG_INF("eSPI HC EMU GET_VW nvw = %u", nvws);

	if (!hc || !hc->vwg_cnt_max || !vws || !nvws) {
		return -EINVAL;
	}

	struct espi_msg_pkt *emsg = &hc->emsg;
	struct espi_resp_pkt *ersp = &hc->ersp;

	memset(emsg, 0, sizeof(struct espi_msg_pkt));
	memset(ersp, 0, sizeof(struct espi_resp_pkt));

	emsg->buf[0] = ESPI_OPCODE_GET_VWIRE;
	crc8 = crc8_init();
	crc8 = crc8_update(crc8, (const void *)emsg->buf, 1u);
	crc8 = crc8_finalize(crc8);
	emsg->buf[1] = crc8;

	/* transmit message, keep QSPI open */
	ret = espi_hc_fd_tx_fifo((const uint8_t *)emsg->buf, 2u, BIT(0));
	if (ret) {
		return ret;
	}

	ret = espi_hc_fd_tar();
	if (ret) {
		return ret;
	}

	/* read response code and number of VWire groups in response */
	ret = espi_hc_fd_rx_fifo(ersp->buf, 2u, 0);
	if (ret) {
		return ret;
	}

	resp_code = ersp->buf[0];
	if ((resp_code & ESPI_RESP_CODE_RESP_MSK) != ESPI_RESP_CODE_RESP_OK) {
		LOG_ERR("GET VW bad response: 0x%02x", resp_code);
		/* make sure ESPI_nCS is de-asserted and exit */
		espi_hc_close();
		return -EIO;
	}

	nvw = ersp->buf[1] + 1; /* number of VW groups in the response is 0 based */

	/* each VWire group is two bytes: host index and group value.
	 * Add 3 bytes for 16-bit status and CRC8.
	 * Set transfer flag to close transfer when done.
	 */
	remlen = (nvw * 2u) + 3u;
	ret = espi_hc_fd_rx_fifo(&ersp->buf[2], remlen, BIT(1));
	if (ret) {
		return ret;
	}

	/* total response length add two for response code and num_vws byte.
	 * NOTE: length includes CRC8
	 */
	rsplen = remlen + 2u;
	rsp_crc8 = ersp->buf[rsplen - 1ul];

	crc8 = crc8_init();
	crc8 = crc8_update(rsp_crc8, (const void *)ersp->buf, rsplen - 1u);
	crc8 = crc8_finalize(rsp_crc8);

	if (rsp_crc8 != crc8) {
		LOG_ERR("GET_VW: crc=0x%02x rsp_crc=0x%02x", crc8, rsp_crc8);
		return -EIO;
	}

	hc->pkt_status = (((uint16_t)ersp->buf[rsplen - 2u] << 8)
			  | (uint16_t)ersp->buf[rsplen - 3u]);

	LOG_INF("eSPI HC EMU GET_VW Count = %u Status = 0x%04x", nvw, hc->pkt_status);

	/* copy VW data into vws */
	uint8_t *vwb = &ersp->buf[2];

	/* NOTE: 2-tuple = (host index, value) where
	 * value[3:0] = VWire bit map
	 * value[7:4] = Valid bit map
	 */
	for (n = 0; n < nvw; n++) {
		vws->host_index = *vwb++;
		vws->levels = (*vwb & 0xfu);
		vws->valid = ((*vwb >> 4) & 0xfu);
		LOG_INF("  VW group: HI=0x%02x Levels=0x%02x Valid=0x%02x",
			vws->host_index, vws->levels, vws->valid);
		vwb++;
		vws++;
	}

	/* hc->vwg_cnt = (uint8_t)(nvw & 0xffu); */
	*nrecv = (uint8_t)(nvw & 0xffu);

	return 0;
}

/* Always read maximum possible number of VWires = 64 groups
 * If Target has fewer VWires it will return 0x00 or 0xff.
 * We know exactly the number of groups from the second byte in read data stream.
 * TX phase
 * 0: GET_VWIRE opcode
 * 1: CRC
 * TAR phase: Two clocks with I/O lines tri-stated
 * RX phase
 * 0: Response byte
 * 1: VW zero based count
 * 2: VW[0] index
 * 3: VW[0] data
 * 4: VW[1] index
 * 5: VW[1] data
 * ...
 * N: Status b[7:0]
 * N+1: Status b[15:8]
 * N+2: CRC
 * If we read all 64 groups total response size = 2 + (64 * 2) + 2 + 1 = 133 bytes
 */
int espi_hc_ctx_emu_get_vws2(struct espi_hc_context *hc, struct espi_hc_vw *vws,
			     uint8_t nvws, uint8_t *nrecv)
{
	int ret = 0;
	size_t rsplen = 0, remlen = 0, n = 0, nvw = 0;
	uint8_t crc8 = 0, rsp_crc8 = 0, resp_code = 0;

	LOG_INF("eSPI HC EMU GET_VW nvw = %u", nvws);

	if (!hc || !hc->vwg_cnt_max || !vws || !nvws) {
		return -EINVAL;
	}

	struct espi_msg_pkt *emsg = &hc->emsg;
	struct espi_resp_pkt *ersp = &hc->ersp;

	memset(emsg, 0, sizeof(struct espi_msg_pkt));
	memset(ersp, 0, sizeof(struct espi_resp_pkt));

	emsg->buf[0] = ESPI_OPCODE_GET_VWIRE;
	crc8 = crc8_init();
	crc8 = crc8_update(crc8, (const void *)emsg->buf, 1u);
	crc8 = crc8_finalize(crc8);
	emsg->buf[1] = crc8;

	ret = espi_hc_emu_xfr((const uint8_t *)emsg->buf, 2u, ersp->buf, 133u);
	if (ret) {
		LOG_ERR("eSPI HC EMU GET_VW v2 xfr error: %d", ret);
		return ret;
	}

	resp_code = ersp->buf[0];
	if ((resp_code & ESPI_RESP_CODE_RESP_MSK) != ESPI_RESP_CODE_RESP_OK) {
		LOG_ERR("GET VW bad response: 0x%02x", resp_code);
		/* make sure ESPI_nCS is de-asserted and exit */
		espi_hc_close();
		return -EIO;
	}

	nvw = ersp->buf[1] + 1; /* number of VW groups in the response is 0 based */

	/* each VWire group is two bytes: host index and group value.
	 * Add 3 bytes for 16-bit status and CRC8.
	 * Set transfer flag to close transfer when done.
	 */
	remlen = (nvw * 2u) + 3u;

	/* total response length add two for response code and num_vws byte.
	 * NOTE: length includes CRC8
	 */
	rsplen = remlen + 2u;
	rsp_crc8 = ersp->buf[rsplen - 1ul];

	crc8 = crc8_init();
	crc8 = crc8_update(rsp_crc8, (const void *)ersp->buf, rsplen - 1u);
	crc8 = crc8_finalize(rsp_crc8);

	if (rsp_crc8 != crc8) {
		LOG_ERR("GET_VW: crc=0x%02x rsp_crc=0x%02x", crc8, rsp_crc8);
		return -EIO;
	}

	hc->pkt_status = ((uint16_t)ersp->buf[rsplen - 2u] << 8) | (uint16_t)ersp->buf[rsplen - 3u];

	LOG_INF("eSPI HC EMU GET_VW Count = %u Status = 0x%04x", nvw, hc->pkt_status);

	/* copy VW data into vws */
	uint8_t *vwb = &ersp->buf[2];

	/* NOTE: 2-tuple = (host index, value) where
	 * value[3:0] = VWire bit map
	 * value[7:4] = Valid bit map
	 */
	for (n = 0; n < nvw; n++) {
		vws->host_index = *vwb++;
		vws->levels = (*vwb & 0xfu);
		vws->valid = ((*vwb >> 4) & 0xfu);
		LOG_INF("  VW group: HI=0x%02x Levels=0x%02x Valid=0x%02x",
			vws->host_index, vws->levels, vws->valid);
		vwb++;
		vws++;
	}

	*nrecv = (uint8_t)(nvw & 0xffu);

	return 0;
}

/* TODO - how to do this?
 * Do we look at current CTVW state and if no change don't set valid?
 * If no valid bits set for the group do we even transmit it?
 * What if we create a set of API's that do:
 * 1. Set states of CT VWires in struct espi_hc_context *hc including valid bits
 *    if we actually changed a bit.
 * 2. A second API that sends one or more groups specified by a list of Host Indices.
 * 3. We will probably need helper API's to check for how many groups have valid bit
 *    set.
 */
static struct espi_hc_vw *find_ct_vwire_group(struct espi_hc_context *hc, uint8_t host_index)
{
	if (!hc) {
		return NULL;
	}

	for (int i = 0; i < ESPI_MAX_CT_VW_GROUPS; i++) {
		if (hc->c2t_vw[i].host_index == host_index) {
			return &hc->c2t_vw[i];
		}
	}

	return NULL;
}

static struct espi_hc_vw *find_tc_vwire_group(struct espi_hc_context *hc, uint8_t host_index)
{
	if (!hc) {
		return NULL;
	}

	for (int i = 0; i < ESPI_MAX_TC_VW_GROUPS; i++) {
		if (hc->t2c_vw[i].host_index == host_index) {
			return &hc->t2c_vw[i];
		}
	}

	return NULL;
}

int espi_hc_get_vw_group_data(struct espi_hc_context *hc, uint8_t host_index,
			      struct espi_hc_vw *vwgroup)
{
	struct espi_hc_vw *p = NULL;

	if (!hc || !vwgroup) {
		return -1;
	}

	p = find_ct_vwire_group(hc, host_index);
	if (p) {
		vwgroup->host_index = p->host_index;
		vwgroup->levels = p->levels;
		vwgroup->mask = p->mask;
		vwgroup->valid = p->valid;
		return 0;
	}

	p = find_tc_vwire_group(hc, host_index);
	if (p) {
		vwgroup->host_index = p->host_index;
		vwgroup->levels = p->levels;
		vwgroup->mask = p->mask;
		vwgroup->valid = p->valid;
		return 0;
	}

	return -2;
}


int espi_hc_set_ct_vwire(struct espi_hc_context *hc, uint8_t host_index, uint8_t src, uint8_t level)
{
	struct espi_hc_vw *ctvw = NULL;
	uint8_t mask = 0u;

	if (!hc || (src > 3)) {
		return -EINVAL;
	}

	ctvw = find_ct_vwire_group(hc, host_index);
	if (!ctvw) {
		return -ESRCH;
	}

	mask = BIT(src);
	if (!(mask & ctvw->mask)) {
		return -EINVAL; /* trying to touch not implemented(rsvd) */
	}

	if (ctvw->levels ^ mask) { /* changing state? */
		/* change state value and set corresponding valid bit */
		if (level) {
			ctvw->levels |= mask;
		} else {
			ctvw->levels &= ~mask;
		}
		ctvw->valid |= BIT(src);
	}

	return 0;
}

int espi_hc_ctx_emu_update_t2c_vw(struct espi_hc_context *hc, struct espi_hc_vw *vws,
				  uint8_t nvws)
{
	struct espi_hc_vw *tcvw = NULL;

	if (!hc || !vws) {
		return -EINVAL;
	}

	for (uint8_t n = 0; n < nvws; n++) {
		tcvw = find_tc_vwire_group(hc, vws->host_index);
		if (!tcvw) {
			return -ESRCH;
		}
		tcvw->levels = vws->levels;
		tcvw->valid = vws->valid;
	}

	return 0;
}

int espi_hc_ctx_emu_put_host_index(struct espi_hc_context *hc, uint8_t host_index)
{
	struct espi_hc_vw *ctvw = NULL;
	int ret;

	if (!hc) {
		return -EINVAL;
	}

	ctvw = find_ct_vwire_group(hc, host_index);
	if (!ctvw) {
		return -ESRCH;
	}

	ret = espi_hc_ctx_emu_put_vws(hc, ctvw, 1u);

	return ret;
}

int espi_hc_ctx_emu_put_mult_host_index(struct espi_hc_context *hc, uint8_t *hidxs,
					uint8_t nhidx, uint32_t flags)
{
	struct espi_hc_vw *ctvw = NULL;
	int ret;
	uint32_t idx, n;
	uint8_t crc8, rsp_crc8;

	LOG_INF("eSPI HC EMU PUT_VW MHI = %u", nhidx);

	if (!hc || !hidxs || !nhidx || (nhidx > hc->vwg_cnt_max)) {
		return -EINVAL;
	}

	struct espi_msg_pkt *emsg = &hc->emsg;
	struct espi_resp_pkt *ersp = &hc->ersp;
	uint8_t *msgbuf = emsg->buf;
	uint8_t *rspbuf = emsg->buf;

	memset(emsg, 0, sizeof(struct espi_msg_pkt));
	memset(ersp, 0, sizeof(struct espi_resp_pkt));

	msgbuf[0] = ESPI_OPCODE_PUT_VWIRE;
	msgbuf[1] = nhidx - 1u; /* number of groups is 0 based [0, 0x3f] */
	idx = 2u;
	n = nhidx;
	while (n--) {
		ctvw = find_ct_vwire_group(hc, *hidxs);
		if (!ctvw) {
			return -ESRCH;
		}

		msgbuf[idx++] = ctvw->host_index;
		if (flags & BIT(0)) { /* send reset value */
			msgbuf[idx++] = ctvw->reset_val | (ctvw->mask << 4);
		} else {
			msgbuf[idx++] = ctvw->levels | (ctvw->valid << 4);
		}
		hidxs++;
	}
	crc8 = crc8_init();
	crc8 = crc8_update(crc8, (const void *)msgbuf, idx);
	crc8 = crc8_finalize(crc8);
	msgbuf[idx] = crc8;

	ret = espi_hc_emu_xfr((const uint8_t *)msgbuf, idx+1, rspbuf, 4u);
	if (ret) {
		return ret;
	}

	rsp_crc8 = crc8_init();
	rsp_crc8 = crc8_update(rsp_crc8, (const void *)rspbuf, 3u);
	rsp_crc8 = crc8_finalize(rsp_crc8);

	if (rsp_crc8 != rspbuf[3]) {
		LOG_ERR("PUT_VW MHI: rsp_crc=0x%02x crc=0x%02x",
			rsp_crc8, rspbuf[3]);
		return -EIO;
	}

	hc->pkt_status = (uint16_t)rspbuf[1] | ((uint16_t)rspbuf[2] << 8);

	LOG_INF("eSPI HC EMU PUT_VW MHI Status = 0x%04x", hc->pkt_status);

	return 0;
}

/* Peripheral Channel protocols
 *
 * PUT_IORD_SHORT_1 = 0x40 1-byte
 * PUT_IORD_SHORT_2 = 0x41 2-byte2
 * PUT_IORD_SHORT_4 = 0x43 4-bytes
 *
 * PUT_IOWR_SHORT_1 = 0100_01xy = 0x44 1-byte
 * PUT_IOWR_SHORT_2 = 0100_01xy = 0x45 2-byte2
 * PUT_IOWR_SHORT_4 = 0100_01xy = 0x47 4-byte2
 *
 * Short I/O Read: 1, 2, or 4 bytes.
 *   byte[0] = PUT_IORD_SHORT opcode encodes data length!
 *   byte[1] = Address[15:8] MSB first
 *   byte[2] = Address[7:0]
 *   byte[3] = CRC
 *
 * Short I/O Read Response
 *   byte[0] = Response code
 *   byte[1] = data[7:0]	if 8-bit read we only get one data byte
 *   byte[2] = data[15:8]	if 16-bit read we get two data bytes
 *   byte[3] = data[23:16]
 *   byte[4] = data[31:24]	if 32-bit read we get four data bytes
 *   byte[5] = status[7:0]
 *   byte[6] = status[15:8]
 *   byte[7] = CRC
 *
 * Short I/O Write: 1, 2, or 4 bytes
 *   byte[0] = PUT_IOWR_SHORT opcode encodes data length!
 *   byte[1] = Address[15:8]
 *   byte[2] = Address[7:0]
 *   byte[3] = data[7:0]
 *   byte[4] = data[15:8]
 *   byte[5] = data[23:16]
 *   byte[6] = data[31:24]
 *   byte[7] = CRC
 *
 * Short I/O Write Response
 *   byte[0] = Response code
 *   byte[1] = status[7:0]
 *   byte[2] = status[15:8]
 *   byte[3] = CRC
 */

/* Transmit eSPIO PUT_IOWR packet to target.
 * io_addr_len b[15:0] = I/O address implemented in target
 * io_addr_len b[19:16] = data length in bytes (1, 2, or 4)
 * The target is allowed to respond with one or more WAIT_STATEs
 * after the TAR. To handle this we will more data than necessary
 * in the response phase. The target will generate 1's for the
 * extra clocks. After clocking in the 32 byte response data we
 * parse it and skip over WAIT_STATEs.
 * A WAIT_STATE is a single byte response code with value 0x0F.
 */
int espi_hc_emu_put_iowr(struct espi_hc_context *hc, uint32_t io_addr_len,
			 uint32_t data, uint16_t *cmd_status)
{
	int ret = 0;
	uint16_t status = 0;
	uint8_t n = 0, pktlen = 0;
	uint8_t iolen = 0;
	uint8_t msg_crc = 0;
	uint8_t rsp_crc = 0;
	uint8_t opcode = 0x44u;

	LOG_INF("eSPI HC EMU PUT_IOWR 0x%0x 0x%0x", io_addr_len, data);

	if (!hc || !cmd_status) {
		return -EINVAL;
	}

	iolen = (uint8_t)((io_addr_len >> 16) & 0xfu);
	switch (iolen) {
	case 1:
		pktlen = 5u;
		break;
	case 2:
		pktlen = 6u;
		opcode |= 0x01u;
		break;
	case 4:
		pktlen = 8u;
		opcode |= 0x03u;
		break;
	default:
		LOG_ERR("IO Length in b[19:16] of I/O address incorrect");
		return -EINVAL;
	}

	struct espi_resp_pkt *ersp = &hc->ersp;
	struct espi_msg_pkt *emsg = &hc->emsg;
	uint8_t *buf = emsg->buf;
	uint8_t *rbuf = ersp->buf;

	memset(emsg, 0, sizeof(struct espi_msg_pkt));
	memset(ersp, 0, sizeof(struct espi_resp_pkt));

	buf[0] = opcode;
	buf[1] = (uint8_t)((io_addr_len >> 8) & 0xffu);
	buf[2] = (uint8_t)(io_addr_len & 0xffu);
	n = 3u;
	while (n < (pktlen - 1u)) {
		buf[n] = (uint8_t)(data & 0xffu);
		data >>= 8;
		n++;
	}
	msg_crc = crc8_init();
	msg_crc = crc8_update(msg_crc, (const void *)buf, (pktlen - 1u));
	msg_crc = crc8_finalize(msg_crc);
	emsg->crc8 = msg_crc;
	buf[pktlen - 1u] = msg_crc;

	ret = espi_hc_emu_xfr((const uint8_t *)buf, pktlen, ersp->buf, 32u);
	if (ret) {
		return ret;
	}

	/* Skip over WAIT_STATEs as they are not part of the response CRC calculation. */
	n = 0;
	while (n < 32u) {
		if (ersp->buf[n] != 0x0Fu) {
			break;
		}
		n++;
	}

	if (n >= 32u) {
		LOG_ERR("PUT_IOWR error received 32 WAIT_STATEs!");
		return -EIO;
	}

	rsp_crc = crc8_init();
	rsp_crc = crc8_update(rsp_crc, (const void *)&rbuf[n], 3u);
	rsp_crc = crc8_finalize(rsp_crc);

	if (rsp_crc != rbuf[n + 3u]) {
		LOG_ERR("PUT_IOWR: rsp_crc=0x%02x crc=0x%02x",
			rsp_crc, rbuf[n + 3u]);
		*cmd_status = 0xffffu;
		return -EIO;
	}

	status = rbuf[n + 2u];
	status <<= 8;
	status |= rbuf[n + 1u];
	*cmd_status = status;

	LOG_INF("eSPI HC EMU PUT_IOWR Status 0x%04x", status);

	return 0;
}

int espi_hc_emu_put_iord(struct espi_hc_context *hc, uint32_t io_addr_len,
			 uint32_t *data, uint16_t *cmd_status)
{
	uint32_t resp_data = 0u;
	int ret = 0;
	uint16_t status = 0u;
	uint8_t iolen = 0u;
	uint8_t pktlen = 0u; /* response packet length */
	uint8_t rsp_crc8 = 0;
	uint8_t opcode = 0x40u;

	LOG_INF("eSPI HC emu: PUT_IORD io_addr_len=0x%0x", io_addr_len);

	if (!hc || !data || !cmd_status) {
		return -EINVAL;
	}

	iolen = (uint8_t)((io_addr_len >> 16) & 0xfu);
	switch (iolen) {
	case 1:
		pktlen = 5u;
		break;
	case 2:
		pktlen = 6u;
		opcode |= 0x01u;
		break;
	case 4:
		pktlen = 8u;
		opcode |= 0x03u;
		break;
	default:
		LOG_ERR("IO Length in b[19:16] of I/O address incorrect");
		return -EINVAL;
	}

	struct espi_msg_pkt *emsg = &hc->emsg;
	struct espi_resp_pkt *ersp = &hc->ersp;

	memset(emsg, 0, sizeof(struct espi_msg_pkt));
	memset(ersp, 0, sizeof(struct espi_resp_pkt));

	emsg->buf[0] = opcode;
	emsg->buf[1] = (uint8_t)((io_addr_len >> 8) & 0xffu);
	emsg->buf[2] = (uint8_t)(io_addr_len & 0xffu);

	emsg->crc8 = crc8_init();
	emsg->crc8 = crc8_update(emsg->crc8, (const void *)emsg->buf, 3u);
	emsg->crc8 = crc8_finalize(emsg->crc8);
	emsg->buf[3] = emsg->crc8;

	ret = espi_hc_emu_xfr((const uint8_t *)emsg->buf, 4u, ersp->buf, pktlen);
	if (ret) {
		return ret;
	}

	rsp_crc8 = crc8_init();
	rsp_crc8 = crc8_update(rsp_crc8, (const void *)ersp->buf, (pktlen - 1u));
	rsp_crc8 = crc8_finalize(rsp_crc8);

	if (rsp_crc8 != ersp->buf[(pktlen - 1u)]) {
		LOG_ERR("PUT_IORD: CRC error: expected=0x%02x calc=0x%02x",
			rsp_crc8, ersp->buf[(pktlen - 1u)]);
		*cmd_status = 0xffffu;
		return -EIO;
	}

	/* status[7:0] at index pktlen - 3 */
	status = ((uint16_t)ersp->buf[pktlen - 3u] << 8) | ersp->buf[pktlen - 2u];
	/* response data starts at index 1 */
	resp_data = ersp->buf[1];
	if (iolen > 1) {
		resp_data |= ((uint32_t)ersp->buf[2] << 8);
	}
	if (iolen > 2) {
		resp_data |= ((uint32_t)ersp->buf[3] << 16);
		resp_data |= ((uint32_t)ersp->buf[4] << 24);
	}

	*data = resp_data;
	*cmd_status = status;

	LOG_INF("eSPI HC EMU PUT_IORD Status 0x%04x", status);

	return 0;
}
