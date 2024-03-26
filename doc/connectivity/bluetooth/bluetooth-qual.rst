.. _bluetooth-qual:

Bluetooth Qualification
#######################

Qualification setup
*******************

The Zephyr Bluetooth host can be qualified using Bluetooth's PTS (Profile Tuning
Suite) software. It is originally a manual process, but is automated by using
the `AutoPTS automation software <https://github.com/auto-pts/auto-pts>`_.

The setup is described in more details in the pages linked below.

.. toctree::
   :maxdepth: 1

   autopts/autopts-win10.rst
   autopts/autopts-linux.rst


Qualification Listings
**********************

The Zephyr BLE stack has obtained qualification listings for both the Host
and the Controller.
See the tables below for a list of qualification listings

Host qualifications
===================

.. list-table::
   :header-rows: 1

   * - Zephyr version
     - Link
     - Qualifying Company

   * - 2.2.x
     - `QDID 151074 <https://launchstudio.bluetooth.com/ListingDetails/109287>`_
     - Demant A/S

   * - 1.14.x
     - `QDID 139258 <https://launchstudio.bluetooth.com/ListingDetails/95152>`__
     - The Linux Foundation

   * - 1.13
     - `QDID 119517 <https://launchstudio.bluetooth.com/ListingDetails/70189>`__
     - Nordic Semiconductor

Mesh qualifications
===================

.. list-table::
   :header-rows: 1

   * - Zephyr version
     - Link
     - Qualifying Company

   * - 1.14.x
     - `QDID 139259 <https://launchstudio.bluetooth.com/ListingDetails/95153>`__
     - The Linux Foundation

Controller qualifications
=========================

.. list-table::
   :header-rows: 1

   * - Zephyr version
     - Link
     - Qualifying Company
     - Compatible Hardware

   * - 2.2.x
     - `QDID 150092 <https://launchstudio.bluetooth.com/ListingDetails/108089>`__
     - Nordic Semiconductor
     - nRF52x

   * - 1.14.x
     - `QDID 135679 <https://launchstudio.bluetooth.com/ListingDetails/90777>`__
     - Nordic Semiconductor
     - nRF52x

   * - 1.9 to 1.13
     - `QDID 101395 <https://launchstudio.bluetooth.com/ListingDetails/25166>`__
     - Nordic Semiconductor
     - nRF52x

ICS Features
*************

The ICS features for each supported protocol & profile can be found in
the following documents:

.. toctree::
   :maxdepth: 1

   gap-pics.rst
   gatt-pics.rst
   l2cap-pics.rst
   sm-pics.rst
   rfcomm-pics.rst
   mesh-pics.rst
   dis-pics.rst
