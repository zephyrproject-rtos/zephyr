.. _llext:

Linkable Loadable Extensions (LLEXT)
####################################

.. toctree::
   :maxdepth: 1

   build
   load
   api

The llext subsystem provides a toolbox for extending the functionality of an
application at runtime with linkable loadable code.

Extensions can be loaded from precompiled ELF formatted data which is
verified, loaded, and linked with other extensions. Extensions can be
manipulated and introspected to some degree, as well as unloaded when no longer
needed.
