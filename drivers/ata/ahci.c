/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ata_ahci, LOG_LEVEL_DBG);

#include <errno.h>

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/iommu/iommu.h>
#include <zephyr/sys/mem_blocks.h>

#include "ahci.h"

#define DT_DRV_COMPAT ata_ahci

#define BAR_AHCI_BASE_ADDR_DEFAULT 0

#define SATA_SIG_ATA   0x00000101 // SATA drive
#define SATA_SIG_ATAPI 0xEB140101 // SATAPI drive
#define SATA_SIG_SEMB  0xC33C0101 // Enclosure management bridge
#define SATA_SIG_PM    0x96690101 // Port multiplier

#define ATA_CMD_READ_DMA_EX 0x25
#define ATA_CMD_ID_ATA      0xec

/*
 * Command list 1k * 32, 1024 bytes per port
 * FIS Structure 256 * 32, 256 bytes per port
 * Command table 256 * 32 * 32, 256 bytes per command, each port has 32 commands
 *   Each command table has 8 entries.
 */
#define AHCI_BASE_SIZE                       (1024 + 256 + 256 * 32)
#define AHCI_PORT_DMA_BUFFER_SIZE            1024
#define BLOCK_NUM_CEILING(total, block_size) ((total + block_size - 1) / block_size)

#define DMA_BUFFER_BLOCK_SIZE 0x1000 /* Make sure each block is aligned to this */
#define DMA_BUFFER_BLOCK_NUM                                                                       \
	(BLOCK_NUM_CEILING(AHCI_BASE_SIZE, DMA_BUFFER_BLOCK_SIZE) +                                \
	 BLOCK_NUM_CEILING(AHCI_PORT_DMA_BUFFER_SIZE, DMA_BUFFER_BLOCK_SIZE))

#define BIT_ISACTIVE(var, pos) (((var) & (1 << pos)))

static void *ahci_base;
static uint8_t *port_buf;

SYS_MEM_BLOCKS_DEFINE_STATIC(dma_buffer, DMA_BUFFER_BLOCK_SIZE, DMA_BUFFER_BLOCK_NUM, 0x1000)

struct ata_ahci_device_config {
	uint32_t placeholder;
};

struct ata_ahci_device_data {
	DEVICE_MMIO_RAM;

	/* BDF & DID/VID */
	struct pcie_dev *pcie;
	struct iommu_domain *iommu_domain;

	uint8_t cmdslots;
	mem_addr_t port_mmio[32];
	mem_addr_t port_sysmem[32];
};

enum sata_fis_type {
	FIS_TYPE_REG_H2D = 0x27,   // Register FIS - host to device
	FIS_TYPE_REG_D2H = 0x34,   // Register FIS - device to host
	FIS_TYPE_DMA_ACT = 0x39,   // DMA activate FIS - device to host
	FIS_TYPE_DMA_SETUP = 0x41, // DMA setup FIS - bidirectional
	FIS_TYPE_DATA = 0x46,      // Data FIS - bidirectional
	FIS_TYPE_BIST = 0x58,      // BIST activate FIS - bidirectional
	FIS_TYPE_PIO_SETUP = 0x5F, // PIO setup FIS - device to host
	FIS_TYPE_DEV_BITS = 0xA1,  // Set device bits FIS - device to host
};

struct sata_fis_reg_h2d {
	// DWORD 0
	uint8_t fis_type; // FIS_TYPE_REG_H2D

	uint8_t pmport: 4; // Port multiplier
	uint8_t rsv0: 3;   // Reserved
	uint8_t c: 1;      // 1: Command, 0: Control

	uint8_t command;  // Command register
	uint8_t featurel; // Feature register, 7:0

	// DWORD 1
	uint8_t lba0;   // LBA low register, 7:0
	uint8_t lba1;   // LBA mid register, 15:8
	uint8_t lba2;   // LBA high register, 23:16
	uint8_t device; // Device register

	// DWORD 2
	uint8_t lba3;     // LBA register, 31:24
	uint8_t lba4;     // LBA register, 39:32
	uint8_t lba5;     // LBA register, 47:40
	uint8_t featureh; // Feature register, 15:8

	// DWORD 3
	uint8_t countl;  // Count register, 7:0
	uint8_t counth;  // Count register, 15:8
	uint8_t icc;     // Isochronous command completion
	uint8_t control; // Control register

