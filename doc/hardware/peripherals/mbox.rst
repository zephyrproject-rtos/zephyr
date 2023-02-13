.. _mbox_api:

MBOX
####

Overview
********

An MBOX device is a peripheral capable of passing signals (and data depending
on the peripheral) between CPUs and clusters in the system. Each MBOX instance
is providing one or more channels, each one targeting one other CPU cluster
(multiple channels can target the same cluster).


API Reference
*************

.. doxygengroup:: mbox_interface
