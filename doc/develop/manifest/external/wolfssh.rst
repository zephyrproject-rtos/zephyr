.. _external_module_wolfssh:

wolfSSH
#######

Introduction
************

wolfSSH is a lightweight, portable SSH library optimized for embedded systems, RTOS environments,
and resource-constrained devices. It provides secure shell functionality including SSH server and
client implementations, SCP, and SFTP support. Its support for multiple build configurations makes
it suitable for a wide range of applications and hardware platforms that are utilizing Zephyr
RTOS.

wolfSSH has support for the Zephyr networking stack so applications can use the wolfSSH API to
establish secure SSH connections with other devices or services over the network.

wolfSSH is dual licensed under GPLv3 and commercial licenses.

GitHub Repository: `wolfSSH Repository`_

Requirements
************

* :ref:`external_module_wolfssl` for cryptographic operations

Usage with Zephyr
*****************

Add wolfSSH as a project to your west.yml:

.. code-block:: yaml

  manifest:
    remotes:
    # <your other remotes>
    - name: wolfssh
      url-base: https://github.com/wolfssl
  projects:
    # <your other projects>
    - name: wolfssh
      path: modules/lib/wolfssh
      revision: master
      remote: wolfssh

Update west's modules:

.. code-block:: bash

   west update

Now west recognizes ``wolfssh`` as a module, and will include its Kconfig and CMakeLists.txt in
the build system.

For more regarding the usage of wolfSSH with Zephyr, please refer to the `wolfSSH Zephyr Example
Usage`_.

For application code examples in Zephyr, please refer to the `wolfSSL NXP AppCodeHub`_.

For wolfSSH API documentation, please refer to the `wolfSSH Documentation`_.

Reference
*********

.. target-notes::

.. _wolfSSH Repository:
    https://github.com/wolfSSL/wolfssh

.. _wolfSSH Zephyr Example Usage:
    https://github.com/wolfSSL/wolfssh/blob/master/zephyr/README.md#build-and-run-samples

.. _wolfSSL NXP AppCodeHub:
    https://github.com/wolfSSL/nxp-appcodehub

.. _wolfSSH Documentation:
    https://www.wolfssl.com/documentation/manuals/wolfssh/
