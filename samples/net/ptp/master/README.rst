.. _ptp-master-sample:

PTP Master Sample Application
#############################

Overview
********

The PTP master sample application for Zephyr allows boards to synchronize
a IEEE 1588-2008 (PTP V2) slaves running on the network to this board's time.

The source code for this sample application can be found at:
:zephyr_file:`samples/net/ptp/master`.

Requirements
************

This example requires one of the boards listed below and a IEEE 1588-2008
master, for example a Linux machine running a ``linuxptp`` server.

It is recommended to use a network card with hardware timestamping support
on the Linux machine.

Building and Running
********************

A good way to run this sample is to run this PTP master application with
an embedded device like NXP FRDM-K64F, Nucleo-H743-ZI, Nucleo-H745ZI-Q,
Nucleo-F767ZI or Atmel SAM-E70 Xplained. Note that PTP is only supported for
boards that have an Ethernet port and which has support for collecting
timestamps for sent and received Ethernet frames.

Follow these steps to build the PTP master sample application:

.. zephyr-app-commands::
   :zephyr-app: samples/net/ptp/master
   :board: <board to use>
   :goals: build
   :compact:

Once running on the same network as a IEEE 1588-2008 (PTP V2) slave, you will
observe slave synchronize to the clock of this board.

Quality of synchronization will depend on quality of the clock source, the
oscillator on the board and the network equipment.

Setting up Linux Host
=====================

On Linux Host, use ``linuxptp`` project.

Get linuxptp project sources

.. code-block:: console

    git clone git://git.code.sf.net/p/linuxptp/code

Compile the ``ptp4l`` daemon and start it like this:

.. code-block:: console

    sudo ./ptp4l -2 -f default.cfg -i eth0 -m -s

Here, ``eth0`` is the name of the interface on the same network as the board
running an example code.
