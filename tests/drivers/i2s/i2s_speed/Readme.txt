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

MIMXRT1060-EVK[B/C]:
This board uses a single SAI and connects the TX and RX signals by shorting externally on the EVK.
These are the HW changes required to run this test on MIMXRT1060-EVK[B/C]:
- Remove jumper J99
- Short BCLK J23-pin5  (SAI1_RX_BCLK  / GPIO_AD_B1_11) to J23-pin23 (SAI1_TX_BCLK  / GPIO_AD_B1_14)
- Short SYNC J23-pin9  (SAI1_RX_SYNC  / GPIO_AD_B1_10) to J23-pin16 (SAI1_TX_SYNC  / GPIO_AD_B1_15)
- Short Data J23-pin11 (SAI1_RX_DATA0 / GPIO_AD_B1_12) to J23-pin17 (SAI1_TX_DATA0 / GPIO_AD_B1_13)

- NOTE: these connections cause contention with the following signals/features:
	- camera CSI_D[2-7]