	// DWORD 4
	uint8_t rsv1[4]; // Reserved
} __packed;

struct sata_fis_reg_d2h {
	// DWORD 0
	uint8_t fis_type; // FIS_TYPE_REG_D2H

	uint8_t pmport: 4; // Port multiplier
	uint8_t rsv0: 2;   // Reserved
	uint8_t i: 1;      // Interrupt bit
	uint8_t rsv1: 1;   // Reserved

	uint8_t status; // Status register
	uint8_t error;  // Error register

	// DWORD 1
	uint8_t lba0;   // LBA low register, 7:0
	uint8_t lba1;   // LBA mid register, 15:8
	uint8_t lba2;   // LBA high register, 23:16
	uint8_t device; // Device register

	// DWORD 2
	uint8_t lba3; // LBA register, 31:24
	uint8_t lba4; // LBA register, 39:32
	uint8_t lba5; // LBA register, 47:40
	uint8_t rsv2; // Reserved

	// DWORD 3
	uint8_t countl;  // Count register, 7:0
	uint8_t counth;  // Count register, 15:8
	uint8_t rsv3[2]; // Reserved

	// DWORD 4
	uint8_t rsv4[4]; // Reserved
} __packed;

struct sata_fis_data {
	// DWORD 0
	uint8_t fis_type; // FIS_TYPE_DATA

	uint8_t pmport: 4; // Port multiplier
	uint8_t rsv0: 4;   // Reserved

	uint8_t rsv1[2]; // Reserved

	// DWORD 1 ~ N
	uint32_t data[1]; // Payload
} __packed;

struct sata_fis_pio_setup {
	// DWORD 0
	uint8_t fis_type; // FIS_TYPE_PIO_SETUP

	uint8_t pmport: 4; // Port multiplier
	uint8_t rsv0: 1;   // Reserved
	uint8_t d: 1;      // Data transfer direction, 1 - device to host
	uint8_t i: 1;      // Interrupt bit
	uint8_t rsv1: 1;

	uint8_t status; // Status register
	uint8_t error;  // Error register

	// DWORD 1
	uint8_t lba0;   // LBA low register, 7:0
	uint8_t lba1;   // LBA mid register, 15:8
	uint8_t lba2;   // LBA high register, 23:16
	uint8_t device; // Device register

	// DWORD 2
	uint8_t lba3; // LBA register, 31:24
	uint8_t lba4; // LBA register, 39:32
	uint8_t lba5; // LBA register, 47:40
	uint8_t rsv2; // Reserved

	// DWORD 3
	uint8_t countl;   // Count register, 7:0
	uint8_t counth;   // Count register, 15:8
	uint8_t rsv3;     // Reserved
	uint8_t e_status; // New value of status register

	// DWORD 4
	uint16_t tc;     // Transfer count
	uint8_t rsv4[2]; // Reserved
} __packed;

struct sata_fis_dma_setup {
	/* DWORD 0 */
	uint8_t fis_type; // FIS_TYPE_DMA_SETUP

	uint8_t pmport: 4; // Port multiplier
	uint8_t rsv0: 1;   // Reserved
	uint8_t d: 1;      // Data transfer direction, 1 - device to host
	uint8_t i: 1;      // Interrupt bit
	uint8_t a: 1;      // Auto-activate. Specifies if DMA Activate FIS is needed

	uint8_t rsved[2]; // Reserved

	/* DWORD 1&2 */
	uint64_t DMAbufferID; // DMA Buffer Identifier. Used to Identify DMA buffer in host memory.
			      // SATA Spec says host specific and not in Spec. Trying AHCI spec
			      // might work.

	/* DWORD 3 */
	uint32_t rsvd; // More reserved

	/* DWORD 4 */
	uint32_t DMAbufOffset; // Byte offset into buffer. First 2 bits must be 0

	/* DWORD 5 */
	uint32_t TransferCount; // Number of bytes to transfer. Bit 0 must be 0

	/* DWORD 6 */
	uint32_t resvd; // Reserved
} __packed;

struct ahci_hba_fis {
	// 0x00
	struct sata_fis_dma_setup dsfis; // DMA Setup FIS
	uint8_t pad0[4];

	// 0x20
	struct sata_fis_pio_setup psfis; // PIO Setup FIS
	uint8_t pad1[12];

	// 0x40
	struct sata_fis_reg_d2h rfis; // Register – Device to Host FIS
	uint8_t pad2[4];

