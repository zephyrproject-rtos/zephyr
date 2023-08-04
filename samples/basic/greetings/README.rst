.. _greetings:

Greetings
#########

Overview
********

A simple sample that can be used with any :ref:`supported board <boards>` and
prints "Zephyr says 'Hi!'" to the console, once per second.

It's an alternative to :ref:`blinky <blinky-sample>` for boards with no
controllable LEDs.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/greetings
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

To build for another board, change "qemu_x86" above to that board's name.

Sample Output
=============

.. code-block:: console

    Zephyr says 'Hi!'
    Zephyr says 'Hi!'
    Zephyr says 'Hi!'
    Zephyr says 'Hi!'

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
