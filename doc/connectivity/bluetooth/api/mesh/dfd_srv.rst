.. _bluetooth_mesh_dfd_srv:

Firmware Distribution Server
############################

The Firmware Distribution Server model implements the Distributor role for the
:ref:`bluetooth_mesh_dfu` subsystem. It extends the :ref:`bluetooth_mesh_blob_srv`, which it uses to
receive the firmware image binary from the Initiator node. It also instantiates a
:ref:`bluetooth_mesh_dfu_cli`, which it uses to distribute firmware updates throughout the mesh
network.

.. note::

   Currently, the Firmware Distribution Server supports out-of-band (OOB) retrieval of firmware
   images over SMP service only.

The Firmware Distribution Server does not have an API of its own, but relies on a Firmware
Distribution Client model on a different device to give it information and trigger image
distribution and upload.

Firmware slots
**************

The Firmware Distribution Server is capable of storing multiple firmware images for distribution.
Each slot contains a separate firmware image with metadata, and can be distributed to other mesh
nodes in the network in any order. The contents, format and size of the firmware images are vendor
specific, and may contain data from other vendors. The application should never attempt to execute
or modify them.

The slots are managed remotely by a Firmware Distribution Client, which can both upload new slots
and delete old ones. The application is notified of changes to the slots through the Firmware
Distribution Server's callbacks (:cpp:type:`bt_mesh_fd_srv_cb`). While the metadata for each
firmware slot is stored internally, the application must provide a :ref:`bluetooth_mesh_blob_stream`
for reading and writing the firmware image.

API reference
*************

.. doxygengroup:: bt_mesh_dfd_srv
   :project: Zephyr
   :members:
