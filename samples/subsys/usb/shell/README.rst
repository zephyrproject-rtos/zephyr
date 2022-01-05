.. _usb_shell-app:

USB support shell sample
########################

Overview
********

The sample enables new experimental USB device support and the shell function.
It is primarily intended to aid in the development and testing of USB constoller
drivers and new USB support.

Building and flashing
*********************

Assuming the board has a supported USB device controller, the example can be
built like:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/shell
   :board: reel_board
   :goals: flash
   :compact:

Sample shell interaction
========================

.. code-block:: console

   uart:~$ usbd defaults
   dev: USB descriptors initialized
   uart:~$ usbd config add 1
   uart:~$ usbd class add foobaz 1
   dev: added USB class foobaz to configuration 1
   uart:~$ usbd init
   dev: USB initialized
   uart:~$ usbd enable
   dev: USB enabled