	// 0x58
	uint16_t sdbfis; // Set Device Bit FIS

	// 0x60
	uint8_t ufis[64];

	// 0xA0
	uint8_t rsv[0x100 - 0xA0];
} __packed;

struct ahci_hba_cmd_header {
	// DW0
	uint8_t cfl: 5; // Command FIS length in DWORDS, 2 ~ 16
	uint8_t a: 1;   // ATAPI
	uint8_t w: 1;   // Write, 1: H2D, 0: D2H
	uint8_t p: 1;   // Prefetchable

	uint8_t r: 1;    // Reset
	uint8_t b: 1;    // BIST
	uint8_t c: 1;    // Clear busy upon R_OK
	uint8_t rsv0: 1; // Reserved
	uint8_t pmp: 4;  // Port multiplier port

	uint16_t prdtl; // Physical region descriptor table length in entries

	// DW1
	uint32_t prdbc; // Physical region descriptor byte count transferred

	// DW2, 3
	uint32_t ctba;  // Command table descriptor base address
	uint32_t ctbau; // Command table descriptor base address upper 32 bits

	// DW4 - 7
	uint32_t rsv1[4]; // Reserved
} __packed;

struct ahci_hba_prdt_entry {
	uint32_t dba;  // Data base address
	uint32_t dbau; // Data base address upper 32 bits
	uint32_t rsv0; // Reserved

	// DW3
	uint32_t dbc: 22; // Byte count, 4M max
	uint32_t rsv1: 9; // Reserved
	uint32_t i: 1;    // Interrupt on completion
} __packed;

struct ahci_hba_cmd_tbl {
	// 0x00
	uint8_t cfis[64]; // Command FIS

	// 0x40
	uint8_t acmd[16]; // ATAPI command, 12 or 16 bytes

	// 0x50
	uint8_t rsv[48]; // Reserved

	// 0x80
	struct ahci_hba_prdt_entry
		prdt_entry[1]; // Physical region descriptor table entries, 0 ~ 65535
} __packed;

/*
 * define the IDENTIFY DEVICE structure
 */
struct _ata_identify_device_data {
	uint16_t reserved1[10];        /* 0-9 */
	uint16_t serial_number[10];    /* 10-19 */
	uint16_t reserved2[3];         /* 20-22 */
	uint16_t firmware_revision[4]; /* 23-26 */
	uint16_t model_number[20];     /* 27-46*/
	uint16_t reserved3[170];       /* 47-216 */
	uint16_t rotational_speed;     /* 217 */
	uint16_t reserved4[38];        /* 218-255 */
} __packed;

// Check device type
static int check_type(HBA_PORT *port)
{
	uint32_t ssts = port->ssts;

	if (FIELD_GET(SSTS_DET_M, ssts) != DET_PRESENT) { // Check drive status
		return AHCI_DEV_NULL;
	}
	if (FIELD_GET(SSTS_IPM_M, ssts) != IPM_ACTIVE) {
		return AHCI_DEV_NULL;
	}

	switch (port->sig) {
	case SATA_SIG_ATAPI:
		return AHCI_DEV_SATAPI;
	case SATA_SIG_SEMB:
		return AHCI_DEV_SEMB;
	case SATA_SIG_PM:
		return AHCI_DEV_PM;
	default:
		return AHCI_DEV_SATA;
	}
}

static void probe_ahci(const struct device *dev)
{
	struct ata_ahci_device_data *data = dev->data;

	sys_write32(sys_read32(data->_mmio + AHCI_GHC) | GHC_HR, data->_mmio + AHCI_GHC);
	k_sleep(K_MSEC(1));

	if ((sys_read32(data->_mmio + AHCI_GHC) & GHC_AE) == 0) {
		LOG_DBG("Default mode is legacy, change it to AHCI mode");
		sys_write32(sys_read32(data->_mmio + AHCI_GHC) | GHC_AE, data->_mmio + AHCI_GHC);
	}

	uint8_t ncs = (sys_read32(data->_mmio + AHCI_CAP) >> 8) & 0x1fUL;
	data->cmdslots = ncs;
}

