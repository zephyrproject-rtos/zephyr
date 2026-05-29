.. zephyr:code-sample:: nanopb
   :name: Nanopb

   Serialize and deserialize structured data using the nanopb module.

Overview
********

A simple protocol buffer sample using :ref:`nanopb_reference` for serializing structured data
to platform independent raw buffers or streams.

The structured data to encode/decode is presented as follows:

.. code-block:: proto

   syntax = "proto3";

   message SimpleMessage {
       int32 lucky_number = 1;
       bytes buffer = 2;
       int32 unlucky_number = 3;
   }

Configuration
*************

This sample uses two configuration options to modify the behavior.

* :kconfig:option:`CONFIG_SAMPLE_BUFFER_SIZE` sets the ``buffer`` field's size
* :kconfig:option:`CONFIG_SAMPLE_UNLUCKY_NUMBER` either enables or disables the ``unlucky_number``
  field.

Building and Running
********************

This application can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/nanopb
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:
