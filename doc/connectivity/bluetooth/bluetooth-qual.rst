.. _bluetooth-qual:

Qualification
#############

Qualification setup
*******************

.. _auto_pts_repository:
   https://github.com/auto-pts/auto-pts

The Zephyr Bluetooth host can be qualified using Bluetooth's PTS (Profile Tuning
Suite) software. It is originally a manual process, but is automated by using
the `AutoPTS automation software <auto_pts_repository>`_.

The setup is described in more details in the pages linked below.

.. toctree::
   :maxdepth: 1

   autopts/autopts-win10.rst
   autopts/autopts-linux.rst

ICS Features
************

.. _bluetooth_qualification_website:
   https://qualification.bluetooth.com/

The Zephyr ICS file for the Host features can be downloaded here:
:download:`ICS_Zephyr_Bluetooth_Host.pts
</tests/bluetooth/qualification/ICS_Zephyr_Bluetooth_Host.pts>`.

Use the `Bluetooth Qualification website <bluetooth_qualification_website>`_ to view and edit the ICS.
