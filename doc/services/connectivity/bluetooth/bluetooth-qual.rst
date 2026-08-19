.. _bluetooth-qual:

Qualification
#############

Details on the Bluetooth SIG qualification process can be found at:
https://www.bluetooth.com/develop-with-bluetooth/qualify

Qualification setup
*******************

.. _AutoPTS automation software:
   https://github.com/auto-pts/auto-pts

The Zephyr Bluetooth host can be qualified using Bluetooth's PTS (Profile Tuning
Suite) software. It is originally a manual process, but is automated by using
the `AutoPTS automation software`_.

The setup is described in more details in the pages linked below.

.. toctree::
   :maxdepth: 1

   autopts/autopts-win10.rst
   autopts/autopts-linux.rst

ICS Features
************

.. _Bluetooth Qualification website:
   https://qualification.bluetooth.com/

The Zephyr ICS file for the Host features can be downloaded here:
:download:`ICS_Zephyr_Bluetooth_Host.pts
</tests/bluetooth/qualification/ICS_Zephyr_Bluetooth_Host.pts>`.

Use the `Bluetooth Qualification website`_ to view and edit the ICS.

Qualified releases
******************

.. _Bluetooth qualification listing 332380:
   https://qualification.bluetooth.com/ListingDetails/332380

The Zephyr Project provides a pre-qualified Bluetooth host stack to make it
easy for users to build qualified Bluetooth products. It can be included in
a product qualification design, providing feature coverage and reducing
qualification effort. The scope of qualification may vary by release, and
the details can be checked below.

.. list-table::
  :header-rows: 1

  * - Release
    - Design Number
    - Details
  * - v4.4
    - Q385945
    - `Bluetooth qualification listing 332380`_