static void probe_port(const struct device *dev)
{
	// Search disk in implemented ports
	struct ata_ahci_device_data *data = dev->data;
	HBA_MEM *abar = (HBA_MEM *)data->_mmio;

	uint32_t pi = sys_read32(data->_mmio + AHCI_PI);
	int i = 0;
	while (i < 32) {
		if (pi & 1) {
			int dt = check_type(&abar->ports[i]);
			if (dt == AHCI_DEV_SATA) {
				data->port_mmio[i] = (mem_addr_t) & (abar->ports[i]);
				LOG_INF("SATA drive found at port %d\n", i);
			} else if (dt == AHCI_DEV_SATAPI) {
				data->port_mmio[i] = (mem_addr_t) & (abar->ports[i]);
				LOG_INF("SATAPI drive found at port %d\n", i);
			} else if (dt == AHCI_DEV_SEMB) {
				data->port_mmio[i] = (mem_addr_t) & (abar->ports[i]);
				LOG_INF("SEMB drive found at port %d\n", i);
			} else if (dt == AHCI_DEV_PM) {
				LOG_INF("PM drive found at port %d\n", i);
			} else {
				LOG_INF("No drive found at port %d\n", i);
			}
		}

		pi >>= 1;
		i++;
	}
}

// Start command engine
static void start_cmd(const struct device *dev, int portno)
{
	struct ata_ahci_device_data *data = dev->data;
	mem_addr_t port_mmio = data->port_mmio[portno];

	while ((sys_read32(port_mmio + AHCI_P_TFD) & TFD_STS_BSY) != 0) {
		;
	}

	while ((sys_read32(port_mmio + AHCI_P_TFD) & TFD_STS_DRQ) != 0) {
		;
	}

	while ((sys_read32(port_mmio + AHCI_P_TFD) & TFD_STS_ERR) != 0) {
		;
	}

	while (FIELD_GET(SSTS_DET_M, sys_read32(port_mmio + AHCI_P_SSTS)) != DET_PRESENT) {
		;
	}

	// Set FRE (bit4) and ST (bit0)
	sys_write32(sys_read32(port_mmio + AHCI_P_CMD) | CMD_SUD, port_mmio + AHCI_P_CMD);
	sys_write32(sys_read32(port_mmio + AHCI_P_CMD) | CMD_ICC_ACTIVE, port_mmio + AHCI_P_CMD);
	sys_write32(sys_read32(port_mmio + AHCI_P_CMD) | CMD_FRE, port_mmio + AHCI_P_CMD);
	sys_write32(sys_read32(port_mmio + AHCI_P_CMD) | CMD_ST, port_mmio + AHCI_P_CMD);
}

// Stop command engine
static void stop_cmd(const struct device *dev, int portno)
{
	struct ata_ahci_device_data *data = dev->data;
	mem_addr_t port_mmio = data->port_mmio[portno];

	if ((sys_read32(port_mmio + AHCI_P_CMD) & CMD_ST) != 0) {
		// Clear ST (bit0)
		sys_write32(sys_read32(port_mmio + AHCI_P_CMD) & ~CMD_ST, port_mmio + AHCI_P_CMD);
		while ((sys_read32(port_mmio + AHCI_P_CMD) & CMD_CR) != 0) {
			;
		}
	}

	if ((sys_read32(port_mmio + AHCI_P_CMD) & CMD_FRE) != 0) {
		// Clear FRE (bit4)
		sys_write32(sys_read32(port_mmio + AHCI_P_CMD) & ~CMD_FRE, port_mmio + AHCI_P_CMD);
		while ((sys_read32(port_mmio + AHCI_P_CMD) & CMD_FR) != 0) {
			;
		}
	}

	// Wait until FR (bit14), CR (bit15) are cleared
	while (1) {
		if (sys_read32(port_mmio + AHCI_P_CMD) & CMD_FR) {
			continue;
		}
		if (sys_read32(port_mmio + AHCI_P_CMD) & CMD_CR) {
			continue;
		}
		break;
	}
}

