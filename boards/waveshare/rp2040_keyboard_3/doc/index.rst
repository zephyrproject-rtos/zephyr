.. zephyr:board:: rp2040_keyboard_3

Overview
********

The `Waveshare 3-Key Shortcut Keyboard Development Board`_ is based on the RP2040 microcontroller
from Raspberry Pi Ltd. It has three keys with the default values "CTRL", "C" and "V". The board
is equipped with two USB type C connectors. There are two versions of the keyboard, where the
difference is in the housing (plastic or metal). This board definition can be used with both versions.


Hardware
********

- Microcontroller Raspberry Pi RP2040, with a max frequency of 133 MHz
- Dual ARM Cortex M0+ cores
- 264 kByte SRAM
- 2 Mbyte QSPI flash
- 3 user keys
- 3 RGB LEDs (Neopixels); one in each user key
- Dual USB type C connectors (use one at a time)
- RESET and BOOT buttons

The RESET and BOOT buttons are located on the back side of the board, on separate long edges
of the board. The BOOT button is located at the long edge with a USB connector, and the RESET
is located at the other long edge of the board.


Default Zephyr Peripheral Mapping
=================================

.. rst-class:: rst-columns

    - Button CTRL (left) : GPIO14
    - Button C (center) : GPIO13
    - Button V (right) : GPIO12
    - RGB LEDs: GPIO18

Note that no serial port pins (RX or TX) are exposed. By default this board uses USB for
terminal output.

See also `Waveshare P2040-Keyboard-3 wiki`_ and `schematic`_.


Supported Features
==================

.. zephyr:board-supported-hw::


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The board does not expose the SWDIO and SWCLK pins, so programming must be done via the USB port.
Press and hold the BOOT button, and then press the RESET button, and the device will appear as
a USB mass storage unit. Building your application will result in a :file:`build/zephyr/zephyr.uf2`
file. Drag and drop the file to the USB mass storage unit, and the board will be reprogrammed.

For more details on programming RP2040-based boards, see :zephyr:board:`rpi_pico` and especially
:ref:`rpi_pico_programming_and_debugging`.


Flashing
========

To run the :zephyr:code-sample:`led-strip` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/led/led_strip/
   :board: rp2040_keyboard_3
   :goals: build flash

Try also the :zephyr:code-sample:`dining-philosophers`, :zephyr:code-sample:`input-dump` and
:zephyr:code-sample:`button` samples.

Samples where text is printed only just at startup, for example :zephyr:code-sample:`hello_world`,
are difficult to use as the text is already printed once you connect to the newly created
USB console endpoint.

To run a program that acts as a keyboard (with the keys CTRL, C and V), use the
:zephyr:code-sample:`usb-hid-keyboard` sample with some modifications. First remove the line
``source "boards/common/usb/Kconfig.cdc_acm_serial.defconfig"`` from the
:zephyr_file:`boards/waveshare/rp2040_keyboard_3/Kconfig.defconfig` file. Then do the
modifications below to the :zephyr_file:`samples/subsys/usb/hid-keyboard/src/main.c` file.

Change:

.. code-block:: c

    case INPUT_KEY_0:
        if (kb_evt.value) {
            report[KB_KEY_CODE1] = HID_KEY_NUMLOCK;
        } else {
            report[KB_KEY_CODE1] = 0;
        }

        break;
    case INPUT_KEY_1:
        if (kb_evt.value) {
            report[KB_KEY_CODE2] = HID_KEY_CAPSLOCK;
        } else {
            report[KB_KEY_CODE2] = 0;
        }

        break;
    case INPUT_KEY_2:
        if (kb_evt.value) {
            report[KB_KEY_CODE3] = HID_KEY_SCROLLLOCK;
        } else {
            report[KB_KEY_CODE3] = 0;
        }

        break;

to:

.. code-block:: c

    case INPUT_KEY_LEFTCTRL:
        if (kb_evt.value) {
            report[KB_MOD_KEY] = HID_KBD_MODIFIER_LEFT_CTRL;
        } else {
            report[KB_MOD_KEY] = 0;
        }

        break;
    case INPUT_KEY_C:
        if (kb_evt.value) {
            report[KB_KEY_CODE1] = HID_KEY_C;
        } else {
            report[KB_KEY_CODE1] = 0;
        }

        break;
    case INPUT_KEY_V:
        if (kb_evt.value) {
            report[KB_KEY_CODE1] = HID_KEY_V;
        } else {
            report[KB_KEY_CODE1] = 0;
        }

        break;


References
**********

.. target-notes::

.. _Waveshare 3-Key Shortcut Keyboard Development Board:
    https://www.waveshare.com/rp2040-keyboard-3.htm

.. _Waveshare P2040-Keyboard-3 wiki:
    https://www.waveshare.com/wiki/RP2040-Keyboard-3

.. _schematic:
    https://files.waveshare.com/wiki/RP2040-Keyboard-3/RP2040-Keyboard-3-Schematic.pdf
