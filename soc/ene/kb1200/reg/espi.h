/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB1200_ESPI_H
#define ENE_KB1200_ESPI_H

/**
 *  Structure type to access Enhanced Serial Peripheral Interface (ESPI).
 */
struct espi_regs {
	volatile uint32_t Reserved0;    /*Reserved */
	volatile uint32_t ESPIID;       /*ID Register */
	volatile uint32_t ESPIGENCFG;   /*General Configuration Register */
	volatile uint32_t Reserved1;    /*Reserved */
	volatile uint32_t ESPIC0CFG;    /*Channel-0(Peripheral) Configuration Register */
	volatile uint32_t Reserved2[3]; /*Reserved */
	volatile uint32_t ESPIC1CFG;    /*Channel-1(Virtual Wire) Configuration Register */
	volatile uint32_t Reserved3[3]; /*Reserved */
	volatile uint32_t ESPIC2CFG;    /*Channel-2(Out-Of-Band) Configuration Register */
	volatile uint32_t Reserved4[3]; /*Reserved */
	volatile uint32_t ESPIC3CFG;    /*Channel-3(Flash Access) Configuration Register */
	volatile uint32_t ESPIC3CFG2;   /*Channel-3(Flash Access) Configuration2 Register*/
	volatile uint32_t Reserved5[2]; /*Reserved */
	volatile uint32_t ESPISTA;      /*Status Register */
	volatile uint32_t ESPIRF;       /*Reset Flag Register */
	volatile uint16_t ESPIGEF;      /*Global Error Flag Register */
	volatile uint16_t Reserved6;    /*Reserved */
	volatile uint32_t ESPICMD;      /*Command Register */
	volatile uint32_t ESPICRC;      /*CRC Register */
	volatile uint32_t ESPIRST;      /*Reset Register */
};

/**
 *  Structure type to access ESPI Virtual Wire (ESPIVW).
 */
struct espivw_regs {
	volatile uint8_t ESPIVWIE;       /*Interrupt Enable Register */
	volatile uint8_t Reserved0[3];   /*Reserved */
	volatile uint8_t ESPIVWPF;       /*Event Pending Flag Register */
	volatile uint8_t Reserved1[3];   /*Reserved */
	volatile uint8_t ESPIVWEF;       /*Error Flag Register */
	volatile uint8_t Reserved2[3];   /*Reserved */
	volatile uint16_t ESPIVWIRQ;     /*IRQ Configuration Register */
	volatile uint16_t Reserved3;     /*Reserved */
	volatile uint32_t ESPISVWD;      /*System Event Direction Register */
	volatile uint32_t ESPIVWB10;     /*Block1/0 Register */
	volatile uint32_t ESPIVWB32;     /*Block3/2 Register */
	volatile uint32_t Reserved4;     /*Reserved */
	volatile uint16_t ESPIVWTX;      /*Tx Register */
	volatile uint16_t Reserved5;     /*Reserved */
	volatile uint8_t ESPIVWRXI;      /*Rx Index Register */
	volatile uint8_t ESPIVWRXV;      /*Rx Valid Flag Register */
	volatile uint16_t Reserved6;     /*Reserved */
	volatile uint32_t ESPIVWCNT;     /*Counter Value Register */
	volatile uint32_t Reserved7[5];  /*Reserved */
	volatile uint8_t ESPIVWTF[32];   /*Tx Flag Register */
	volatile uint8_t ESPIVWRF[32];   /*Rx Flag Register */
	volatile uint32_t Reserved8[32]; /*Reserved */
	volatile uint8_t ESPIVWIDX[256]; /*Index Table Register */
};

/**
 *  Structure type to access ESPI Out-Of-Band (ESPIOOB).
 */