static void port_rebase(const struct device *dev, int portno)
{
	struct ata_ahci_device_data *data = dev->data;
	mem_addr_t abar = data->_mmio;
	mem_addr_t port_mmio = data->port_mmio[portno];
	uint64_t addr;
	uint64_t clb;

	stop_cmd(dev, portno);

	// Command list offset: 1K*portno
	// Command list entry size = 32
	// Command list entry maxim count = 32
	// Command list maxim size = 32*32 = 1K per port
	addr = (uint64_t)ahci_base;
	memset((void *)addr, 0, 1024);

	sys_write32((uint32_t)addr, port_mmio + AHCI_P_CLB);
	sys_write32((uint32_t)(addr >> 32), port_mmio + AHCI_P_CLBU);

	// FIS offset: 400H
	// FIS entry size = 256 bytes per port
	addr = (uint64_t)ahci_base + 0x400;
	memset((void *)addr, 0, 256);

	sys_write32((uint32_t)addr, port_mmio + AHCI_P_FB);
	sys_write32((uint32_t)(addr >> 32), port_mmio + AHCI_P_FBU);

	// Command table offset: 500H
	// Command table size = 256*32 = 8K per port
	clb = sys_read32(port_mmio + AHCI_P_CLB);
	struct ahci_hba_cmd_header *cmdheader = (struct ahci_hba_cmd_header *)(clb);
	for (int i = 0; i < 32; i++) {
		cmdheader[i].prdtl = 0; // 8 prdt entries per command table
					// 256 bytes per command table
		mem_addr_t ctba = clb + 0x500 + 256 * i;
		cmdheader[i].ctba = (uint32_t)ctba;
		cmdheader[i].ctbau = (uint32_t)(ctba >> 32);
		memset((void *)ctba, 0, 256);
	}

	uint32_t scontrol = sys_read32(port_mmio + AHCI_P_SCTL);
	int tries = 5;
	do {
		scontrol = (scontrol & 0x0f0) | 0x300;
		sys_write32(scontrol, port_mmio + AHCI_P_SCTL);

		k_sleep(K_MSEC(100));

		scontrol = sys_read32(port_mmio + AHCI_P_SCTL);
	} while ((scontrol & 0xf0f) != 0x300 && --tries);

	if ((scontrol & 0xf0f) != 0x300) {
		LOG_WRN("failed to resume link (scontrol %X)", scontrol);
	}

	sys_write32(sys_read32(abar + AHCI_GHC) | GHC_IE, abar + AHCI_GHC);

	start_cmd(dev, portno); // Start command engine
}

// Find a free command list slot
static int find_cmdslot(HBA_PORT *port, uint8_t cmdslots)
{
	// If not set in SACT and CI, the slot is free
	uint32_t slots = (port->sact | port->ci);
	for (int i = 0; i < cmdslots; i++) {
		if ((slots & 1) == 0) {
			return i;
		}
		slots >>= 1;
	}
	LOG_DBG("Cannot find free command list entry\n");
	return -1;
}

