.. _microbit_pong:

BBC micro:bit pong game
#######################

Overview
********

Play pong over as single player or Bluetooth between two micro:bit
devices.

The game works by controlling a paddle with the two buttons of the
micro:bit (labeled A and B). Initially the playing mode is selected: use
button A to toggle between single- and multi-player, and press button B
to select the current choice. To start the game, the player with the
ball launches the ball by pressing both buttons.

When multi-player mode has been selected the game will try to look for
and connect to a second micro:bit which has also been set into multi-
player mode.

If the board has a piezo buzzer connected to pin 0, this will be used to
generate beeps whenever the ball hits a wall or the paddle.

Building
********

.. zephyr-app-commands::
   :zephyr-app: samples/boards/bbc_microbit/pong
   :board: bbc_microbit
   :goals: build
   :compact:

