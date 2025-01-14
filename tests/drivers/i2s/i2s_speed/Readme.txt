i2s_speed Test
##################

Board-specific details:

MIMXRT1170_EVK:
This board uses CONFIG_I2S_TEST_SEPARATE_DEVICES=n and connects TX and RX blocks in one
SAI peripheral by shorting signals externally on the EVK.
These are the HW changes required to run this test:
	- Short BCLK J9-pin1  (SAI1_RX_BCLK)  to J9-pin11  (SAI1_TX_BCLK)
	- Short SYNC J9-pin5  (SAI1_RX_SYNC)  to J9-pin13  (SAI1_TX_SYNC)
	- Short Data J9-pin7  (SAI1_RX_DATA)  to J9-pin9   (SAI1_TX_DATA)

MIMXRT1170_EVKB (SCH-55139 Rev C/C1/C2):
This board uses CONFIG_I2S_TEST_SEPARATE_DEVICES=n and connects TX and RX blocks in one
SAI peripheral by shorting signals externally on the EVK.
These are the HW changes required to run this test:
	- Populate R2124, R2125
	- Remove J99, J100
	- Short Data J99-pin1 (SAI1_RX_DATA0) to J100-pin1 (SAI1_TX_DATA0)

FRDM-MCXN947:
This board uses CONFIG_I2S_TEST_SEPARATE_DEVICES=y and connects two SAI peripherals by shorting
signals externally on the EVK.  These are the HW changes required to run this test:
        - Short BCLK J1-pin9  (SAI1_RX_BCLK/P3_18) to J3-pin15 (SAI0_TX_BCLK/P2_6)
        - Short SYNC J1-pin13 (SAI1_RX_FS/P3_19)   to J3-pin13 (SAI0_TX_FS/P2_7)
        - Short Data J1-pin15 (SAI1_RXD0/P3_21)    to J3-pin7  (SAI0_TXD0/P2_2)
