#ifndef ZEPHYR_DRIVERS_CC1101_REGISTERS_H_
#define ZEPHYR_DRIVERS_CC1101_REGISTERS_H_

#define CC1101_IOCFG2_ADDR 0x00
union CC1101_IOCFG2 {
	uint8_t reg;
	struct {
		uint8_t GDO2_CFG: 6;
		uint8_t GDO2_INV: 1;
		uint8_t R0: 1;
	} __packed;
};

#define CC1101_IOCFG1_ADDR 0x01
union CC1101_IOCFG1 {
	uint8_t reg;
	struct {
		uint8_t GDO1_CFG: 6;
		uint8_t GDO1_INV: 1;
		uint8_t R0: 1;
	} __packed;
};

#define CC1101_IOCFG0_ADDR 0x02
union CC1101_IOCFG0 {
	uint8_t reg;
	struct {
		uint8_t GDO0_CFG: 6;
		uint8_t GDO0_INV: 1;
		uint8_t R0: 1;
	} __packed;
};

#define CC1101_FIFOTHR_ADDR 0x03
union CC1101_FIFOTHR {
	uint8_t reg;
	struct {
		uint8_t FIFO_THR: 4;
		uint8_t CLOSE_IN_RX: 2;
		uint8_t ADC_RETENTION: 1;
		uint8_t R0: 1;
	} __packed;
};

#define CC1101_SYNC1_ADDR 0x04
#define CC1101_SYNC0_ADDR 0x05
#define CC1101_PKTLEN_ADDR 0x06
#define CC1101_PKTCTRL1_ADDR 0x07
union CC1101_PKTCTRL1 {
	uint8_t reg;
	struct {
		uint8_t ADR_CHK: 2;
		uint8_t APPEND_STATUS: 1;
		uint8_t CRC_AUTOFLUSH: 1;
		uint8_t R0: 1;
		uint8_t PQT: 3;
	} __packed;
};

#define CC1101_PKTCTRL0_ADDR 0x08
union CC1101_PKTCTRL0 {
	uint8_t reg;
	struct {
		uint8_t LENGTH_CONFIG: 2;
		uint8_t CRC_EN: 1;
		uint8_t R0: 1;
		uint8_t PKT_FORMAT: 2;
		uint8_t WHITE_DATA: 1;
		uint8_t R1: 1;
	} __packed;
};

#define CC1101_ADDR_ADDR 0x09
#define CC1101_CHANNR_ADDR 0x0A
#define CC1101_FSCTRL1_ADDR 0x0B
union CC1101_FSCTRL1 {
	uint8_t reg;
	struct {
		uint8_t FREQ_IF: 5;
		uint8_t R0: 3;
	} __packed;
};

#define CC1101_FSCTRL0_ADDR 0x0C
#define CC1101_FREQ2_ADDR 0x0D
union CC1101_FREQ2 {
	uint8_t reg;
	struct {
		uint8_t FREQ: 6;
		uint8_t R0: 2;
	} __packed;
};

#define CC1101_FREQ1_ADDR 0x0E
#define CC1101_FREQ0_ADDR 0x0F
#define CC1101_MDMCFG4_ADDR 0x10
union CC1101_MDMCFG4 {
	uint8_t reg;
	struct {
		uint8_t DRATE_E: 4;
		uint8_t CHANBW_M: 2;
		uint8_t CHANBW_E: 2;
	} __packed;
};

#define CC1101_MDMCFG3_ADDR 0x11
#define CC1101_MDMCFG2_ADDR 0x12
union CC1101_MDMCFG2 {
	uint8_t reg;
	struct {
		uint8_t SYNC_MODE: 3;
		uint8_t MANCHESTER_EN: 1;
		uint8_t MOD_FORMAT: 3;
		uint8_t DEM_DCFILT_OFF: 1;
	} __packed;
};

#define CC1101_MDMCFG1_ADDR 0x13
union CC1101_MDMCFG1 {
	uint8_t reg;
	struct {
		uint8_t CHANSPC_E: 2;
		uint8_t R0: 2;
		uint8_t NUM_PREAMBLE: 3;
		uint8_t FEC_EN: 1;
	} __packed;
};

#define CC1101_MDMCFG0_ADDR 0x14
#define CC1101_DEVIATN_ADDR 0x15
union CC1101_DEVIATN {
	uint8_t reg;
	struct {
		uint8_t DEVIATION_M: 3;
		uint8_t R0: 1;
		uint8_t DEVIATION_E: 3;
		uint8_t R1: 1;
	} __packed;
};

