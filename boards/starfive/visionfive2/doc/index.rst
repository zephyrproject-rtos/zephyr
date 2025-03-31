.. zephyr:board:: visionfive2

Overview
********

The StarFive VisionFive 2 is a development board with a StarFive JH7110
multi-core 64bit RISC-V SoC.

Programming and debugging
*************************

.. zephyr:board-supported-runners::

Building
========

Applications for the ``visionfive2`` board configuration can be built
as usual (see :ref:`build_an_application`) using the corresponding board name:

.. zephyr-app-commands::
   :board: visionfive2
   :goals: build

`spl_tool <https://github.com/starfive-tech/Tools/tree/master/spl_tool/>`_
is a jh7110 signature tool used to generate spl header information
and generate ``zephyr.bin.normal.out``.

.. code-block:: console

   ./spl_tool -c -f build/zephyr/zephyr.bin

This will create a new file ``build/zephyr/zephyr.bin.normal.out`` that can be flashed.
This step is necessary as zephyr binary must contain the SPL header info in order
to run it in M-Mode (Machine Mode) since S-Mode (Supervisor Mode) is
currently not supported.

Flashing
========

.. note::
   The following steps use minicom for serial communication, feel free to use
   any other serial terminal that supports xmodem based file transfers.
   Thanks to @orangecms for his vf2-loader tool which makes the flashing process easier

git clone the vf2-loader tool from https://github.com/orangecms/vf2-loader.git and
xmodem tool from https://github.com/orangecms/xmodem.rs.git side by side.

VisionFive2 uses uart for flashing. Refer to
`VisionFive2 Recovery Quick Start Guide
<https://doc-en.rvspace.org/VisionFive2/Quick_Start_Guide/VisionFive2_SDK_QSG/recovering_bootloader%20-%20vf2.html>`_
to connect your serial-to-usb converter. Now power on the board and using
minicom access board's serial and press the reset switch on the board until you see CCCCCC... prompt

Copy the ``zephyr.bin.normal.out`` from ``build/zephyr/zephyr.bin.normal.out``
to previously git cloned vf2-loader/ directory and cd into it.
Flash the ``zephyr.bin.normal.out`` using this command:

.. code-block:: console

   cargo run -- zephyr.bin.normal.out && minicom -D /dev/ttyUSB0

.. code-block:: text

   cargo run -- zephyr.bin.normal.out && minicom -D /dev/ttyUSB0
   Finished dev [unoptimized + debuginfo] target(s) in 0.03s
   Running `target/debug/vf2-loader zephyr.bin.normal.out`
   Welcome to minicom 2.7.1
   OPTIONS: I18n
   Compiled on Dec 23 2019, 02:06:26.
   Port /dev/ttyUSB0, 14:59:24
   Press CTRL-A Z for help on special keys
   6*** Booting Zephyr OS build v3.6.0-rc3 ***
   Hello World! visionfive2