struct espioob_regs {
	volatile uint8_t ESPIOOBIE;      /*Interrupt Enable Register */
	volatile uint8_t Reserved0[3];   /*Reserved */
	volatile uint8_t ESPIOOBPF;      /*Event Pending Flag Register */
	volatile uint8_t Reserved1[3];   /*Reserved */
	volatile uint8_t ESPIOOBEF;      /*Error Flag Register */
	volatile uint8_t Reserved2[3];   /*Reserved */
	volatile uint32_t Reserved3;     /*Reserved */
	volatile uint16_t ESPIOOBRL;     /*Rx Length Register */
	volatile uint16_t Reserved4;     /*Reserved */
	volatile uint16_t ESPIOOBTX;     /*Tx Control Register */
	volatile uint16_t Reserved5;     /*Reserved */
	volatile uint8_t ESPIOOBPECC;    /*PEC Register */
	volatile uint8_t Reserved6[3];   /*Reserved */
	volatile uint32_t Reserved7;     /*Reserved */
	volatile uint8_t ESPIOOBDAT[73]; /*Data Register */
};

/**
 *  Structure type to access ESPI Flash Access (ESPIFA).
 */
struct espifa_regs {
	volatile uint8_t ESPIFAIE;      /*Interrupt Enable Register */
	volatile uint8_t Reserved0[3];  /*Reserved */
	volatile uint8_t ESPIFAPF;      /*Event Pending Flag Register */
	volatile uint8_t Reserved1[3];  /*Reserved */
	volatile uint8_t ESPIFAEF;      /*Error Flag Register */
	volatile uint8_t Reserved2[3];  /*Reserved */
	volatile uint32_t Reserved3;    /*Reserved */
	volatile uint32_t ESPIFABA;     /*Base Address Register */
	volatile uint8_t ESPIFACNT;     /*Count Register */
	volatile uint8_t Reserved4[3];  /*Reserved */
	volatile uint16_t ESPIFALEN;    /*Completion length Register */
	volatile uint16_t Reserved5;    /*Reserved */
	volatile uint8_t ESPIFAPTCL;    /*Protocol Issue Register */
	volatile uint8_t Reserved6[3];  /*Reserved */
	volatile uint8_t ESPIFADAT[64]; /*Data Register */
};

/* ESPI */
#define ESPI_FREQ_POS        16
#define ESPI_ALERT_POS       19
#define ESPI_IOMODE_POS      24
#define ESPI_IOMODE_MASK     0x03
#define ESPI_CH_SUPPORT_MASK 0x0F

#define ESPI_RST_GPIO_Num 0x07

#define ESPI_FREQ_MAX_20M 0
#define ESPI_FREQ_MAX_25M 1
#define ESPI_FREQ_MAX_33M 2
#define ESPI_FREQ_MAX_50M 3
#define ESPI_FREQ_MAX_66M 4

#define ESPI_IO_SINGLE           0
#define ESPI_IO_SINGLE_DUAL      1
#define ESPI_IO_SINGLE_QUAD      2
#define ESPI_IO_SINGLE_DUAL_QUAD 3

#define ESPI_ALERT_OD 1

#define ESPI_SUPPORT_ESPIPH  0x01
#define ESPI_SUPPORT_ESPIVW  0x02
#define ESPI_SUPPORT_ESPIOOB 0x04
#define ESPI_SUPPORT_ESPIFA  0x08

#define ESPI_CH0_READY  0x02
#define ESPI_CH0_ENABLE 0x01

#define ESPI_CH1_READY  0x02
#define ESPI_CH1_ENABLE 0x01

#define ESPI_CH2_READY  0x02
#define ESPI_CH2_ENABLE 0x01

#define ESPI_CH3_READY  0x02
#define ESPI_CH3_ENABLE 0x01

/* ESPI Peripheral */
#define ENE_ESPI_IO2RAM_SIZE_MAX 0x100

/* ESPI Virtual Wire (ESPIVW) */
#define ESPIVW_RX_EVENT 0x01
#define ESPIVW_TX_EVENT 0x02

#define ESPIVW_RX_VALID_FLAG 0x01

#define ESPIVW_VALIDBIT_POS  4
#define ESPIVW_VALIDBIT_MASK 0xF0

#define ESPIVW_TXINDEX_POS  8
#define ESPIVW_TXINDEX_MASK 0xFF

