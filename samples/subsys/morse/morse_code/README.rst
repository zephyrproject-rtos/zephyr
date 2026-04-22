.. zephyr:code-sample:: morse-morse_code
   :name: Morse Code
   :relevant-api: morse_code

   Send a text using Morse Code on a GPIO pin.

Overview
********

In this sample, a text can be sent using Morse Code driver. The Morse Code is
implemented based on the `M.1677-International Morse code`_ norm.

When the application starts, it automatically sends the text "PARIS". This word
is universally used to determine the number of Words Per Minute (wps). This
means that when using a speed of 20 it is possible to transmit the word "PARIS"
20 times in a minute.

Requirements
************

To use this sample with the GPIO backends, the following hardware is required:

* A board with a GPIO pin available. For convenience, it could be the exactly
  line which configure an LED in the board.
* A Smartphone with an application like `Morse Code Engineer`_.

Otherwise, to check the PWM backend, the following hardware is required:

* A board with a PWM-LED pin available.
* This sample does not provide an input driver.

The validation of the PWM can be use toguether with a speaker. In this case,
You can set the main frequency using the PWM period in ms. The output is a
square pwm wave. The ``nucleo_h753zi`` board configuration define a very small
characters per second which makes easy to visualize the "on" as a pwm blink
frequency.


Building and Running
********************

This sample should work on any board that have a timer counter implementation
and a free GPIO pin. For example, it can be run on the ``sam_v71_xult`` board
following the below step:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/morse/morse_code
   :board: sam_v71_xult/samv71q21
   :goals: flash
   :compact:


To build and run the PWM sample:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/morse/morse_code
   :board: nucleo_h753zi
   :goals: flash
   :compact:

Sample Output
=============

When the board boot the word "PARIS" will be transmitted automatically. The user
can use then the morse shell commands to interact with the example. By typing
morse_code <TAB> the commands will be print on the terminal.

The command config can be used to change the speed and the command send can be
used to transmit a text. A phrase can be sent using a text inside quotes.

