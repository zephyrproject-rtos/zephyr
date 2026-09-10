.. zephyr:board:: mikroe_quail

Overview
********
MikroE Quail for STM32 is a development board containing an `STM32F427`_
microcontroller. It is equipped with four mikroBUS sockets.
The edges of the board are lined with screw terminals and USB ports for
additional connectivity.

Hardware
********
The Quail board contains the following connections:

  - Four mikroBUS connectors
  - 32 screw terminals
  - two USB ports, one for programming and one for external storage

Furthermore the board contains three LEDs that are connected
to the microcontroller.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The four mikroBUS interfaces are aliased in the device tree so that their
peripherals can be accessed using ``mikrobus_N_INTERFACE`` so e.g. the SPI on
bus 2 can be found by the alias ``mikrobus_2_spi``. The numbering corresponds
with the marking on the board.

For connections on the edge connectors, please refer to `Quail for STM32 User Manual`_.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``mikroe_quail`` board can be built and flashed in the usual way
(see :ref:`build_an_application` and :ref:`application_run` for more details).


Flashing
========
The board ships with a locked flash, and will fail with the message:

.. code-block:: console

   Error: stm32x device protected

Unlocking with OpenOCD makes it possible to flash.

.. code-block:: console

   $ openocd -f /usr/share/openocd/scripts/interface/stlink-v2.cfg \
       -f /usr/share/openocd/scripts/target/stm32f4x.cfg -c init\
       -c "reset halt" -c "stm32f4x unlock 0" -c "reset run" -c shutdown

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mikroe_quail
   :goals: build flash

You should see the following message on the console:

.. code-block:: console

   Hello World! mikroe_quail


Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: mikroe_quail
   :maybe-skip-config:
   :goals: debug

References
**********

.. target-notes::

.. _Quail website:
    https://www.mikroe.com/quail
.. _Quail for STM32 User Manual:
    https://download.mikroe.com/documents/starter-boards/other/quail/quail-board-manual-v100.pdf
.. _STM32F427VIT6 Website:
    https://www.st.com/en/microcontrollers-microprocessors/stm32f427vi.html
.. _STM32F427:
    https://www.st.com/resource/en/datasheet/stm32f427vg.pdf
