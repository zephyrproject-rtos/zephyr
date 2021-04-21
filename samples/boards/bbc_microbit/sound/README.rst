.. _microbit_sound:

BBC micro:bit sound
###################

Overview
********

This sample demonstrates how to use a piezo buzzer connected
to port P0 on the edge connector of the BBC micro:bit board.

Requirements
************

A separate piezo buzzer connected to the board. One example is the MI:Power
board that has a piezo buzzer in addition to a coin-cell battery. Resellers of
this board can be fairly easily found using online search.

Building and running
********************

The sample can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/bbc_microbit/sound
   :board: bbc_microbit
   :goals: build flash
   :compact:

Sample Output
=============

This sample outputs sounds through a connected piezo buzzer based on
button presses of the two main buttons. For each press the current
output frequency will be printed on the 5x5 LED display.
