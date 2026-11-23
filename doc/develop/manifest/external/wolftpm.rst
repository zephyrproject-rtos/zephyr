.. _external_module_wolftpm:

wolfTPM
#######

Introduction
************

wolfTPM is a lightweight, portable TPM 2.0 library optimized for
embedded systems, RTOS environments, and resource-constrained devices.
It provides a full TPM 2.0 implementation including support for
cryptographic operations, key generation, secure storage, and
attestation.

wolfTPM has been integrated as a Zephyr module with CMake and Kconfig
support, making it simple to include TPM functionality in any
Zephyr-based project. The module supports device tree integration for
I2C communication with TPM devices - you can configure the I2C bus by
setting ``WOLFTPM_ZEPHYR_I2C_BUS`` in ``user_settings.h`` to the node
describing the I2C bus on your device. I2C speed can be configured with
``WOLFTPM_ZEPHYR_I2C_SPEED``.

wolfTPM is dual licensed under GPLv3 and commercial licenses.

GitHub Repository: `wolfTPM Repository`_

Requirements
************

* :ref:`external_module_wolfssl` for cryptographic operations

Usage with Zephyr
*****************

Add wolfTPM as a project to your west.yml:

.. code-block:: yaml

  manifest:
    remotes:
    # <your other remotes>
    - name: wolftpm
      url-base: https://github.com/wolfssl
  projects:
    # <your other projects>
    - name: wolftpm
      path: modules/crypto/wolftpm
      revision: v3.10.0
      remote: wolftpm

.. note::

   The revision shown above is an example. Check the `wolfTPM Repository`_
   releases page for the latest release tag to ensure you are using the desired
   version.

Update west's modules:

.. code-block:: bash

   west update

Now west recognizes ``wolftpm`` as a module, and will include its
Kconfig and CMakeLists.txt in the build system.

Sample Applications
*******************

wolfTPM includes two sample applications for Zephyr:

* **wolftpm_wrap_test** - tests core TPM wrapper functionality
* **wolftpm_wrap_caps** - displays TPM capabilities

Both examples build and run successfully on qemu_x86, providing a solid
foundation for development.

Configuration
*************

The module uses a ``user_settings.h`` configuration file that can be
customized to match project-specific requirements. For I2C communication
with TPM devices, configure:

* ``WOLFTPM_ZEPHYR_I2C_BUS`` - set to the device tree node describing
  your I2C bus
* ``WOLFTPM_ZEPHYR_I2C_SPEED`` - set the I2C line speed

Additional Resources
********************

For more regarding the usage of wolfTPM with Zephyr, please refer to
the `wolfTPM Zephyr Example Usage`_ and the `wolfTPM Zephyr
Announcement`_.

For application code examples in Zephyr, please refer to the
`wolfSSL NXP AppCodeHub`_.

For wolfTPM API documentation, please refer to the `wolfTPM Documentation`_.

Reference
*********

.. target-notes::

.. _wolfTPM Repository:
    https://github.com/wolfSSL/wolfTPM

.. _wolfTPM Zephyr Example Usage:
    https://github.com/wolfSSL/wolfTPM/blob/master/zephyr/README.md

.. _wolfSSL NXP AppCodeHub:
    https://github.com/wolfSSL/nxp-appcodehub

.. _wolfTPM Documentation:
    https://www.wolfssl.com/documentation/manuals/wolftpm/

.. _wolfTPM Zephyr Announcement:
    https://www.wolfssl.com/wolftpm-support-for-zephyr-rtos/
