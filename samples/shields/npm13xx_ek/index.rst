.. zephyr:code-sample:: npm13xx_ek
   :name: nPM13xx EK

   Interact with an nPM1300 or nPM1304 PMIC using the shell interface and buttons.

Overview
********

This sample is provided for evaluation of :ref:`npm1300_ek` and :ref:`npm1304_ek`. The sample
demonstrates use of Dynamic Voltage Scaling (DVS) regulator API feature which allows to switch
between chosen voltage levels or regulator modes by toggling configured GPIOs with a single
regulator API call.

It also provides a shell interface to support the features of the nPM1300 and nPM1304 PMICs,
including:

- Regulators (BUCK1/2, LDO1/2)
- GPIO

Requirements
************

The shield needs to be wired to a host board featuring the Arduino I2C connector. PMIC's GPIO3 acts
as an interrupt output, pins GPIO1 and GPIO2 are configured to control regulator modes (controlled
by the MCU software through DVS).

In addition, PMIC's VDDIO pin should be connected to an appropriate voltage reference.

E.g. for an nRF52 Series DK the wiring is as follows:

   +------------------+-------+-------+-------+-------+-------+-------+-----+
   | nPM13xx EK pins  |  SDA  |  SCL  | GPIO1 | GPIO2 | GPIO3 | VDDIO | GND |
   +------------------+-------+-------+-------+-------+-------+-------+-----+
   |  nRF52 DK pins   | P0.26 | P0.27 | P0.17 | P0.18 | P0.22 |  VDD  | GND |
   +------------------+-------+-------+-------+-------+-------+-------+-----+

Building and Running
********************

The sample is designed so that it can run on any platform (references to host GPIOs connected to
the nPM13xx EK need to be updated in the ``npm13xx.overlay`` files to reflect chosen HW).

For example, when building for the nRF52 DK and the nPM1300 EK, the following command can be used:

.. zephyr-app-commands::
   :zephyr-app: samples/shields/npm13xx_ek
   :board: nrf52dk/nrf52832
   :shield: npm1300_ek
   :goals: build
   :compact:

Testing
*******

After flashing the sample FW, you need to connect to the shell interface, where in case of
successful initialization you will see the nPM13xx PMIC status messages containing charger status
and battery voltage, current and temperature. Pressing and releasing the SHPHLD button on the EK
will generate interrupts and these events will also be printed in the shell.

Pressing a dedicated button on the host DK (Button 1 on an nRF52 Series DK) will cycle DVS states.

   +-------------------------------------+----------------+---------------+---------------+
   | **DVS state** (GPIO1 | GPIO2 level) | **BUCK2 mode** | **LDO1 mode** | **LDO2 mode** |
   +-------------------------------------+----------------+---------------+---------------+
   |        **0** (HIGH | HIGH)          |      OFF       |      OFF      |      OFF      |
   +-------------------------------------+----------------+---------------+---------------+
   |        **1** (LOW | HIGH)           |  ON retention  |      OFF      |      OFF      |
   +-------------------------------------+----------------+---------------+---------------+
   |        **2** (HIGH | LOW)           |      OFF       |      ON       |      ON       |
   +-------------------------------------+----------------+---------------+---------------+
   |         **3** (LOW | LOW)           |     ON PWM     |      ON       |      ON       |
   +-------------------------------------+----------------+---------------+---------------+

.. note::
   On an nRF52 Series DK, DVS pins are also used for the onboard LED1 and LED2, so you can observe
   the DVS state pins visually: an LED lights up when the respective pin is LOW

Regulators
**********

The ``regulator`` shell interface provides several subcommand to test
the regulators embedded in the PMIC. Below you can find some command examples.

.. code-block:: console

   # list all the supported voltages by BUCK1
   uart:~$ regulator vlist BUCK1
   1.000 V
   1.100 V
   ...

.. code-block:: console

   # enable BUCK2
   uart:~$ regulator enable BUCK2
   # disable BUCK2
   uart:~$ regulator disable BUCK2

.. code-block:: console

   # set BUCK2 voltage to exactly 2V
   uart:~$ regulator vset BUCK2 2V
   # obtain the actual BUCK1 configured voltage
   uart:~$ regulator vget BUCK1
   1.800 V
   # set BUCK1 voltage to a value between 2.35V and 2.45V
   uart:~$ regulator set BUCK1 2.35V 2.45V
   # obtain the actual BUCK1 configured voltage
   uart:~$ regulator get BUCK1
   2.400 V
