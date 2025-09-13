.. _ufs_api:

Universal Flash Storage (UFS)
#############################

The UFS Host Controller driver api offers a generic interface for interacting
with a UFS host controller device. It is used by the UFS subsystem, and is
not intended to be directly used by the application.

UFS Host Controller (UFSHC) Overview
************************************

The UFS host controller driver is based on SCSI Framework. UFSHC is a low
level device driver for UFS subsystem. The UFS subsystem acts as an interface
between SCSI and UFS Host controllers.

The current UFS subsystem implementation supports the following functionality:

UFS controller initialization
=============================

Prepares the UFS Host Controller to transfer commands/responses between UFSHC
and UFS device.

.. _ufs-init-api:

Transfer requests
=================

It handles SCSI commands from SCSI subsystem, form UPIUs and issues UPIUs to
the UFS Host controller.

.. _ufs-scsi-api:

Query Request
=============

It handles query requests which are used to modify and retrieve configuration
information of the device.

.. _ufs-query-api:

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_UFSHC`
* :kconfig:option:`CONFIG_UFS_STACK`
* :kconfig:option:`CONFIG_SCSI`

API Reference
*************

UFS subsystem APIs are provided by ufs.h

.. doxygengroup:: ufs_interface

UFS Host Controller (UFSHC) APIs are provided by ufshc.h

.. doxygengroup:: ufshc_interface