In the below output it is possible to see the steps previously described.

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0  ***
   [00:00:00.059,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:00.060,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 1
   [00:00:00.060,000] <dbg> morse: morse_device_bit_state_handler: Now: 647, Ticks: 0
   [00:00:00.060,000] <dbg> morse: morse_rx_cb_handler: FSM: 0, bit: 1, ticks: 0, c: 0, t: 644
   [00:00:00.119,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:00.119,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 0
   [00:00:00.119,000] <dbg> morse: morse_device_bit_state_handler: Now: 1294, Ticks: 647
   [00:00:00.119,000] <dbg> morse: morse_rx_cb_handler: FSM: 1, bit: 0, ticks: 1, c: 647, t: 644
   [00:00:00.119,000] <dbg> morse: morse_rx_cb_handler: data: 0x00000001
   [00:00:00.179,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:00.179,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 1
   [00:00:00.179,000] <dbg> morse: morse_device_bit_state_handler: Now: 1941, Ticks: 647
   [00:00:00.179,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, bit: 1, ticks: 1, c: 647, t: 644
   [00:00:00.179,000] <dbg> morse: morse_rx_cb_handler: data: 0x00000002
   [00:00:00.239,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:00.298,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:00.358,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:00.358,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 0
   [00:00:00.358,000] <dbg> morse: morse_device_bit_state_handler: Now: 3882, Ticks: 1941
   [00:00:00.358,000] <dbg> morse: morse_rx_cb_handler: FSM: 1, bit: 0, ticks: 3, c: 1941, t: 644
   [00:00:00.358,000] <dbg> morse: morse_rx_cb_handler: data: 0x00000017
   [00:00:00.418,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:00.418,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 1
   [00:00:00.418,000] <dbg> morse: morse_device_bit_state_handler: Now: 4529, Ticks: 647
   [00:00:00.418,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, bit: 1, ticks: 1, c: 647, t: 644
   [00:00:00.418,000] <dbg> morse: morse_rx_cb_handler: data: 0x0000002e
   [00:00:00.477,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:00.537,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:00.597,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:00.597,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 0
   [00:00:00.597,000] <dbg> morse: morse_device_bit_state_handler: Now: 6470, Ticks: 1941
   [00:00:00.597,000] <dbg> morse: morse_rx_cb_handler: FSM: 1, bit: 0, ticks: 3, c: 1941, t: 644
   [00:00:00.597,000] <dbg> morse: morse_rx_cb_handler: data: 0x00000177
   [00:00:00.657,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:00.657,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 1
   [00:00:00.657,000] <dbg> morse: morse_device_bit_state_handler: Now: 7117, Ticks: 647
   [00:00:00.657,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, bit: 1, ticks: 1, c: 647, t: 644
   [00:00:00.657,000] <dbg> morse: morse_rx_cb_handler: data: 0x000002ee
   [00:00:00.716,000] <dbg> morse: morse_load: TX: 0x40, bits: 0x03000000
   [00:00:00.716,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:00.716,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 0
   [00:00:00.716,000] <dbg> morse: morse_device_bit_state_handler: Now: 7764, Ticks: 647
   [00:00:00.716,000] <dbg> morse: morse_rx_cb_handler: FSM: 1, bit: 0, ticks: 1, c: 647, t: 644
   [00:00:00.716,000] <dbg> morse: morse_rx_cb_handler: data: 0x000005dd
   [00:00:00.776,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:00.836,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:00.895,000] <dbg> morse: morse_load: Loading a idx: 1/4
   [00:00:00.895,000] <dbg> morse: morse_load: TX: 0x21, bits: 0x05000017
   [00:00:00.896,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:00.896,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 1
   [00:00:00.896,000] <dbg> morse: morse_device_bit_state_handler: Now: 9705, Ticks: 1941
   [00:00:00.896,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, bit: 1, ticks: 3, c: 1941, t: 644
   [00:00:00.896,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, data: 0x0b0005dd
   [00:00:00.896,000] <dbg> app: morse_rx_cb_handler: RX status: 0, 0x50, 'P'
   [00:00:00.955,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:00.955,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 0
   [00:00:00.955,000] <dbg> morse: morse_device_bit_state_handler: Now: 10352, Ticks: 647
   [00:00:00.955,000] <dbg> morse: morse_rx_cb_handler: FSM: 1, bit: 0, ticks: 1, c: 647, t: 644
   [00:00:00.955,000] <dbg> morse: morse_rx_cb_handler: data: 0x00000001
   [00:00:01.014,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:01.014,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 1
   [00:00:01.015,000] <dbg> morse: morse_device_bit_state_handler: Now: 10999, Ticks: 647
   [00:00:01.015,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, bit: 1, ticks: 1, c: 647, t: 644
   [00:00:01.015,000] <dbg> morse: morse_rx_cb_handler: data: 0x00000002
   [00:00:01.074,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:01.133,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:01.193,000] <dbg> morse: morse_load: TX: 0x40, bits: 0x03000000
   [00:00:01.193,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:01.193,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 0
   [00:00:01.193,000] <dbg> morse: morse_device_bit_state_handler: Now: 12940, Ticks: 1941
   [00:00:01.193,000] <dbg> morse: morse_rx_cb_handler: FSM: 1, bit: 0, ticks: 3, c: 1941, t: 644
   [00:00:01.193,000] <dbg> morse: morse_rx_cb_handler: data: 0x00000017
   [00:00:01.253,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:01.313,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:01.372,000] <dbg> morse: morse_load: Loading r idx: 2/4
   [00:00:01.373,000] <dbg> morse: morse_load: TX: 0x32, bits: 0x0700005d
   [00:00:01.373,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:01.373,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 1
   [00:00:01.373,000] <dbg> morse: morse_device_bit_state_handler: Now: 14881, Ticks: 1941
   [00:00:01.373,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, bit: 1, ticks: 3, c: 1941, t: 644
   [00:00:01.373,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, data: 0x05000017
   [00:00:01.373,000] <dbg> app: morse_rx_cb_handler: RX status: 0, 0x41, 'A'
   [00:00:01.432,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:01.432,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 0
   [00:00:01.432,000] <dbg> morse: morse_device_bit_state_handler: Now: 15528, Ticks: 647
   [00:00:01.432,000] <dbg> morse: morse_rx_cb_handler: FSM: 1, bit: 0, ticks: 1, c: 647, t: 644
   [00:00:01.432,000] <dbg> morse: morse_rx_cb_handler: data: 0x00000001
   [00:00:01.492,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:01.492,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 1
   [00:00:01.492,000] <dbg> morse: morse_device_bit_state_handler: Now: 16175, Ticks: 647
   [00:00:01.492,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, bit: 1, ticks: 1, c: 647, t: 644
   [00:00:01.492,000] <dbg> morse: morse_rx_cb_handler: data: 0x00000002
   [00:00:01.551,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:01.611,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:01.671,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:01.671,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 0
   [00:00:01.671,000] <dbg> morse: morse_device_bit_state_handler: Now: 18116, Ticks: 1941
   [00:00:01.671,000] <dbg> morse: morse_rx_cb_handler: FSM: 1, bit: 0, ticks: 3, c: 1941, t: 644
   [00:00:01.671,000] <dbg> morse: morse_rx_cb_handler: data: 0x00000017
   [00:00:01.730,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:01.730,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 1
   [00:00:01.730,000] <dbg> morse: morse_device_bit_state_handler: Now: 18763, Ticks: 647
   [00:00:01.730,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, bit: 1, ticks: 1, c: 647, t: 644
   [00:00:01.730,000] <dbg> morse: morse_rx_cb_handler: data: 0x0000002e
   [00:00:01.790,000] <dbg> morse: morse_load: TX: 0x40, bits: 0x03000000
   [00:00:01.790,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:01.790,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 0
   [00:00:01.790,000] <dbg> morse: morse_device_bit_state_handler: Now: 19410, Ticks: 647
   [00:00:01.790,000] <dbg> morse: morse_rx_cb_handler: FSM: 1, bit: 0, ticks: 1, c: 647, t: 644
   [00:00:01.790,000] <dbg> morse: morse_rx_cb_handler: data: 0x0000005d
   [00:00:01.849,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:01.909,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:01.969,000] <dbg> morse: morse_load: Loading i idx: 3/4
   [00:00:01.969,000] <dbg> morse: morse_load: TX: 0x29, bits: 0x03000005
   [00:00:01.969,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:01.969,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 1
   [00:00:01.969,000] <dbg> morse: morse_device_bit_state_handler: Now: 21351, Ticks: 1941
   [00:00:01.969,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, bit: 1, ticks: 3, c: 1941, t: 644
   [00:00:01.969,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, data: 0x0700005d
   [00:00:01.969,000] <dbg> app: morse_rx_cb_handler: RX status: 0, 0x52, 'R'
   [00:00:02.028,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:02.028,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 0
   [00:00:02.028,000] <dbg> morse: morse_device_bit_state_handler: Now: 21998, Ticks: 647
   [00:00:02.028,000] <dbg> morse: morse_rx_cb_handler: FSM: 1, bit: 0, ticks: 1, c: 647, t: 644
   [00:00:02.028,000] <dbg> morse: morse_rx_cb_handler: data: 0x00000001
   [00:00:02.088,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:02.088,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 1
   [00:00:02.088,000] <dbg> morse: morse_device_bit_state_handler: Now: 22645, Ticks: 647
   [00:00:02.088,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, bit: 1, ticks: 1, c: 647, t: 644
   [00:00:02.088,000] <dbg> morse: morse_rx_cb_handler: data: 0x00000002
   [00:00:02.147,000] <dbg> morse: morse_load: TX: 0x40, bits: 0x03000000
   [00:00:02.147,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:02.147,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 0
   [00:00:02.147,000] <dbg> morse: morse_device_bit_state_handler: Now: 23292, Ticks: 647
   [00:00:02.147,000] <dbg> morse: morse_rx_cb_handler: FSM: 1, bit: 0, ticks: 1, c: 647, t: 644
   [00:00:02.147,000] <dbg> morse: morse_rx_cb_handler: data: 0x00000005
   [00:00:02.207,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:02.267,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:02.327,000] <dbg> morse: morse_load: Loading s idx: 4/4
   [00:00:02.327,000] <dbg> morse: morse_load: TX: 0x33, bits: 0x05000015
   [00:00:02.327,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:02.327,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 1
   [00:00:02.327,000] <dbg> morse: morse_device_bit_state_handler: Now: 25233, Ticks: 1941
   [00:00:02.327,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, bit: 1, ticks: 3, c: 1941, t: 644
   [00:00:02.327,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, data: 0x03000005
   [00:00:02.327,000] <dbg> app: morse_rx_cb_handler: RX status: 0, 0x49, 'I'
   [00:00:02.386,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:02.386,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 0
   [00:00:02.386,000] <dbg> morse: morse_device_bit_state_handler: Now: 25880, Ticks: 647
   [00:00:02.386,000] <dbg> morse: morse_rx_cb_handler: FSM: 1, bit: 0, ticks: 1, c: 647, t: 644
   [00:00:02.386,000] <dbg> morse: morse_rx_cb_handler: data: 0x00000001
   [00:00:02.446,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:02.446,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 1
   [00:00:02.446,000] <dbg> morse: morse_device_bit_state_handler: Now: 26527, Ticks: 647
   [00:00:02.446,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, bit: 1, ticks: 1, c: 647, t: 644
   [00:00:02.446,000] <dbg> morse: morse_rx_cb_handler: data: 0x00000002
   [00:00:02.505,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:02.505,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 0
   [00:00:02.505,000] <dbg> morse: morse_device_bit_state_handler: Now: 27174, Ticks: 647
   [00:00:02.505,000] <dbg> morse: morse_rx_cb_handler: FSM: 1, bit: 0, ticks: 1, c: 647, t: 644
   [00:00:02.505,000] <dbg> morse: morse_rx_cb_handler: data: 0x00000005
   [00:00:02.565,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 1
   [00:00:02.565,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 1
   [00:00:02.565,000] <dbg> morse: morse_device_bit_state_handler: Now: 27821, Ticks: 647
   [00:00:02.565,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, bit: 1, ticks: 1, c: 647, t: 644
   [00:00:02.565,000] <dbg> morse: morse_rx_cb_handler: data: 0x0000000a
   [00:00:02.624,000] <dbg> morse: morse_dot_tick_handler: Finish transmission
   [00:00:02.624,000] <dbg> morse_gpio_tx: morse_gpio_tx_bit_state: state: 0
   [00:00:02.624,000] <dbg> morse_gpio_rx: morse_gpio_rx_isr_handler: state: 0
   [00:00:02.624,000] <dbg> morse: morse_device_bit_state_handler: Now: 28468, Ticks: 647
   [00:00:02.624,000] <inf> app: TX status: 0
   [00:00:02.624,000] <dbg> morse: morse_rx_cb_handler: FSM: 1, bit: 0, ticks: 1, c: 647, t: 644
   [00:00:02.624,000] <dbg> morse: morse_rx_cb_handler: data: 0x00000015
   [00:00:03.165,000] <dbg> morse: morse_word_blank_handler: RX Blank ticks: 5850
   [00:00:03.165,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, bit: 0, ticks: 9, c: 5850, t: 644
   [00:00:03.165,000] <dbg> morse: morse_rx_cb_handler: FSM: 2, data: 0x05000015
   [00:00:03.165,000] <dbg> app: morse_rx_cb_handler: RX status: 2, 0x53, 'S'
   [00:00:03.166,000] <inf> app: RX Data: PARIS
   uart:~$

.. _M.1677-International Morse Code:
    https://www.itu.int/rec/R-REC-M.1677-1-200910-I

.. _Morse Code Engineer:
    https://play.google.com/store/apps/details?id=com.gyokovsolutions.morsecodeengineer&hl=en_US
