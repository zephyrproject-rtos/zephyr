/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB1200_FSMBM_H
#define ENE_KB1200_FSMBM_H

/**
 * Structure type to access Flexible SMBus Master (FSMBM).
 */
struct fsmbm_regs {
	volatile uint32_t FSMBMCFG;      /* Configuration Register */
	volatile uint8_t  FSMBMIE;       /* Interrupt Enable Register */
	volatile uint8_t  Reserved0[3];
	volatile uint8_t  FSMBMPF;       /* Event Pending Flag Register */
	volatile uint8_t  Reserved1[3];
	volatile uint8_t  FSMBMFRT;      /* Protocol Control Register */
	volatile uint8_t  Reserved2[3];
	volatile uint16_t FSMBMPEC;      /* PEC Value Register */
	volatile uint16_t Reserved3;
	volatile uint8_t  FSMBMSTS;      /* Status Register */
	volatile uint8_t  Reserved4[3];
	volatile uint8_t  FSMBMADR;      /* Slave Address Register */
	volatile uint8_t  Reserved5[3];
	volatile uint8_t  FSMBMCMD;      /* Command Register */
	volatile uint8_t  Reserved6[3];
	volatile uint8_t  FSMBMDAT[32];  /* Data Register */
	volatile uint8_t  FSMBMPRTC_P;   /* Protocol Register */
	volatile uint8_t  FSMBMPRTC_C;   /* Protocol Register */
	volatile uint16_t Reserved7;
	volatile uint8_t  FSMBMNADR;     /* HostNotify Slave Address Register */
	volatile uint8_t  Reserved8[3];
	volatile uint16_t FSMBMNDAT;     /* HostNotify Data Register */
	volatile uint16_t Reserved9;
};

#define FSMBM_NUM                       10

/* data->state */
#define STATE_IDLE                      0
#define STATE_SENDING                   1
#define STATE_RECEIVING                 2
#define STATE_COMPLETE                  3

/* PROTOCOL */
#define FLEXIBLE_PROTOCOL               0x7F

/* Error code */
#define FSMBM_NO_ERROR                  0x00
#define FSMBM_DEVICE_ADDR_NO_ACK        0x10
#define FSMBM_CMD_NO_ACK                0x12
#define FSMBM_DEVICE_DATA_NO_ACK        0x13
#define FSMBM_LOST_ARBITRATION          0x17
#define FSMBM_SMBUS_TIMEOUT             0x18
#define FSMBM_UNSUPPORTED_PRTC          0x19
#define FSMBM_SMBUS_BUSY                0x1A
#define FSMBM_STOP_FAIL                 0x1E
#define FSMBM_PEC_ERROR                 0x1F
#define FSMBM_SDA_TIMEOUT               0x80
#define FSMBM_MAX_TIMEOUT               200         /* Unit: ms */

/* Packet Form */
#define FRT_NONE                        0x00
#define FRT_STOP                        0x01
#define FRT_PEC                         0x02
#define FRT_CNT                         0x04
#define FRT_CMD                         0x08
#define FRT_PEC_STOP                    (FRT_PEC | FRT_STOP)
#define FRT_CNT_STOP                    (FRT_CNT | FRT_STOP)
#define FRT_CNT_PEC                     (FRT_CNT | FRT_PEC)
#define FRT_CNT_PEC_STOP                (FRT_CNT | FRT_PEC | FRT_STOP)
#define FRT_CMD_STOP                    (FRT_CMD | FRT_STOP)
#define FRT_CMD_PEC                     (FRT_CMD | FRT_PEC)
#define FRT_CMD_PEC_STOP                (FRT_CMD | FRT_PEC | FRT_STOP)
#define FRT_CMD_CNT                     (FRT_CMD | FRT_CNT)
#define FRT_CMD_CNT_STOP                (FRT_CMD | FRT_CNT | FRT_STOP)
#define FRT_CMD_CNT_PEC                 (FRT_CMD | FRT_CNT | FRT_PEC)
#define FRT_CMD_CNT_PEC_STOP            (FRT_CMD | FRT_CNT | FRT_PEC | FRT_STOP)

/* HW */
#define FSMBM_BUFFER_SIZE               0x20
#define FSMBM_MAXCNT                    0xFF

#define FSMBM_WRITE                     0x00
#define FSMBM_READ                      0x01

/* Clock Setting = 1 / (1u + (1u * N))  ,50% Duty Cycle */
#define FSMBM_CLK_1M                    0x0000
#define FSMBM_CLK_500K                  0x0101
#define FSMBM_CLK_333K                  0x0202
#define FSMBM_CLK_250K                  0x0303
#define FSMBM_CLK_200K                  0x0404
#define FSMBM_CLK_167K                  0x0505
#define FSMBM_CLK_143K                  0x0606
#define FSMBM_CLK_125K                  0x0707
#define FSMBM_CLK_111K                  0x0808
#define FSMBM_CLK_100K                  0x0909
#define FSMBM_CLK_91K                   0x0A0A
#define FSMBM_CLK_83K                   0x0B0B
#define FSMBM_CLK_71K                   0x0D0D
#define FSMBM_CLK_63K                   0x0F0F
#define FSMBM_CLK_50K                   0x1313
#define FSMBM_CLK_40K                   0x1818
#define FSMBM_CLK_30K                   0x2020
#define FSMBM_CLK_20K                   0x3131
#define FSMBM_CLK_10K                   0x6363
/* Other(non 50% Duty Cycle) */
#define FSMBM_CLK_400K                  0x0102
#define FSMBM_CLK_666K                  0x0001

#define FSMBM_COMPLETE_EVENT            0x01
#define FSMBM_HOST_NOTIFY_EVENT         0x02
#define FSMBM_BLOCK_FINISH_EVENT        0x04

#define FSMBM_FUNCTION_ENABLE           0x01
#define FSMBM_TIMEOUT_ENABLE            0x02
#define FSMBM_HW_RESET                  0x10

#define FSMBM_CLK_POS                   16
#define FSMBM_CLK_MASK                  0xFFFF
#define FSMBM_STS_MASK                  0x1F

#endif /* ENE_KB1200_FSMBM_H */
