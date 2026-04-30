.. zephyr:board:: usbcanfd_Oleksii_g473

USBCANFD SOLO
#############

Adapter USBCANFD 1ch. up to 8 Mbps
can-module.com

.. figure:: img/G473.png
   :align: center
   :alt: USBCANFD SOLO

   USBCANFD SOLO

https://github.com/AlekseyMamontov/CANnectivity-CANFD-adapters

Overview
--------
USBCANFD SOLO is a professional CAN FD adapter based on the STM32G473 microcontroller.

Default Zephyr Peripheral Mapping:
----------------------------------

- CAN_RX/BOOT0 : PB8
- CAN_TX : PB9
- ledRX : PA6
- ledTX : PA5
- USB_DN : PA11
- USB_DP : PA12
- SWDIO : PA13
- SWCLK : PA14
- NRST : PG10

System Clock
------------
The FDCAN1 peripheral is driven by PLLQ, which has 80 MHz frequency.

.. _STM32G473 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32g473.html

.. _STM32G4 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
