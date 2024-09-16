.. _llext:

Linkable Loadable Extensions (LLEXT)
####################################

The LLEXT subsystem provides a toolbox for extending the functionality of an
application at runtime with linkable loadable code.

Extensions are precompiled executables in ELF format that can be verified,
loaded, and linked with the main Zephyr binary. Extensions can be manipulated
and introspected to some degree, as well as unloaded when no longer needed.

.. toctree::
   :maxdepth: 1

   config
   build
   load
   api

.. note::

   The LLEXT subsystem requires architecture-specific support. It is currently
   available only on ARM, ARM64 and Xtensa cores.
