.. zephyr:board:: usbcanfd_oleksii_g431

USBCANFD SOLO
#############

The USBCANFD SOLO is a 1-channel CAN-FD adapter supporting speeds up to 8 Mbps from can-module.com.

.. figure:: img/G431.jpg
   :align: center
   :alt: USBCANFD SOLO

   USBCANFD SOLO

Hardware and source code details can be found at the `CANnectivity-CANFD-adapters GitHub <https://github.com/AlekseyMamontov/CANnectivity-CANFD-adapters>`_.

Default Zephyr Peripheral Mapping
---------------------------------

.. rst-class:: rst-columns

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

The FDCAN1 peripheral is driven by PLLQ, which has a frequency of 80 MHz.

References
----------

.. _STM32G431C8 on www.st.com:
   https://www.st.com/en/microcontrollers-microprocessors/stm32g431c8.html

.. _STM32G4 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
