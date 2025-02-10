.. zephyr:code-sample:: morse_code
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

To use this sample, the following hardware is required:

* A board with a GPIO pin available. For convenience, it could be the exactly
  line which configure an LED in the board.
* A Smartphone with an application like `Morse Code Engineer`_.

Building and Running
********************

This sample should work on any board that have a timer counter implementation
and a free GPIO pin. For example, it can be run on the ``sam_v71_xult`` board
following the below step:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/misc/morse_code
   :board: sam_v71_xult
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

   [00:00:00.000,000] <inf> counter_sam_tc: Device tc@40010000 initialized
   *** Booting Zephyr OS build zephyr-v3.5.0 ***
   [00:00:02.532,000] <inf> app: Status: 0
   uart:~$ morse_code
   config  send
   uart:~$ morse_code config morse 200
   uart:~$ morse_code send morse "Zephyr is the best!!"
   [00:00:40.235,000] <inf> morse_code_shell: Status: 0
   uart:~$

.. _M.1677-International Morse Code: https://www.itu.int/rec/R-REC-M.1677-1-200910-I
.. _Morse Code Engineer: https://play.google.com/store/apps/details?id=com.gyokovsolutions.morsecodeengineer&hl=en_US
