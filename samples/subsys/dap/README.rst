.. zephyr:code-sample:: cmsis-dap
   :name: CMSIS-DAP

   Implement a custom CMSIS-DAP controller using SWDP interface driver.

Overview
********

This sample app demonstrates use of a SWDP interface driver and CMSIS DAP
controller through DAP USB backend.

Requirements
************

This sample supports multiple hardware configurations:

The simplest configuration would be to connect ``SWDIO`` to ``dio``, ``SWDCLK`` to ``clk``
and optionally ``nRESET`` to ``reset``.  The optional ``noe`` pin is used to enable the port,
e.g. if the SWD connections are multiplexed.

Building and Running
********************

The following commands build and flash DAP sample.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/dap
   :board: nrf52840dk_nrf52840
   :goals: flash
   :compact:

Connect HIC to the target and try some pyOCD commands, for example:

.. code-block:: console

   pyocd commander -t nrf52840

   0029527 W Board ID FE5D is not recognized [mbed_board]
   Connected to NRF52840 [Sleeping]: FE5D244DFE1F33DB
   pyocd> read32 0x20004f18 32
   20004f18:  20001160 2000244c 00000000 0000e407    | ..` .$L........|
   20004f28:  ffffffff ffffffff 00000000 aaaaaaaa    |................|
   pyocd> halt
   Successfully halted device
   pyocd> reg
   general registers:
         lr: 0x00009cdd                   r7: 0x00000000 (0)
         pc: 0x000033ca                   r8: 0x00000000 (0)
         r0: 0x00000000 (0)               r9: 0x00000000 (0)
         r1: 0x20002854 (536881236)      r10: 0x00000000 (0)
         r2: 0x20000be4 (536873956)      r11: 0x00000000 (0)
         r3: 0x00000000 (0)              r12: 0x00000000 (0)
         r4: 0x200017e8 (536877032)       sp: 0x20002898
         r5: 0x20001867 (536877159)     xpsr: 0x61000000 (1627389952)
         r6: 0x00000000 (0)
