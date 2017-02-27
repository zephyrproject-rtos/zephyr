.. _ip_stack_migrate:

Migrating from Zephyr v1.6 IP Stack to v1.7
###########################################

Zephyr v1.6 and earlier is using network IP stack that has its origin in uIP.
The uIP stack was modified heavily to use it in Zephyr. The uIP stack had
limitations described below that required new and native IP stack to be
developed.

This document is a high level description of the changes between these two
major Zephyr releases. For individual changes, the application developer can
find more details in the network header file documentation in include/net/.

Because of this new native IP stack, the following changes are required to
migrate applications using the older v1.6 IP stack to the new v1.7 IP stack:

* **Dual IPv6 and IPv4 stack support.** In Zephyr v1.6, applications could not
  use both IPv6 and IPv4 simultaneously. This is changed in Zephyr v1.7 and the
  IP stack supports both IPv6 and IPv4 at the same time. In practice this means
  that applications should be prepared to support both IPv6 and IPv4 in the
  code.

* **Multiple simultaneous network technologies support.**
  In Zephyr v1.7 it is possible to have multiple network technologies enabled
  at the same time. This means that applications can utilize concurrently e.g.,
  IEEE 802.15.4 and Bluetooth IP networking. The different network technologies
  are abstracted to network interfaces and there can be multiple network
  interfaces in the system depending on configuration.

* **Network Kconfig options are changed.** Most of the networking configuration
  options are renamed. Please check the :ref:`networking` documentation for the
  new names.

* **All uIP based APIs are gone.** Those APIs were not public in v1.6 but
  applications could call them anyway. These uIP APIs were mainly used to set
  IP address etc. management style operations. The new management APIs can be
  found in net_if.h and net_mgmt.h in Zephyr v1.7.

* **Network buffer management is changed.** In earlier Zephyr versions, there
  were big 1280 byte long buffers that applications could utilize. In Zephyr
  v1.7 this is changed so that device memory is utilized more efficiently.
  Now small buffer fragments are allocated to store the data, and these
  fragments can then be chained together to store larger amount of data. For
  applications this means that the memory received from network or sent to
  network is not contiguous and application should use the helper functions
  found in nbuf.h when reading and writing the network data.

* **Network context/socket API is changed.** The new API found in net_context.h
  is more BSD-socket-like than the earlier API. The new context API is not
  fully BSD socket compatible as it needs to support both synchronous and
  asynchronous operations. Porting BSD socket application to use the
  net_context_* API is easier than in Zephyr v1.6.

* **CoAP library API is changed.** The Zephyr v1.6 CoAP API is removed.
  The new Zephyr v1.7 API is called ZoAP and it can be found in zoap.h header
  file.

* **The TinyDTLS library is removed.** The tinydtls crypto library was used
  only by CoAP in Zephyr v1.6 and it is removed in Zephyr v1.7. It is replaced
  by mbedtls library.
