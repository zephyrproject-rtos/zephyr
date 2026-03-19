.. _biometrics_api:

Biometrics
##########

Overview
********

The biometrics API provides a unified interface for biometric sensors such as
fingerprint scanners, iris scanners, and face recognition modules. These sensors
are commonly used for secure authentication in embedded systems, access control
devices, and IoT applications.

The API supports the full lifecycle of biometric operations including enrollment,
template management, and matching. Sensors can store templates on-device or on
the host system depending on hardware capabilities.

A typical fingerprint enrollment process requires capturing multiple samples
of the same finger to create a reliable template. The matching process compares
a captured sample against stored templates to verify identity.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_BIOMETRICS`
* :kconfig:option:`CONFIG_BIOMETRICS_SHELL`
* :kconfig:option:`CONFIG_BIOMETRICS_INIT_PRIORITY`

Shell commands
**************

.. zephyr:shell_help::
   :main_command: biometrics
   :main_kconfig: CONFIG_BIOMETRICS_SHELL
   :extra_kconfig_deps: CONFIG_BIOMETRICS
   :platform: native_sim/native/64

API Reference
*************

.. doxygengroup:: biometrics_interface
