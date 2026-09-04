.. zephyr:code-sample:: infineon-mbox
   :name: Infineon IPC MBOX
   :relevant-api: mbox_interface

   Ping-pong a counter between two cores using the Infineon IPC MBOX driver.

Overview
********

This sample demonstrates cross-core messaging on the Infineon inter-processor
communication (IPC) block through the :ref:`MBOX API <mbox_api>`, using the
register-based ``infineon,mbox`` driver.

Two cores run this same application and ping-pong a 32-bit counter. One core is
built as "core A" (``CONFIG_MBOX_SAMPLE_CORE_A=y``) and the other as "core B";
the two ends use opposite TX and RX channels:

* core A sends on channel 1 and receives on channel 0,
* core B sends on channel 0 and receives on channel 1.

Which physical core is core A, and how the mailbox channels map onto the IPC
block, are described entirely in the board overlay and its ``.conf`` fragment,
so the application itself stays device-agnostic. Each core sends on its TX
channel and receives on its RX channel; the receive callback prints the value
it got from the other core.

Building and Running
********************

The two cores are built and flashed as separate images. Use the ``-d`` option
to keep a distinct build directory per core.

This is a TF-M multicore configuration, so both images must be built with
``-DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y``. The CM33 image is built first; the
CM55 build then references it through ``-DPSE84_CM33_BUILD_DIR``, which must
match the CM33 (m33/ns) build directory. On this board the CM55 image is core A
and the CM33 image is core B; each core's ``boards/*.conf`` fragment selects
``CONFIG_MBOX_SAMPLE_CORE_A`` accordingly, so no extra option is needed.

.. code-block:: console

   # CM33 (secure/non-secure core) -- build first
   west build -b kit_pse84_eval/pse846gps2dbzc4a/m33/ns \
       samples/boards/infineon/mbox -d build_cm33 \
       -- -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y

   # CM55 (application core) -- references the CM33 build directory
   west build -b kit_pse84_eval/pse846gps2dbzc4a/m55 \
       samples/boards/infineon/mbox -d build_cm55 \
       -- -DCONFIG_PSOC_EDGE_M55_SRF_SUPPORT=y -DPSE84_CM33_BUILD_DIR=build_cm33

   # Flash the CM55 image first, then the CM33 image
   west flash -d build_cm55
   west flash -d build_cm33

.. note::

   If the CM55 does not have a valid application before the CM33 image is
   downloaded, the CM33 may report errors while trying to start it. Flash the
   CM55 image first.

Open a serial terminal (minicom, putty, etc.) connected to the board with the
following settings:

* Speed: 115200
* Data: 8 bits
* Parity: None
* Stop bits: 1

Both cores share the same console UART, so their output is interleaved on one
terminal. After reset the following (abbreviated) output appears:

.. code-block:: console

   *** Booting Zephyr OS ***
   Hello from APP [core A]
   Ping [core A] (on channel 1)
   Hello from APP [core B]
   Ping [core B] (on channel 0)
   Pong [core A] (on channel 0 [data 1, 4])
   Pong [core B] (on channel 1 [data 1, 4])
   Ping [core A] (on channel 1)
   Ping [core B] (on channel 0)
   Pong [core A] (on channel 0 [data 2, 4])
   Pong [core B] (on channel 1 [data 2, 4])
