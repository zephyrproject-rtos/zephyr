.. _runtime_config_reference:


Runtime Configuration System
############################

Zephyr RTOS provides a submodule called 'settings' used
for persistent storage such as saving and loading runtime
configuration data. A variety of storage implementations
are provided behind a common API using FCB, NVS, or a
file system.  These different implementations give the
application developer flexibility to select an appropriate
storage medium, and even change it later as needs change.

The runtime configuration system is used by various Zephyr
components and can be used simultaneously by user applications.

.. toctree::
   :maxdepth: 1

   settings/settings.rst
