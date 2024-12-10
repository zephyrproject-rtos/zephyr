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

MIMXRT1060-EVKC:
This board uses CONFIG_I2S_TEST_SEPARATE_DEVICES=y and connects two SAI peripherals by shorting
signals externally on the EVK.  Due to contention with the signals, these changes may cause
some complications when debugging and booting.  One of the JTAG pins has contention, so SWD
should be used for debugging and programming the flash.  There are also conflicts with the
BOOT_MODE signals.  After this test app is programmed in the flash, if the MCU is reset while the
SAIs are driving the BOOT_MODE signals, the MCU may not boot properly.  Cycling power to the board
should boot the test properly.  If there are issues connecting through the debugger, force the MCU
to boot in SDP mode by setting the DIP switches SW4 to On-On-On-Off, and then cycle power to the
board.  Reprogram or erase the flash, and then revert SW4 to On-On-Off-On to boot from flash.

These are the HW changes required to run this test on MIMXRT1060-EVKC:
- Remove jumper J99 and resistor R220
- Populate R379 (or short the resistor pads)
- Short BCLK J33-pin2 (SAI1_RX_BCLK  / GPIO_AD_B1_11) to J56-pin2  (SAI2_TX_BCLK / GPIO_AD_B0_05)
- Short SYNC J33-pin1 (SAI1_RX_SYNC  / GPIO_AD_B1_10) to J57-pin2  (SAI2_TX_SYNC / GPIO_AD_B0_04)
- Short Data J99-pin1 (SAI1_RX_DATA0 / GPIO_AD_B1_12) to J19-pin10 (SAI2_TX_DATA / GPIO_AD_B0_09)

- NOTE: these connections cause contention with the following signals/features:
	- BOOT_MODE[0-1] on SW4
	- camera CSI_D[5-7]
	- JTAG_TDI
	- ENET_RST