#define CC1101_MCSM2_ADDR 0x16
union CC1101_MCSM2 {
	uint8_t reg;
	struct {
		uint8_t RX_TIME: 3;
		uint8_t RX_TIME_QUAL: 1;
		uint8_t RX_TIME_RSSI: 1;
		uint8_t R0: 3;
	} __packed;
};

#define CC1101_MCSM1_ADDR 0x17
union CC1101_MCSM1 {
	uint8_t reg;
	struct {
		uint8_t TXOFF_MODE: 2;
		uint8_t RXOFF_MODE: 2;
		uint8_t CCA_MODE: 2;
		uint8_t R0: 2;
	} __packed;
};

#define CC1101_MCSM0_ADDR 0x18
union CC1101_MCSM0 {
	uint8_t reg;
	struct {
		uint8_t XOSC_FORCE_ON: 1;
		uint8_t PIN_CTRL_EN: 1;
		uint8_t PO_TIMEOUT: 2;
		uint8_t FS_AUTOCAL: 2;
		uint8_t R0: 2;
	} __packed;
};

#define CC1101_FOCCFG_ADDR 0x19
union CC1101_FOCCFG {
	uint8_t reg;
	struct {
		uint8_t FOC_LIMIT: 2;
		uint8_t FOC_POST_K: 1;
		uint8_t FOC_PRE_K: 2;
		uint8_t FOC_BS_CS_GATE: 1;
		uint8_t R0: 2;
	} __packed;
};

#define CC1101_BSCFG_ADDR 0x1A
union CC1101_BSCFG {
	uint8_t reg;
	struct {
		uint8_t BS_LIMIT: 2;
		uint8_t BS_POST_KP: 1;
		uint8_t BS_POST_KI: 1;
		uint8_t BS_PRE_KP: 2;
		uint8_t BS_PRE_KI: 2;
	} __packed;
};

#define CC1101_AGCCTRL2_ADDR 0x1B
union CC1101_AGCCTRL2 {
	uint8_t reg;
	struct {
		uint8_t MAGN_TARGET: 3;
		uint8_t MAX_LNA_GAIN: 3;
		uint8_t MAX_DVGA_GAIN: 2;
	} __packed;
};

#define CC1101_AGCCTRL1_ADDR 0x1C
union CC1101_AGCCTRL1 {
	uint8_t reg;
	struct {
		uint8_t CARRIER_SENSE_ABS_THR: 4;
		uint8_t CARRIER_SENSE_REL_THR: 2;
		uint8_t AGC_LNA_PRIORITY: 1;
		uint8_t R0: 1;
	} __packed;
};

#define CC1101_AGCCTRL0_ADDR 0x1D
union CC1101_AGCCTRL0 {
	uint8_t reg;
	struct {
		uint8_t FILTER_LENGTH: 2;
		uint8_t AGC_FREEZE: 2;
		uint8_t WAIT_TIME: 2;
		uint8_t HYST_LEVEL: 2;
	} __packed;
};

#define CC1101_WOREVT1_ADDR 0x1E
#define CC1101_WOREVT0_ADDR 0x1F
#define CC1101_WORCTRL_ADDR 0x20
union CC1101_WORCTRL {
	uint8_t reg;
	struct {
		uint8_t WOR_RES: 2;
		uint8_t R0: 1;
		uint8_t RC_CAL: 1;
		uint8_t EVENT1: 3;
		uint8_t RC_PD: 1;
	} __packed;
};

#define CC1101_FREND1_ADDR 0x21
union CC1101_FREND1 {
	uint8_t reg;
	struct {
		uint8_t MIX_CURRENT: 2;
		uint8_t LODIV_BUF_CURRENT_RX: 2;
		uint8_t LNA2MIX_CURRENT: 2;
		uint8_t LNA_CURRENT: 2;
	} __packed;
};

#define CC1101_FREND0_ADDR 0x22
union CC1101_FREND0 {
	uint8_t reg;
	struct {
		uint8_t PA_POWER: 3;
		uint8_t R0: 1;
		uint8_t LODIV_BUF_CURRENT_TX: 2;
		uint8_t R1: 2;
	} __packed;
};

#define CC1101_FSCAL3_ADDR 0x23
union CC1101_FSCAL3 {
	uint8_t reg;
	struct {
		uint8_t FSCAL3_0: 4;
		uint8_t CHP_CURR_CAL_EN: 2;
		uint8_t FSCAL3_1: 2;
	} __packed;
};

