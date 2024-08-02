.. _dt-bindings:

Devicetree bindings
###################

A devicetree on its own is only half the story for describing hardware, as it
is a relatively unstructured format. *Devicetree bindings* provide the other
half.

A devicetree binding declares requirements on the contents of nodes, and
provides semantic information about the contents of valid nodes. Zephyr
devicetree bindings are YAML files in a custom format (Zephyr does not use the
dt-schema tools used by the Linux kernel).

These pages introduce bindings, describe what they do, note where they are
found, and explain their data format.

.. note::

   See the :ref:`devicetree_binding_index` for reference information on
   bindings built in to Zephyr.

.. toctree::
   :maxdepth: 2

   bindings-intro.rst
   bindings-syntax.rst
   bindings-upstream.rst
