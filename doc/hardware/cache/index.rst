.. _cache-guide:

Cache Interface
###############

This is a high-level guide to cache interface and Kconfig options related to
cache controllers. See :ref:`cache_api` for API reference material.

Zephyr has different Kconfig options to control how the cache controller is
implemented and controlled.

* :kconfig:option:`CONFIG_CPU_HAS_DCACHE` /
  :kconfig:option:`CONFIG_CPU_HAS_ICACHE`: these hidden options should be
  selected at SoC / platform level when the CPU actually supports a data or
  instruction cache. The cache controller can be in the core or can be an
  external cache controller for which a driver is provided.

  These options have the goal to document an available feature and should be
  set whether we plan to support and use the caches in Zephyr or not.

* :kconfig:option:`CONFIG_DCACHE` / :kconfig:option:`CONFIG_ICACHE`: these
  options must be selected when support for data or instruction cache is
  present and working in zephyr.

  All the code paths related to cache control must be conditionally enabled
  depending on these symbols. When the symbol is set the cache is considered
  enabled and used.

  These symbols say nothing about the actual API interface exposed to the user.
  For example a platform using the data cache can enable the
  :kconfig:option:`CONFIG_DCACHE` symbol and use some HAL exported function in
  some platform-specific code to enable and manage the d-cache.

* :kconfig:option:`CONFIG_CACHE_MANAGEMENT`: this option must be selected when
  the cache operations are exposed to the user through a standard API (see
  :ref:`cache_api`).

  When this option is enabled we assume that all the cache functions are
  implemented in the architectural code or in an external cache controller
  driver.

* :kconfig:option:`CONFIG_ARCH_CACHE`/:kconfig:option:`CONFIG_EXTERNAL_CACHE`:
  mutually exclusive options for :kconfig:option:`CACHE_TYPE` used to define
  whether the cache operations are implemented at arch level or using an
  external cache controller with a provided driver.

  * :kconfig:option:`CONFIG_ARCH_CACHE`: the cache API is implemented by the
    arch code

  * :kconfig:option:`CONFIG_EXTERNAL_CACHE`: the cache API is implemented by a
    driver that supports the external cache controller. In this case the driver
    must be located as usual in the :file:`drivers/cache/` directory

.. _cache_api:

Cache API
*********

.. doxygengroup:: cache_interface