#define CC1101_FSCAL2_ADDR 0x24
union CC1101_FSCAL2 {
	uint8_t reg;
	struct {
		uint8_t FSCAL2: 5;
		uint8_t VCO_CORE_H_EN: 1;
		uint8_t R0: 2;
	} __packed;
};

#define CC1101_FSCAL1_ADDR 0x25
union CC1101_FSCAL1 {
	uint8_t reg;
	struct {
		uint8_t FSCAL1: 6;
		uint8_t R0: 2;
	} __packed;
};

#define CC1101_FSCAL0_ADDR 0x26
union CC1101_FSCAL0 {
	uint8_t reg;
	struct {
		uint8_t FSCAL0: 7;
		uint8_t R0: 1;
	} __packed;
};

#define CC1101_RCCTRL1_ADDR 0x27
union CC1101_RCCTRL1 {
	uint8_t reg;
	struct {
		uint8_t RCCTRL1: 7;
		uint8_t R0: 1;
	} __packed;
};

#define CC1101_RCCTRL0_ADDR 0x28
union CC1101_RCCTRL0 {
	uint8_t reg;
	struct {
		uint8_t RCCTRL0: 7;
		uint8_t R0: 1;
	} __packed;
};

#define CC1101_SRES_ADDR 0x30
#define CC1101_SFSTXON_ADDR 0x31
#define CC1101_SXOFF_ADDR 0x32
#define CC1101_SCAL_ADDR 0x33
#define CC1101_SRX_ADDR 0x34
#define CC1101_STX_ADDR 0x35
#define CC1101_SIDLE_ADDR 0x36
#define CC1101_SWOR_ADDR 0x38
#define CC1101_SPWD_ADDR 0x39
#define CC1101_SFRX_ADDR 0x3A
#define CC1101_SFTX_ADDR 0x3B
#define CC1101_SWORRST_ADDR 0x3C
#define CC1101_SNOP_ADDR 0x3D
#define CC1101_PARTNUM_ADDR 0x30
#define CC1101_VERSION_ADDR 0x31
#define CC1101_FREQEST_ADDR 0x32
#define CC1101_LQI_ADDR 0x33
union CC1101_LQI {
	uint8_t reg;
	struct {
		uint8_t LQI_EST: 7;
		uint8_t CRC_OK: 1;
	} __packed;
};

#define CC1101_RSSI_ADDR 0x34
#define CC1101_MARCSTATE_ADDR 0x35
union CC1101_MARCSTATE {
	uint8_t reg;
	struct {
		uint8_t MARC_STATE: 6;
		uint8_t R0: 2;
	} __packed;
};

#define CC1101_WORTIME1_ADDR 0x36
#define CC1101_WORTIME0_ADDR 0x37
#define CC1101_PKTSTATUS_ADDR 0x38
union CC1101_PKTSTATUS {
	uint8_t reg;
	struct {
		uint8_t GDO0: 1;
		uint8_t R0: 1;
		uint8_t GDO2: 1;
		uint8_t SFD: 1;
		uint8_t CCA: 1;
		uint8_t PQT_REACHED: 1;
		uint8_t CS: 1;
		uint8_t CRC_OK: 1;
	} __packed;
};

#define CC1101_VCO_VC_DAC_ADDR 0x39
#define CC1101_TXBYTES_ADDR 0x3A
union CC1101_TXBYTES {
	uint8_t reg;
	struct {
		uint8_t NUM_TXBYTES: 7;
		uint8_t TXFIFO_UNDERFLOW: 1;
	} __packed;
};

#define CC1101_RXBYTES_ADDR 0x3B
union CC1101_RXBYTES {
	uint8_t reg;
	struct {
		uint8_t NUM_RXBYTES: 7;
		uint8_t RXFIFO_OVERFLOW: 1;
	} __packed;
};

#define CC1101_RCCTRL1_STATUS_ADDR 0x3C
union CC1101_RCCTRL1_STATUS {
	uint8_t reg;
	struct {
		uint8_t RCCTRL1_STATUS: 7;
		uint8_t R0: 1;
	} __packed;
};

#define CC1101_RCCTRL0_STATUS_ADDR 0x3D
union CC1101_RCCTRL0_STATUS {
	uint8_t reg;
	struct {
		uint8_t RCCTRL0_STATUS: 7;
		uint8_t R0: 1;
	} __packed;
};

#define CC1101_PATABLE_ADDR 0x3E
#define CC1101_RX_FIFO_ADDR 0x3F
#define CC1101_TX_FIFO_ADDR 0x3F

#endif // ZEPHYR_DRIVERS_CC1101_REGISTERS_H_
