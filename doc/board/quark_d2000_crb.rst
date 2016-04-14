.. _quark_d2000_crb:

Quark D2000 CRB
###############

Overview
********
The Intel® Quark ™ microcontroller D2000 package is shipped as a 40-pin QFN
component.

Intel™ Quark® microcontroller D2000 contains the following items:

- On-board components:

  - Accelerometer/Magnetometer sensor
  - UART/JTAG to USB convert for USB debug port

- Expansion options:

  - “Arduino Uno” compatible SIL sockets ( 3.3V IO Only )

- Other connectors:

  - 1x USB 2.0 Device Port – micro Type B
  - On-board coin cell battery holder
  - 5V input a screw terminal/header (external power or Li-ion)
  - EEMBC power input header

Board Layout
************

General information for the board can be found at the `Intel Website`_,
which includes both `schematics`_ and BRD files.

Supported Features
******************

+-----------+------------+-----------------------+
| Interface | Controller | Driver/Component      |
+===========+============+=======================+
| MVIC      | on-chip    | interrupt_controller  |
+-----------+------------+-----------------------+
| UART      | on-chip    | serial port-polling;  |
|           |            | serial port-interrupt |
+-----------+------------+-----------------------+
| SPI       | on-chip    | spi                   |
+-----------+------------+-----------------------+
| I2C       | on-chip    | i2c                   |
+-----------+------------+-----------------------+
| GPIO      | on-chip    | gpio                  |
+-----------+------------+-----------------------+
| PWM       | on-chip    | pwm                   |
+-----------+------------+-----------------------+


Developing for the D2000
************************

The D2000 board configuration details are found in the project's tree at
:file:`boards/quark_d2000_crb`.  The make target for this board is
quark_d2000_crb.

Building a Binary
-----------------

To build a Zephyr applications for the D2000 board, the ``BOARD`` configuration
option must be defined at compile time.  Follow these steps to build the
hello_world application as an example.

#. Source the :file:`zephyr-env.sh` file.

#. Change directories to the application directory.  For hello_world this is:

   .. code-block:: console

      $ cd $ZEPHYR_BASE/samples/hello_world/nanokernel

#. Build the binary:

   .. code-block:: console

      $ make pristine && make BOARD=quark_d2000_crb ARCH=x86

Flashing a Binary
-----------------

#. Since the board has a built-in JTAG; it is possible to flash the device
   through the USB only.  Set the following jumpers to enable the built-in JTAG:

   +--------+------+--------+------+------+
   | Jumper | UART | Common | JTAG | Name |
   +========+======+========+======+======+
   | J9     | Open |   X    |  X   | TDO  |
   +--------+------+--------+------+------+
   | J10    | Open |   X    |  X   | TDI  |
   +--------+------+--------+------+------+
   | J11    | Open |   X    |  X   | TRST |
   +--------+------+--------+------+------+
   | J12    |  X   |  N/A   |  X   | TMS  |
   +--------+------+--------+------+------+
   | J17    |  X   |  N/A   |  X   | TCK  |
   +--------+------+--------+------+------+

#. Connect the D2000 via USB to the host computer.

#. Once the binary is built, it can be flashed to the device by:

   .. code-block:: console

      $ make BOARD=quark_d2000_crb ARCH=x86 flash

Debugging a Binary
------------------

To debug an application on the D2000 platform, follow these steps.  As an
example, we are using the hello_world application.

#. Source the :file:`zephyr-env.sh` file.

#. Go to the application's folder:.

   .. code-block:: console

      $ cd $ZEPHYR_BASE/samples/hello_world/nanokernel

#. Verify the final binary is in :file:`outdir/zephyr.elf`.

#. If the binary is not there, please re-build using the steps described above.

#. To enable the debug process, enter:

   .. code-block:: console

      $ make BOARD=quark_d2000_crb ARCH=x86 debug


Bibliography
************

.. _Intel Website:
   http://www.intel.com/content/www/us/en/embedded/products/quark/mcu/d2000/quark-d2000-crb-user-guide.html

.. _schematics:
   http://www.intel.com/content/www/us/en/embedded/products/quark/mcu/d2000/quark-d2000-crb-schematics.html

