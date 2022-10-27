.. _devicetree:

Devicetree
##########

A *devicetree* is a hierarchical data structure primarily used to describe
hardware. Zephyr uses devicetree in two main ways:

- to describe hardware to the :ref:`device_model_api`
- to provide that hardware's initial configuration

This page links to a high level guide on devicetree as well as reference
material.

.. _dt-guide:

Devicetree Guide
****************

The pages in this section are a high-level guide to using devicetree for Zephyr
development.

.. toctree::
   :maxdepth: 2

   intro.rst
   design.rst
   bindings.rst
   api-usage.rst
   howtos.rst
   troubleshooting.rst
   dt-vs-kconfig.rst

.. _dt-reference:

Devicetree Reference
********************

These pages contain reference material for Zephyr's devicetree APIs and
built-in bindings.

For the platform-independent details, see the `Devicetree specification`_.

.. _Devicetree specification: https://www.devicetree.org/

.. We use ":glob:" with "*" here to add the generated bindings page.

.. toctree::
   :maxdepth: 3
   :glob:

   api/*