static int port_read(const struct device *dev, int portno, uint32_t startl, uint32_t starth,
		     uint32_t count, uint64_t buf_p)
{
	struct ata_ahci_device_data *data = dev->data;
	mem_addr_t port_mmio = data->port_mmio[portno];
	mem_addr_t clb = sys_read32(port_mmio + AHCI_P_CLB);

	int spin = 0; // Spin lock timeout counter
	int slot = find_cmdslot((HBA_PORT *)port_mmio, data->cmdslots);
	if (slot == -1) {
		return -ENOMEM;
	}

	struct ahci_hba_cmd_header *cmdheader = (struct ahci_hba_cmd_header *)(clb + 0x20UL * slot);
	cmdheader += slot;
	cmdheader->cfl = sizeof(struct sata_fis_reg_h2d) / sizeof(uint32_t); // Command FIS size
	cmdheader->w = 0;                                                    // Read from device
	cmdheader->c = 0;
	cmdheader->p = 0;
	cmdheader->prdtl = (uint16_t)((count - 1) >> 4) + 1; // PRDT entries count
	cmdheader->ctbau = 0;

	mem_addr_t ctb = (mem_addr_t)cmdheader->ctba;
	struct ahci_hba_cmd_tbl *cmdtbl = (struct ahci_hba_cmd_tbl *)ctb;
	memset(cmdtbl, 0,
	       sizeof(struct ahci_hba_cmd_tbl) +
		       (cmdheader->prdtl - 1) * sizeof(struct ahci_hba_prdt_entry));

	// 8K bytes (16 sectors) per PRDT
	int i = 0;
	for (; i < cmdheader->prdtl - 1; i++) {
		cmdtbl->prdt_entry[i].dba = (uint32_t)buf_p;
		cmdtbl->prdt_entry[i].dbau = (uint32_t)((buf_p << 32) & 0xffffffffUL);
		cmdtbl->prdt_entry[i].dbc = 8 * 1024 - 1; // 8K bytes (this value should always be
							  // set to 1 less than the actual value)
		cmdtbl->prdt_entry[i].i = 1;
		buf_p += 4 * 1024; // 4K words
		count -= 16;       // 16 sectors
	}
	// Last entry
	cmdtbl->prdt_entry[i].dba = (uint32_t)buf_p;
	cmdtbl->prdt_entry[i].dbau = (uint32_t)((buf_p << 32) & 0xffffffffUL);
	// cmdtbl->prdt_entry[i].dbc = (count << 9) - 1; // 512 bytes per sector
	cmdtbl->prdt_entry[i].dbc = (count << 9) - 1; // 512 bytes per sector
	cmdtbl->prdt_entry[i].i = 0;

	// Setup command
	struct sata_fis_reg_h2d *cmdfis = (struct sata_fis_reg_h2d *)(&cmdtbl->cfis);

	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1; // Command
	cmdfis->command = ATA_CMD_ID_ATA;

	cmdfis->lba0 = (uint8_t)startl;
	cmdfis->lba1 = (uint8_t)(startl >> 8);
	cmdfis->lba2 = (uint8_t)(startl >> 16);
	// cmdfis->device = 1 << 6; // LBA mode
	cmdfis->device = 0xa0; // LBA mode

	cmdfis->lba3 = (uint8_t)(startl >> 24);
	cmdfis->lba4 = (uint8_t)starth;
	cmdfis->lba5 = (uint8_t)(starth >> 8);

	// cmdfis->countl = count & 0xFF;
	// cmdfis->counth = (count >> 8) & 0xFF;
	cmdfis->countl = 0x0;
	cmdfis->counth = 0x0;
	cmdfis->control = 0x08;

	// The below loop waits until the port is no longer busy before issuing a new command
	while ((sys_read32(port_mmio + AHCI_P_TFD) & (TFD_STS_BSY | TFD_STS_DRQ)) &&
	       spin < 1000000) {
		spin++;
	}
	if (spin == 1000000) {
		LOG_WRN("Port is hung\n");
		return -EBUSY;
	}

	sys_write32(0x1UL << slot, port_mmio + AHCI_P_SACT);
	sys_write32(0x1UL << slot, port_mmio + AHCI_P_CI);

	// Wait for completion
	while (1) {
		// In some longer duration reads, it may be helpful to spin on the DPS bit
		// in the PxIS port field as well (1 << 5)
		uint32_t ci_val, sact_val;

		ci_val = sys_read32(port_mmio + AHCI_P_CI);
		sact_val = sys_read32(port_mmio + AHCI_P_SACT);
		if (!BIT_ISACTIVE(ci_val, slot) && !BIT_ISACTIVE(sact_val, slot)) {
			break;
		}

		if (sys_read32(port_mmio + AHCI_P_IS) & IS_TFES_B) // Task file error
		{
			LOG_WRN("Read disk error\n");
			return -EFAULT;
		}

		k_usleep(1);
	}

	// Check after completion
	if (sys_read32(port_mmio + AHCI_P_IS) & IS_TFES_B) {
		LOG_DBG("Read disk error\n");
		return -EFAULT;
	}

	/* we don't use upper 32 bits */
	mem_addr_t fb = sys_read32(port_mmio + AHCI_P_FB);
	struct ahci_hba_fis *fis = (struct ahci_hba_fis *)fb;
	/* ATA_CMD_ID_ATA go through with PIOFIS */
	if (fis->psfis.error) {
		LOG_WRN("Device report an error");
		return -EFAULT;
	}

	LOG_DBG("transfer bytes: %d", fis->psfis.tc);

	return 0;
}

static void ata_show_identify_device(struct _ata_identify_device_data *data)
{
	int i;
	char buf[40];

	for (i = 0; i < 10; i++) {
		buf[2 * i] = (data->serial_number[i] >> 8) & 0xff;
		buf[2 * i + 1] = data->serial_number[i] & 0xff;
	}
	buf[2 * i] = '\0';
	LOG_INF("Serial Number: %s", buf);

	for (i = 0; i < 4; i++) {
		buf[2 * i] = (data->firmware_revision[i] >> 8) & 0xff;
		buf[2 * i + 1] = data->firmware_revision[i] & 0xff;
	}
	buf[2 * i] = '\0';
	LOG_INF("Firmware revision: %s", buf);

	for (i = 0; i < 20; i++) {
		buf[2 * i] = (data->model_number[i] >> 8) & 0xff;
		buf[2 * i + 1] = data->model_number[i] & 0xff;
	}
	buf[2 * i] = '\0';
	LOG_INF("Model Number: %s", buf);
}

