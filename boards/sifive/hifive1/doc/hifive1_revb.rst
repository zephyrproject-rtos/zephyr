.. zephyr:board:: hifive1_revb

Overview
********

The HiFive1 Rev B is an Arduino-compatible development board with an FE310-G002 RISC-V SoC.

.. figure:: img/hifive1_revb.jpg
   :align: center
   :alt: SiFive HiFive1 Rev B board

   SiFive HiFive1 Rev B board (image courtesy of SiFive)

Programming and debugging
*************************

.. zephyr:board-supported-runners::

Building
========

Applications for the HiFive1 Rev B board configuration can be built as usual (see
:ref:`build_an_application`) using the corresponding board name:

.. zephyr-app-commands::
   :board: hifive1_revb
   :goals: build

Flashing
========

The HiFive 1 Rev B uses Segger J-Link OB for flashing and debugging. To flash and
debug the board, you'll need to install the
`Segger J-Link Software and Documentation Pack
<https://www.segger.com/downloads/jlink#J-LinkSoftwareAndDocumentationPack>`_
and choose version V6.46a or later (Downloads for Windows, Linux, and macOS are
available).

Now you can flash the application as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details):

.. code-block:: console

   west flash

Depending on your OS you might have to run the flash command as superuser.

Debugging
=========

Refer to the detailed overview about :ref:`application_debugging`.