#define ESPIVW_BLK_BASE_POS  0
#define ESPIVW_BLK_BASE_MASK 0xFF
#define ESPIVW_BLK_DIR_POS   8
#define ESPIVW_BLK_DIR_MASK  0xFF

#define ESPIVW_INDEXBASE_MASK 0xF8
#define ESPIVW_INDEXNUM_MASK  0x07

#define ESPIVW_BLK_NUM           4
#define ESPIVW_SYSTEMEVNET_INDEX 0x00
#define ESPIVW_B0_INTERNAL_INDEX 0x40
#define ESPIVW_B1_INTERNAL_INDEX 0x48
#define ESPIVW_B2_INTERNAL_INDEX 0x80
#define ESPIVW_B3_INTERNAL_INDEX 0x88

#define ESPIVW_B0_BASE 0x40
#define ESPIVW_B1_BASE 0x48
#define ESPIVW_B2_BASE 0x60
#define ESPIVW_B3_BASE 0x88

#define ENE_IDX02_OFS (ESPIVW_SYSTEMEVNET_INDEX + 0x02)
#define ENE_IDX03_OFS (ESPIVW_SYSTEMEVNET_INDEX + 0x03)
#define ENE_IDX04_OFS (ESPIVW_SYSTEMEVNET_INDEX + 0x04)
#define ENE_IDX05_OFS (ESPIVW_SYSTEMEVNET_INDEX + 0x05)
#define ENE_IDX06_OFS (ESPIVW_SYSTEMEVNET_INDEX + 0x06)
#define ENE_IDX07_OFS (ESPIVW_SYSTEMEVNET_INDEX + 0x07)
#define ENE_IDX40_OFS (ESPIVW_B0_INTERNAL_INDEX + 0x00)
#define ENE_IDX41_OFS (ESPIVW_B0_INTERNAL_INDEX + 0x01)
#define ENE_IDX42_OFS (ESPIVW_B0_INTERNAL_INDEX + 0x02)
#define ENE_IDX47_OFS (ESPIVW_B0_INTERNAL_INDEX + 0x07)
#define ENE_IDX4A_OFS (ESPIVW_B1_INTERNAL_INDEX + 0x02)

#define ENE_IDX60_OFS (ESPIVW_B2_INTERNAL_INDEX + 0x00)
#define ENE_IDX61_OFS (ESPIVW_B2_INTERNAL_INDEX + 0x01)
#define ENE_IDX64_OFS (ESPIVW_B2_INTERNAL_INDEX + 0x04)
#define ENE_IDX67_OFS (ESPIVW_B2_INTERNAL_INDEX + 0x07)

/* ESPI Out-Of-Band (ESPIOOB) */
#define ESPIOOB_TX_EVENT      0x01
#define ESPIOOB_RX_EVENT      0x02
#define ESPIOOB_DISABLE_EVENT 0x80

#define ESPIOOB_TX_ISSUE       0x100
#define ENE_ESPIOOB_RXLEN_MASK 0x0FFF

#define ESPIOOB_MAX_TIMEOUT 2000 /* Unit: ms */
#define ESPIOOB_BUFSIZE     73
#define ESPIOOB_TX_MAX      69

/* ESPI Flash Access (ESPIFA) */
#define ESPIFA_TX_FINISH_EVENT            0x01
#define ESPIFA_WRITE_ERASE_COMPLETE_EVENT 0x02
#define ESPIFA_READ_COMPLETE_EVENT        0x04
#define ESPIFA_UNSUCCESS_EVENT            0x08
#define ESPIFA_DISABLE_EVENT              0x80

#define ESPIFA_WRITE 0x01
#define ESPIFA_READ  0x02
#define ESPIFA_ERASE 0x03

#define ESPI_FLASH_NP_AVAIL 0x2000

#define ESPIFA_BUFSIZE     64    /* Must be power of 2 */
#define ESPIFA_MAX_TIMEOUT 20000 /* Unit: ms */

#endif /* ENE_KB1200_ESPI_H */
