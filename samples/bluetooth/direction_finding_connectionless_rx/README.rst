.. bluetooth_direction_finding_connectionless_rx:

Bluetooth: Direction Finding Periodic Advertising Locator
#########################################################

Overview
********

A simple application demonstrating the BLE Direction Finding CTE Locator
functionality by receiving and sampling sending Constant Tone Extension with
periodic advertising PDUs.

Requirements
************

* nRF5340DK board
* nRF52833DK board with nRF52833 SOC
* nRF52833DK board with nRF52820 SOC
* antenna matrix for AoA (optional)

Building and Running
********************

This sample can be found under :zephyr_file:`samples/bluetooth/direction_finding_connectionless_rx`
in the Zephyr tree.

By default the application supports Angle of Arrival and Angle of Departure mode.

To use Angle of Departure mode only, build this application as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/bluetooth/direction_finding_connectionless_rx
   :host-os: unix
   :board: nrf52833dk_nrf52833
   :gen-args: -DOVERLAY_CONFIG=overlay-aod.conf
   :goals: build flash
   :compact:

To run the application on nRF5340DK there must be provided a child image aimed to
run on network core. :zephyr_file:`samples/bluetooth/hci_rpmsg` sample application
may be used for that purpose.

To build :zephyr_file:`samples/bluetooth/hci_rpmsg` sample with direction finding
support enabled:

* create :zephyr_file:`samples/bluetooth/hci_rpmsg/boards/nrf5340dk_nrf5340_cpunet.overlay`with the same content as
  :zephyr_file:`samples/bluetooth/direction_finding_connectionless_rx/boards/nrf52833dk_nrf52833.overlay`
* create :zephyr_file:`samples/bluetooth/hci_rpmsg/boards/nrf5340dk_nrf5340_cpunet.conf`
  file. Copy content of
  :zephyr_file:`samples/bluetooth/direction_finding_connectionless_rx/boards/nrf52833dk_nrf52833.conf.
  Add CONFIG_BT_EXT_ADV=y configuration entry to enable extended size of
  :code:`CONFIG_BT_BUF_CMD_TX_SIZE` config to support LE Set Extended Advertising Data command.

See :ref:`bluetooth samples section <bluetooth-samples>` for common information
about bluetooth samples.

Antenna matrix configuration
****************************

To use this sample when Angle of Arrival mode is enabled, additional GPIOS configuration
is required to control the antenna array. Example of such configuration
is provided in devicetree overlay
:zephyr_file:`samples/bluetooth/direction_finding_connectionless_rx/boards/nrf52833dk_nrf52833.overlay`.

The overlay file provides the information about which GPIOs should be used by the Radio peripheral
to switch between antenna patches during the CTE transmission in the AoD mode. At least two GPIOs
must be provided to enable antenna switching.

The GPIOs are used by the Radio peripheral in order given by the :code:`dfegpio#-gpios` properties.
The order is important because it affects mapping of the antenna switching patterns to GPIOs
(see `Antenna patterns`_).

To successfully use the Direction Finding locator when the AoA mode is enabled, provide the
following data related to antenna matrix design:

* Provide the GPIO pins to :code:`dfegpio#-gpios` properties in
  :zephyr_file:`samples/bluetooth/direction_finding_connectionless_rx/boards/nrf52833dk_nrf52833.overlay`
  file
* Provide the default antenna that will be used to transmit PDU :code:`dfe-pdu-antenna` property in
  :zephyr_file:`samples/bluetooth/direction_finding_connectionless_rx/boards/nrf52833dk_nrf52833.overlay`
  file
* Update the antenna switching patterns in :cpp:var:`ant_patterns` array in
  :zephyr_file:`samples/bluetooth/direction_finding_connectionless_tx/src/main.c`..

Antenna matrix configuration for nRF5340DK SOC is part of child image that is run
on network core. When :zephyr_file:`samples/bluetooth/hci_rpmsg` is used as network
core application, the antenna matrix configuration is stored in the :zephyr_file:`samples/
bluetooth/hci_rpmsg/boards/nrf5340dk_nrf5340_cpunet.overlay`.

Antenna patterns
****************
The antenna switching pattern is a binary number where each bit is applied to a particular antenna
GPIO pin. For example, the pattern 0x3 means that antenna GPIOs at index 0,1 will be set, while
the following are left unset.

This also means that, for example, when using four GPIOs, the pattern count cannot be greater
than 16 and maximum allowed value is 15.

If the number of switch-sample periods is greater than the number of stored switching patterns,
then the radio loops back to the first pattern.
