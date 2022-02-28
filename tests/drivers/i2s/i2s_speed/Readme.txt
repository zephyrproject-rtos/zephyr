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
