.. _devicetree_api:

Devicetree
##########

This page contains reference documentation for ``<devicetree.h>``. See
:ref:`dt-guide` for an introduction. Use of these macros has no impact on
scheduling. They can be used from any calling context and at file scope.

Some of these require a special macro named ``DT_DRV_COMPAT`` to be defined
before they can be used; these are discussed individually below. These macros
are generally meant for use within device drivers.

Generic APIs
************

These APIs can be used anywhere.

Node identifiers
================

You can use node identifiers for devicetree nodes which are enabled (i.e. have
``status = "okay";`` properties) and have matching compatibles. This can be
tested with :c:func:`DT_HAS_NODE()`.

.. doxygengroup:: devicetree-generic-id
   :project: Zephyr

Property access
===============

.. doxygengroup:: devicetree-generic-prop
   :project: Zephyr

Chosen nodes
============

.. doxygengroup:: devicetree-generic-chosen
   :project: Zephyr

Existence checks
================

.. doxygengroup:: devicetree-generic-exist
   :project: Zephyr

Bus helpers
===========

.. doxygengroup:: devicetree-generic-bus
   :project: Zephyr

.. _devicetree-inst-apis:

Instance-based APIs
*******************

These are recommended for use within device drivers. To use them, define
``DT_DRV_COMPAT`` to the lowercase-and-underscores compatible the device driver
implements support for. Note that there are also helpers available for
specific hardware; these are documented in the following sections.

It is an error to use these macros without ``DT_DRV_COMPAT`` defined.

.. doxygengroup:: devicetree-inst
   :project: Zephyr

.. _devicetree-hw-api:

Hardware specific APIs
**********************

The following APIs can also be used by including ``<devicetree.h>``;
no additional include is needed.

ADC
===

.. doxygengroup:: devicetree-adc
   :project: Zephyr

Clocks
======

.. doxygengroup:: devicetree-clocks
   :project: Zephyr

GPIO
====

.. doxygengroup:: devicetree-gpio
   :project: Zephyr

SPI
===

.. doxygengroup:: devicetree-spi
   :project: Zephyr
