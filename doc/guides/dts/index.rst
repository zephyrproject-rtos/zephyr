.. _devicetree:

Devicetree
##########

A *devicetree* is a hierarchical data structure which describes hardware\
[#dt_spelling]_. The `Devicetree specification`_ fully defines its source and
binary representations. Zephyr uses devicetree to describe the hardware
available on its :ref:`boards`, as well as that hardware's initial
configuration.

.. _Devicetree specification: https://www.devicetree.org/

.. toctree::
   :maxdepth: 2

   intro.rst
   design.rst
   bindings.rst
   macros.rst
   howtos.rst
   dt-vs-kconfig.rst

.. rubric:: Footnotes

.. [#dt_spelling] Note that "devicetree" (without spaces) is preferred to
                  "device tree".
