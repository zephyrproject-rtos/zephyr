.. zephyr:code-sample:: ptp
   :name: PTP
   :relevant-api: ptp ptp_time ptp_clock

   Enable PTP support and monitor messages and events via logging.

Overview
********

The PTP sample application for Zephyr will enable PTP support.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/ptp`.

Requirements
************

For generic host connectivity, that can be used for debugging purposes, see
:ref:`networking_with_native_sim` for details.

Building and Running
********************

A good way to run this sample is to run this PTP application inside
native_sim board as described in :ref:`networking_with_native_sim` or with
embedded device like Nucleo-H743-ZI, Nucleo-H745ZI-Q, Nucleo-F767ZI or
Nucleo-H563ZI. Note that PTP is only supported for
boards that have an Ethernet port and which has support for collecting
timestamps for sent and received Ethernet frames.

Follow these steps to build the PTP sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/net/ptp
   :board: <board to use>
   :goals: build
   :compact:

Setting up Linux Host
=====================

By default PTP in Zephyr will not print any PTP debug messages to console.
One can enable debug prints by setting
:kconfig:option:`CONFIG_PTP_LOG_LEVEL_DBG` in the config file.

Get linuxptp project sources

.. code-block:: console

    git clone git://git.code.sf.net/p/linuxptp/code

Compile the ``ptp4l`` daemon and start it like this:

.. code-block:: console

    sudo ./ptp4l -4 -f -i zeth -m -q -l 6 -S

Compile Zephyr application.

.. zephyr-app-commands::
   :zephyr-app: samples/net/ptp
   :board: native_sim
   :goals: build
   :compact:

When the Zephyr image is build, you can start it like this:

.. code-block:: console

    build/zephyr/zephyr.exe -attach_uart