static int ata_ahci_init(const struct device *dev)
{
	int ret;
	// const struct ata_ahci_device_config *dev_cfg = dev->config;
	struct ata_ahci_device_data *data = dev->data;
	struct pcie_bar mbar;

	if (data->pcie->bdf == PCIE_BDF_NONE) {
		return -EINVAL;
	}

	pcie_set_cmd(data->pcie->bdf, PCIE_CONF_CMDSTAT_MASTER | PCIE_CONF_CMDSTAT_MEM, true);
	pcie_probe_mbar(data->pcie->bdf, BAR_AHCI_BASE_ADDR_DEFAULT, &mbar);

	device_map(DEVICE_MMIO_RAM_PTR(dev), mbar.phys_addr, mbar.size, K_MEM_CACHE_NONE);

	//data->iommu_domain = iommu_apply_default_domain(data->unit);
	data->iommu_domain = iommu_get_default_domain(data->pcie);
	iommu_dev_map(data->iommu_domain, (mem_addr_t)dma_buffer.buffer, (mem_addr_t)dma_buffer.buffer,
		      dma_buffer.num_blocks * (1 << dma_buffer.blk_sz_shift), 0);

	ret = sys_mem_blocks_alloc(
		&dma_buffer, BLOCK_NUM_CEILING(AHCI_BASE_SIZE, DMA_BUFFER_BLOCK_SIZE), &ahci_base);
	__ASSERT(ret == 0, "sys_mem_blocks_alloc failed, ret = %d", ret);

	ret = sys_mem_blocks_alloc(
		&dma_buffer, BLOCK_NUM_CEILING(AHCI_PORT_DMA_BUFFER_SIZE, DMA_BUFFER_BLOCK_SIZE),
		(void *)&port_buf);
	__ASSERT(ret == 0, "sys_mem_blocks_alloc failed, ret = %d", ret);
	LOG_DBG("ahch_base address: %p", ahci_base);
	LOG_DBG("port_buf address: %p", port_buf);

	probe_ahci(dev);
	probe_port(dev);
	port_rebase(dev, 0);

	LOG_INF("First test");
	ret = port_read(dev, 0, 0, 0, 1, (uint64_t)port_buf);
	if (ret) {
		LOG_WRN("port read failed, ret = %d", ret);
	}
	ata_show_identify_device((struct _ata_identify_device_data *)port_buf);

	LOG_INF("Clear port read buffer");
	memset(port_buf, 0, DMA_BUFFER_BLOCK_SIZE);

	LOG_INF("Unmapping");
	iommu_dev_unmap(data->iommu_domain, (mem_addr_t)dma_buffer.buffer,
		      dma_buffer.num_blocks * (1 << dma_buffer.blk_sz_shift));

	LOG_INF("Read after iommu_dev_unmap");
	ret = port_read(dev, 0, 0, 0, 1, (uint64_t)port_buf);
	if (ret) {
		LOG_WRN("Port read failed, ret = %d", ret);
	}
	ata_show_identify_device((struct _ata_identify_device_data *)port_buf);

	LOG_INF("Mapping again");
	memset(port_buf, 0, DMA_BUFFER_BLOCK_SIZE);
	iommu_dev_map(data->iommu_domain, (mem_addr_t)dma_buffer.buffer, (mem_addr_t)dma_buffer.buffer,
		      dma_buffer.num_blocks * (1 << dma_buffer.blk_sz_shift), 0);
	ret = port_read(dev, 0, 0, 0, 1, (uint64_t)port_buf);
	if (ret) {
		LOG_WRN("Port read failed, ret = %d", ret);
	}
	ata_show_identify_device((struct _ata_identify_device_data *)port_buf);

	stop_cmd(dev, 0);

	return 0;
}

DEVICE_PCIE_INST_DECLARE(0);

// static const struct ata_ahci_device_config ata_ahci_device_cfg_0 = { };

static struct ata_ahci_device_data ata_ahci_device_data_0 = {
	DEVICE_PCIE_INST_INIT(0, pcie),
};

DEVICE_DT_INST_DEFINE(0, &ata_ahci_init, NULL, &ata_ahci_device_data_0, NULL,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);
