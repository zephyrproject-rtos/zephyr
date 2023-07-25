.. _microbit_display:

BBC micro:bit display
#####################

Overview
********
A simple example that demonstrates how to use the 5x5 LED matrix display
on the BBC micro:bit board.

Building
********

This project outputs various things on the BBC micro:bit display. It can
be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/boards/bbc_microbit/display
   :board: bbc_microbit
   :goals: build
   :compact:

Sample Output
=============

The sample app displays a countdown of the characters 9-0, iterates
through all pixels one-by-one, displays a smiley face, some animations,
and finally the text "Hello Zephyr!" by scrolling.
