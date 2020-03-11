.. _dt-guide:

Devicetree Guide
################

A *devicetree* is a hierarchical data structure which describes hardware\
[#dt_spelling]_. The `Devicetree specification`_ fully defines its source and
binary representations. Zephyr uses devicetree to describe the hardware
available on its :ref:`boards`, as well as that hardware's initial
configuration.

This page is the index for a guide to devicetree and how to use it in Zephyr.
For an API reference, see :ref:`devicetree_api`.

.. _Devicetree specification: https://www.devicetree.org/

.. toctree::
   :maxdepth: 2

   intro.rst
   design.rst
   bindings.rst
   api-usage.rst
   legacy-macros.rst
   howtos.rst
   dt-vs-kconfig.rst

.. rubric:: Footnotes

.. [#dt_spelling] Note that "devicetree" (without spaces) is preferred to
                  "device tree".
