.. _microbit_sound:

BBC micro:bit sound
###################

Overview
********

This is simple example demonstrating how to use a piezo buzzer connected
to port P0 on the edge connector of the BBC micro:bit board. Note that
the buzzer is not part of the main micro:bit board, rather it it needs
to be separately acquired and connected. A well working example is the
MI:Power board that has a piezo buzzer in addition to a coin-cell
battery. Resellers of this board can be fairly easily found using online
search.

Building
********

The sample can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/microbit/sound
   :board: bbc_microbit
   :goals: build
   :compact:

Sample Output
=============

This sample outputs sounds through a connected piezo buzzer based on
button presses of the two main buttons. For each press the current
output frequency will be printed on the 5x5 LED display.
