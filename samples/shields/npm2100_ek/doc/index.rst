.. zephyr:code-sample:: npm2100_ek
   :name: nPM2100 EK

   Interact with the nPM2100 PMIC using the EK buttons and the shell interface.

Overview
********

This sample is provided for evaluation of the :ref:`npm2100_ek`.
It is an example of how the nPM2100 GPIO pins and the shell
interface can be used to control the following features:

- Regulators (BOOST, LDOSW)
- GPIO

Requirements
************

Make the following connections on the nPM2100 EK:

- Remove all existing connections.
- On the **BOOTMON** pin header, select **OFF** with a jumper.
- On the **VSET** pin header, select **3.0V** with a jumper.
- On the **VBAT SEL** pin header, select **VBAT** with a jumper.
- Connect a battery board to the **BATTERY INPUT** connector.

You need to wire the nPM2100 EK to the host board using the
Arduino I2C pins and a GND pin.
For example for an nRF52 Series DK, the wiring is as follows:

+------------------+-------+-------+-----+
| nPM2100 EK pins  | SDA   | SCL   | GND |
+------------------+-------+-------+-----+
| nRF52 DK pins    | P0.26 | P0.27 | GND |
+------------------+-------+-------+-----+

Building and Running
********************

The sample can run on any platform featuring an Arduino I2C connector.
For example, when building for an nRF52 Series DK, use the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/shields/npm2100_ek
   :board: nrf52dk/nrf52832
   :goals: build
   :compact:

.. note::
   This sample automatically sets the ``SHIELD`` to ``npm2100_ek``.
   Once you have flashed the software to your device, boot into the shell interface.
   Use the ``regulator`` command to test the PMIC.
   See the following section for details on the subcommands.

Regulator control
*****************

If the initialization was successful, the terminal displays the following message
with status information:

.. code-block:: console

   PMIC device ok

The sample also reports the battery and boost output voltages as well as the die
temperature measured every two seconds.

Use the buttons on the EK to control the regulators as follows:

+---------------------------+----------------------------------+
| Operation                 | Outcome                          |
+---------------------------+----------------------------------+
| Button **GPIO0** pressed  | BOOST output forced into HP mode |
+---------------------------+----------------------------------+
| Button **GPIO0** released | BOOST output operates in LP mode |
+---------------------------+----------------------------------+
| Button **GPIO1** pressed  | Load Switch on                   |
+---------------------------+----------------------------------+
| Button **GPIO1** released | Load Switch off                  |
+---------------------------+----------------------------------+

The ``regulator`` shell interface provides several subcommand to test
the regulators embedded in the PMIC.

To list all supported voltages for a regulator, run the following command:

.. code-block:: console

   uart:~$ regulator vlist BOOST
   1.800000 V
   1.850000 V
   ...

To enable or disable a regulator, run the following commands:

.. code-block:: console

   uart:~$ regulator enable LDOSW
   uart:~$ regulator disable LDOSW

.. note::
   The BOOST regulator is always enabled.

To set the output voltage of a regulator, run the following command:

.. code-block:: console

   uart:~$ regulator vset BOOST 2.5v
   uart:~$ regulator vget BOOST
   2.500000 V

.. note::
   The boost regulator cannot provide a voltage lower than the battery voltage.

To get the GPIO status, run the following command:

.. code-block:: console

   uart:~$ gpio get npm2100_gpio 0
   0
