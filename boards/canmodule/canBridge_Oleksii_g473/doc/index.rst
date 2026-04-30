.. zephyr:board:: canBridge_Oleksii_g473



USB_CANFD_Bridge 
######################

CAN_Bridge  &  USBCANFD  3ch. up to 5 Mbps 
+ RS422 

can-module.com

.. figure:: img/CanBridge473.png
     :align: center
     :alt:  CanBridge_3ch

     CanBridge_3ch CANFD


https://github.com/AlekseyMamontov/CANnectivity-CANFD-adapters



Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- CAN_RX1: PB8
- CAN_TX2: PB9
- CAN_RX2: PB5
- CAN_TX2: PB6- 
  CAN_RX3: PB4
- CAN_TX3: PB3

- ledCAN1 : PC13
- LedCAN2 : PB7
- ledCAN3 : PA15
- LedSTAT : PA5

- USB_DN : PA11
- USB_DP : PA12

- RS422 TX : PA2
- RS422 RX : PA3

- SWDIO : PA13
- SWCLK : PA14
- NRST  : PG10

System Clock
------------
The FDCAN1,FDCAN2,FDCAN3  peripheral is driven by PLLQ, which has 80 MHz frequency.


.. _STM32G4 reference manual:
   https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
