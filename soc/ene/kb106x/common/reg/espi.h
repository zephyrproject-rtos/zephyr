/*
 * Copyright (c) 2026 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB106X_ESPI_H
#define ENE_KB106X_ESPI_H

/**
 * brief  Structure type to access Enhanced Serial Peripheral Interface (ESPI).
 */
struct espi_regs {
	volatile uint32_t reserved_0;    /*Reserved */
	volatile uint32_t espi_id;       /*ID Register */
	volatile uint32_t espi_gencfg;   /*General Configuration Register */
	volatile uint32_t reserved_1;    /*Reserved */
	volatile uint32_t espi_c0cfg;    /*Channel-0(Peripheral) Configuration Register */
	volatile uint32_t reserved_2[3]; /*Reserved */
	volatile uint32_t espi_c1cfg;    /*Channel-1(Virtual Wire) Configuration Register */
	volatile uint32_t reserved_3[3]; /*Reserved */
	volatile uint32_t espi_c2cfg;    /*Channel-2(Out-Of-Band) Configuration Register */
	volatile uint32_t reserved_4[3]; /*Reserved */
	volatile uint32_t espi_c3cfg;    /*Channel-3(Flash Access) Configuration Register */
	volatile uint32_t espi_c3cfg2;   /*Channel-3(Flash Access) Configuration2 Register*/
	volatile uint32_t reserved_5[2]; /*Reserved */
	volatile uint32_t espi_sta;      /*Status Register */
	volatile uint32_t espi_rf;       /*Reset Flag Register */
	volatile uint16_t espi_gef;      /*Global Error Flag Register */
	volatile uint16_t reserved_6;    /*Reserved */
	volatile uint32_t espi_cmd;      /*Command Register */
	volatile uint32_t espi_crc;      /*CRC Register */
	volatile uint32_t espi_rst;      /*Reset Register */
};

/**
 * brief  Structure type to access ESPI Virtual Wire (ESPIVW).
 */
struct espivw_regs {
	volatile uint8_t espivw_ie;       /*Interrupt Enable Register */
	volatile uint8_t reserved_0[3];   /*Reserved */
	volatile uint8_t espivw_pf;       /*Event Pending Flag Register */
	volatile uint8_t reserved_1[3];   /*Reserved */
	volatile uint8_t espivw_ef;       /*Error Flag Register */
	volatile uint8_t reserved_2[3];   /*Reserved */
	volatile uint16_t espivw_irq;     /*IRQ Configuration Register */
	volatile uint16_t reserved_3;     /*Reserved */
	volatile uint32_t espivw_sed;     /*System Event Direction Register */
	volatile uint32_t espivw_b10;     /*Block1/0 Register */
	volatile uint32_t espivw_b32;     /*Block3/2 Register */
	volatile uint32_t reserved_4;     /*Reserved */
	volatile uint16_t espivw_tx;      /*Tx Register */
	volatile uint16_t reserved_5;     /*Reserved */
	volatile uint8_t espivw_rxi;      /*Rx Index Register */
	volatile uint8_t espivw_rxv;      /*Rx Valid Flag Register */
	volatile uint16_t reserved_6;     /*Reserved */
	volatile uint32_t espivw_cnt;     /*Counter Value Register */
	volatile uint32_t reserved_7[5];  /*Reserved */
	volatile uint8_t espivw_tf[32];   /*Tx Flag Register */
	volatile uint8_t espivw_rf[32];   /*Rx Flag Register */
	volatile uint32_t reserved_8[32]; /*Reserved */
	volatile uint8_t espivw_idx[256]; /*Index Table Register */
};

/**
 * brief  Structure type to access ESPI Out-Of-Band (ESPIOOB).
 */
struct espioob_regs {
	volatile uint8_t espioob_ie;      /*Interrupt Enable Register */
	volatile uint8_t reserved_0[3];   /*Reserved */
	volatile uint8_t espioob_pf;      /*Event Pending Flag Register */
	volatile uint8_t reserved_1[3];   /*Reserved */
	volatile uint8_t espioob_ef;      /*Error Flag Register */
	volatile uint8_t reserved_2[3];   /*Reserved */
	volatile uint32_t reserved_3;     /*Reserved */
	volatile uint16_t espioob_rl;     /*Rx Length Register */
	volatile uint16_t reserved_4;     /*Reserved */
	volatile uint16_t espioob_tx;     /*Tx Control Register */
	volatile uint16_t reserved_5;     /*Reserved */
	volatile uint8_t espioob_pec;     /*PEC Register */
	volatile uint8_t reserved_6[3];   /*Reserved */
	volatile uint32_t reserved_7;     /*Reserved */
	volatile uint8_t espioob_dat[73]; /*Data Register */
};

/**
 * brief  Structure type to access ESPI Flash Access (ESPIFA).
 */
struct espifa_regs {
	volatile uint8_t espifa_ie;      /*Interrupt Enable Register */
	volatile uint8_t reserved_0[3];  /*Reserved */
	volatile uint8_t espifa_pf;      /*Event Pending Flag Register */
	volatile uint8_t reserved_1[3];  /*Reserved */
	volatile uint8_t espifa_ef;      /*Error Flag Register */
	volatile uint8_t reserved_2[3];  /*Reserved */
	volatile uint32_t reserved_3;    /*Reserved */
	volatile uint32_t espifa_ba;     /*Base Address Register */
	volatile uint8_t espifa_cnt;     /*Count Register */
	volatile uint8_t reserved_4[3];  /*Reserved */
	volatile uint16_t espifa_len;    /*Completion length Register */
	volatile uint16_t reserved_5;    /*Reserved */
	volatile uint8_t espifa_ptcl;    /*Protocol Issue Register */
	volatile uint8_t reserved_6[3];  /*Reserved */
	volatile uint8_t espifa_dat[64]; /*Data Register */
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

#define ESPIOOB_BUFSIZE 73
#define ESPIOOB_TX_MAX  69

/* ESPI Flash Access (ESPIFA) */
#define ESPIFA_TX_FINISH_EVENT            0x01
#define ESPIFA_WRITE_ERASE_COMPLETE_EVENT 0x02
#define ESPIFA_READ_COMPLETE_EVENT        0x04
#define ESPIFA_UNSUCCESS_EVENT            0x08
#define ESPIFA_DISABLE_EVENT              0x80

#define ESPIFA_WRITE 0x01
#define ESPIFA_READ  0x02
#define ESPIFA_ERASE 0x03

#define ESPIFA_ACC_ADDR BIT(7)

#define ESPI_FLASH_NP_AVAIL 0x2000

#define ESPIFA_ENE_BUFSIZE 64 /* Must be power of 2 */

#endif /* ENE_KB106X_ESPI_H */
