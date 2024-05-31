i2s_speed Test
##################

Board-specific details:

MIMXRT1170_EVK:
This board uses CONFIG_I2S_TEST_SEPARATE_DEVICES=y and connects two SAI peripherals by shorting
signals externally on the EVK.  These are the HW changes required to run this test:
	- Remove jumper J8 and resistor R78
	- Short BCLK J9-pin1  (SAI1_RX_BCLK)  to J66-pin1  (SAI4_TX_BCLK)
	- Short SYNC J9-pin5  (SAI1_RX_SYNC)  to J64-pin1  (SAI4_TX_SYNC)
	- Short Data J61-pin1 (SAI1_RX_DATA)  to J63-pin1  (SAI4_TX_DATA)

FRDM-MCXN947:
This board uses CONFIG_I2S_TEST_SEPARATE_DEVICES=y and connects two SAI peripherals by shorting
signals externally on the EVK.  These are the HW changes required to run this test:
        - Short BCLK J1-pin9  (SAI1_RX_BCLK/P3_18) to J3-pin15 (SAI0_TX_BCLK/P2_6)
        - Short SYNC J1-pin13 (SAI1_RX_FS/P3_19)   to J3-pin13 (SAI0_TX_FS/P2_7)
        - Short Data J1-pin15 (SAI1_RXD0/P3_21)    to J3-pin7  (SAI0_TXD0/P2_2)
