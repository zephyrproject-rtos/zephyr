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

The sample supports the following boards: :zephyr:board:`nrf52840dk`, :zephyr:board:`nrf5340dk`

The sample also requires an nPM2100 Evaluation Kit (EK) that you need to connect to the
development kit as described in `Wiring`_.

Wiring
******

With this configuration, the nPM2100 EK is wired to supply power to the DK.
This ensures that the TWI communication is at compatible voltage level, and represents a realistic use case for the nPM2100 PMIC.

.. note::

   To prevent leakage currents and program the DK, do not remove the USB connection.

   Unplug the battery from the nPM2100 EK and set the DK power switch to "OFF" while
   applying the wiring.
   If you have issues communicating with the DK or programming it after applying the wiring, try to power cycle the DK and EK.

To connect your DK to the nPM2100 EK, complete the following steps:

#. Prepare the DK for being powered by the nPM2100 EK:

   - Set switch **SW9** ("nRF power source") to position "VDD".
   - Set switch **SW10** ("VEXT -> VnRF") to position "ON".

#. Connect the TWI interface and power supply between the chosen DK and the nPM2100 EK
   as described in the following table:

   +------------------+-------+-------+-----------------------+-----+
   | nPM2100 EK pins  |  SDA  |  SCL  |         VOUT          | GND |
   +------------------+-------+-------+-----------------------+-----+
   | nRF52840 DK pins | P0.26 | P0.27 | P21 External supply + | GND |
   +------------------+-------+-------+-----------------------+-----+
   | nRF5340 DK pins  | P1.02 | P1.03 | P21 External supply + | GND |
   +------------------+-------+-------+-----------------------+-----+

#. Make the following connections on the nPM2100 EK:

   - Remove the USB power supply from the **J4** connector.
   - On the **P6** pin header, connect pins 1 and 2 with a jumper.
   - On the **BOOTMON** pin header, select **OFF** with a jumper.
   - On the **VSET** pin header, select **3.0V** with a jumper.
   - On the **VBAT SEL** switch, select **VBAT** position.
   - Connect a battery board to the **BATTERY INPUT** connector.

Building and Running
********************

To build the sample use the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/shields/npm2100_ek
   :board: nrf52840dk/nrf52840
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
   The BOOST regulator cannot provide a voltage lower than the battery voltage.

To get the GPIO status, run the following command:

.. code-block:: console

   uart:~$ gpio get npm2100_gpio 0
   0
