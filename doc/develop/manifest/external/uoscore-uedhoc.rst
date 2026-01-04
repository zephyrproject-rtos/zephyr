.. _external_module_uoscore_uedhoc:

uOSCORE / uEDHOC
################

Introduction
************

uOSCORE / uEDHOC is a lightweight C implementation of the IETF protocols OSCORE
(:rfc:`8613`) and EDHOC (:rfc:`9528`), specifically designed for microcontrollers.
This implementation is independent of the operating system, cryptographic engine,
and, in the case of uEDHOC, the transport protocol. Notably, uOSCORE / uEDHOC
operates using only stack memory, avoiding heap allocation.

The OSCORE and EDHOC protocols were developed by the IETF to provide a lightweight
alternative to (D)TLS for IoT deployments. Unlike (D)TLS, OSCORE and EDHOC function
at the application layer, which is typically CoAP rather than at the transport
layer. This allows for end-to-end authenticated and encrypted communication through
CoAP proxies—a capability that transport layer security protocols like (D)TLS
cannot achieve.

Usage with Zephyr
*****************

To pull in uoscore-uedhoc as a Zephyr module, either add `zephyr uoscore-uedhoc fork`_
as a West project in the ``west.yaml`` file or pull it in by adding a submanifest (e.g.
``zephyr/submanifests/uoscore-uedhoc.yaml``) file with the following content and
run ``west update``:

.. code-block:: yaml

   manifest:
     projects:
       - name: uoscore-uedhoc
         url: https://github.com/zephyrproject-rtos/uoscore-uedhoc
         revision: zephyr
         path: modules/lib/uoscore-uedhoc # adjust the path as needed

For more detailed information and API documentation, refer to the
`upstream uoscore-uedhoc repository`_.

Reference
*********

.. target-notes::

.. _zephyr uoscore-uedhoc fork:
    https://github.com/zephyrproject-rtos/uoscore-uedhoc

.. _upstream uoscore-uedhoc repository:
    https://github.com/eriptic/uoscore-uedhoc
