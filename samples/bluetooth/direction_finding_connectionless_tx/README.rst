.. _bluetooth_direction_finding_connectionless_tx:

Bluetooth: Direction Finding Periodic Advertising Beacon
########################################################

Overview
********

A simple application demonstrating the BLE Direction Finding CTE Broadcaster
functionality by sending Constant Tone Extension with periodic advertising PDUs.


Requirements
************

* nRF5340DK board
* nRF52833DK board with nRF52833 SOC
* nRF52833DK board with nRF52820 SOC
* antenna matrix for AoD (optional)

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/
direction_finding_connectionless_tx` in the Zephyr tree.

By default the application supports Angle of Arrival and Angle of Departure
mode.

To use Angle of Arrival mode only, build this application as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/direction_finding_connectionless_tx
   :host-os: unix
   :board: nrf52833dk_nrf52833
   :gen-args: -DOVERLAY_CONFIG=overlay-aoa.conf
   :goals: build flash
   :compact:

To run the application on nRF5340DK there must be provided a child image aimed to
run on network core. :zephyr_file:`samples/bluetooth/hci_rpmsg` sample application
may be used for that purpose.

To build :zephyr_file:`samples/bluetooth/hci_rpmsg` sample with direction finding
support enabled:

* create :zephyr_file:`samples/bluetooth/hci_rpmsg/boards/nrf5340dk_nrf5340_cpunet.overlay`
  with the same content as :zephyr_file:`samples/bluetooth/direction_finding_connectionless_tx/
  boards/nrf52833dk_nrf52833.overlay`
* create :zephyr_file:`samples/bluetooth/hci_rpmsg/boards/nrf5340dk_nrf5340_cpunet.conf`
  file. Copy content of :zephyr_file:`samples/bluetooth/direction_finding_connectionless_tx/boards/
  nrf52833dk_nrf52833.conf. Add CONFIG_BT_EXT_ADV=y configuration entry to enable extended size
  of :code:`CONFIG_BT_BUF_CMD_TX_SIZE` config to support LE Set Extended Advertising Data command.

See :ref:`bluetooth samples section <bluetooth-samples>` for common information
about bluetooth samples.

Antenna matrix configuration
****************************

To use this sample with Angle of Departure enabled, additional GPIOS configuration
is required to enable control of the antenna array. Example of such configuration
is provided in :zephyr_file:`samples/bluetooth/direction_finding_connectionless_tx/boards/
nrf52833dk_nrf52833.overlay`.

:zephyr_file:`samples/bluetooth/direction_finding_connectionless_tx/boards/
nrf52833dk_nrf52833.overlay` is a device tree overlay that provides information
about which GPIOS should be used by Radio peripheral to switch between antenna
patches during CTE transmission in AoD mode. At least two GPIOs must be provided
to enable antenna switching. GPIOS are used by Radio peripheral in order following
indices of :code:`dfegpio#-gpios` properties. The order is important because it
affects mapping of antenna switch patterns to GPIOS (see `Antenna patterns`_).

Antenna matrix configuration for nRF5340DK SOC is part of child image that is run
on network core. When :zephyr_file:`samples/bluetooth/hci_rpmsg` is used as network
core application, the antenna matrix configuration is stored in the :zephyr_file:`samples/
bluetooth/hci_rpmsg/boards/nrf5340dk_nrf5340_cpunet.overlay`.

Antenna patterns
****************
The antenna switch pattern is a binary number where each bit is applied to
a particular antenna GPIO pin. For example, the pattern 0x3 means that antenna
GPIOs at index 0,1 will be set, and following are left unset.

This also means that, for example, when using four GPIOs, the patterns count
cannot be greater than 16 and maximum allowed value is 15.

If the number of switch-sample periods is greater than the number of stored
switch patterns, then the radio loops back first pattern.

To successfully use Direction Finding Beacon with AoD mode enabled provide
following data related with owned antenna matrix design:

* provide GPIO pins to :code:`dfegpio#-gpios` properties in
  :zephyr_file:`samples/bluetooth/direction_finding_connectionless_tx/boards/nrf52833dk_nrf52833.overlay`
  file
* provide default antenna that will be used to transmit PDU :code:`dfe-pdu-ant`
  property in
  :zephyr_file:`samples/bluetooth/direction_finding_connectionless_tx/boards/nrf52833dk_nrf52833.overlay`
  file
* update antenna patterns in :cpp:var:`ant_patterns` array in
  :zephyr_file:`samples/bluetooth/direction_finding_connectionless_tx/src/main.c`.
