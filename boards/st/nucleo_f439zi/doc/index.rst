.. zephyr:board:: nucleo_f439zi

Overview
********

The Nucleo F439ZI board features an ARM Cortex-M4 based STM32F439ZI MCU
with a wide range of connectivity support and configurations. This SoC
is basically a clone of the STM32F429ZI with a supplementary hardware
cryptographic accelerator.

More information about STM32F439ZI can be found here:

- `STM32F439ZI on www.st.com`_
- `STM32F439 reference manual`_
- `STM32F439 datasheet`_

This board provides the same features as the
:zephyr:board:`nucleo_f429zi`. You can refer to it for further
information.

Programming and Debugging
*************************

The Nucleo F439ZI board includes an ST-LINK/V2-1 embedded debug tool interface.

Flashing
========

The board is configured to be flashed using west `STM32CubeProgrammer`_ runner,
so its :ref:`installation <stm32cubeprog-flash-host-tools>` is required.

Alternatively, OpenOCD or JLink can also be used to flash the board using
the ``--runner`` (or ``-r``) option:

.. code-block:: console

   $ west flash --runner openocd
   $ west flash --runner jlink


.. target-notes::
.. _STM32F439ZI on www.st.com:
   https://www.st.com/en/microcontrollers/stm32f439zi.html

.. _STM32F439 reference manual:
   https://www.st.com/resource/en/reference_manual/dm00031020.pdf

.. _STM32F439 datasheet:
   https://www.st.com/resource/en/datasheet/stm32f439zi.pdf

.. _STM32CubeProgrammer:
   https://www.st.com/en/development-tools/stm32cubeprog.html
