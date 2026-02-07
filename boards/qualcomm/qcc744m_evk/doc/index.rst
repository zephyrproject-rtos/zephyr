.. zephyr:board:: qcc744m_evk

Overview
********

The QCC744M Evaluation board features the QCC744M module, which contains a QCC744-2 SoC with 4MB of
PSRAM and an external 8MB Flash chip.
Qualcomm QCC74x is a tri-radio chipset integrating 1x1 Wi-Fi 6, Bluetooth 5.4,
and IEEE 802.15.4 (Thread and Zigbee-ready) powered by a 32-bit RISC-V MCU up to 320 MHz, it is
based on, and mostly equivalent, to the Bouffalolab BL61x Serie of chipsets.

Hardware
********

For more information about the Qualcomm QCC74x MCU:

- `Qualcomm QCC74x MCU Datasheet`_
- `Qualcomm QCC74x MCU Programming Manual`_
- `Qualcomm QCC744M EVK Quick Start Guide`_
- `Qualcomm QCC74x SDK`_
- `qcc744m_evk Schematics`_

Supported Features
==================

.. zephyr:board-supported-hw::


Serial Port
===========

The ``qcc744m_evk`` board uses UART0 as default serial port.  It is connected
to the onboard USB Serial converter and the port is used for both program and console.


Programming and Debugging
*************************

Samples
=======

#. Build the Zephyr kernel and the :zephyr:code-sample:`hello_world` sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: qcc744m_evk
      :goals: build

#. ``west flash`` cannot flash QCC74x MCUs at the moment.
   You may first acquire 'QConn Flash' from the QCC74x SDK and follow these instructions:

   * Open Qconn Flash
   * Go to tab 'Flash Utils'
   * Select appropriate flash port and interface.
   * Reset board (press RESET, right button) while holding the BOOT (left) button to enter flash mode
   * In 'Flash Program', browse to ``bsp/board/qcc744dk/config/boot2_qcc743_isp_release_v8.1.9.bin`` from the SDK and set the address to 0x0
   * Press the 'Download' Button
   * Enter Flash mode again
   * In 'Flash Program', browse to zephyr.bin from your build folder and set the address to 0x2000
   * Press the 'Download' Button again.

   Your board is now flashed. Once the first binary has been flashed at 0x0, there is no need to re-flash it
   unless the flash area containing it is erased.

#. Run your favorite terminal program to listen for output. Under Linux the
   terminal should be :code:`/dev/ttyACM1`. For example:

   .. code-block:: console

      $ screen /dev/ttyACM1 115200

   Connection should be configured as follows:

      - Speed: 115200
      - Data: 8 bits
      - Parity: None
      - Stop bits: 1

   Then, press and release RESET button

   .. code-block:: console

      *** Booting Zephyr OS build v4.2.0 ***
      Hello World! qcc744m_evk/bl618m05q2i

Congratulations, you have ``qcc744m_evk`` configured and running Zephyr.


.. _Qualcomm QCC74x MCU Datasheet:
   https://docs.qualcomm.com/bundle/publicresource/topics/80-WL743-1

.. _Qualcomm QCC74x MCU Programming Manual:
   https://docs.qualcomm.com/bundle/publicresource/topics/80-58740-1/

.. _Qualcomm QCC744M EVK Quick Start Guide:
   https://docs.qualcomm.com/bundle/publicresource/topics/80-WL740-250/landingpage.html

.. _Qualcomm QCC74x SDK:
   https://git.codelinaro.org/clo/qcc7xx/QCCSDK-QCC74x

.. _qcc744m_evk Schematics:
   https://docs.qualcomm.com/bundle/publicresource/80-78831-41_REV_AC_QCC744M_Evaluation_Kit_Reference_Schematic.pdf
