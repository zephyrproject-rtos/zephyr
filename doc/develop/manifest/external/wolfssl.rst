.. _external_module_wolfssl:

wolfSSL
#######

Introduction
************

wolfSSL is a lightweight, portable SSL/TLS library optimized for embedded systems, RTOS environments, and resource-constrained devices. It offers a range of cryptographic functions, and secure communication protocols (up to TLS 1.3 and DTLS 1.3) along with post-quantum cryptography support. Its support for multiple build configurations makes it suitable for a wide range of applications and hardware platforms that are utilizing the Zephyr RTOS.

wolfSSL has support for the Zephyr networking stack so applications can use the wolfSSL API to establish secure connections with other devices or services over the network.

wolfSSL is dual licensed under GPLv3 and commercial licenses.

GitHub Repository: `wolfSSL Repository`_

Usage with Zephyr
*****************

Add wolfssl as a project to your west.yml:

.. code-block:: yaml

  manifest:
    remotes:
    # <your other remotes>
    - name: wolfssl
      url-base: https://github.com/wolfssl
  projects:
    # <your other projects>
    - name: wolfssl
      path: modules/crypto/wolfssl
      revision: master
      remote: wolfssl

Update west's modules:

.. code-block:: bash

   west update

Now west recognizes ``wolfssl`` as a module, and will include its Kconfig and
CMakeLists.txt in the build system.

For more regarding the usage of wolfSSL with Zephyr, please refer to the `wolfSSL Zephyr Example Usage`_.

For application code examples in Zephyr, please refer to the `wolfSSL NXP AppCodeHub`_.

For wolfSSL API documentation, please refer to the `wolfSSL Documentation`_.

Reference
*********

.. target-notes::

.. _wolfssl Repository:
    https://github.com/wolfSSL/wolfssl

.. _wolfSSL Zephyr Example Usage:
    https://github.com/wolfSSL/wolfssl/blob/master/zephyr/README.md#build-and-run-wolfcrypt-benchmark-application

.. _wolfSSL NXP AppCodeHub:
    https://github.com/wolfSSL/nxp-appcodehub

.. _wolfSSL Documentation:
    https://www.wolfssl.com/docs/
