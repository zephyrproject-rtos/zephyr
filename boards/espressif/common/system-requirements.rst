:orphan:

.. espressif-system-requirements

Binary Blobs
============

Espressif HAL requires RF binary blobs in order work. Run the command
below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.
