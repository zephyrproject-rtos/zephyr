.. _edac_ibecc:

In Band Error Correction Code (IBECC)
#####################################

Overview
********

The mechanism initially found in Intel Elkhart Lake SOCs and later boards is
an integrated memory controller with IBECC.

The In-Band Error Correction Code (IBECC) improves reliability by providing
error detection and correction. IBECC can work for all or for specific regions
of physical memory space. The IBECC is useful for memory technologies that do
not support the out-of-band ECC.

IBECC adds memory overhead of 1/32 of the memory. This memory is not accessible
and used to store ECC syndrome data. IBECC converts read / write transactions
to two separate transactions: one for actual data and another for cache line
containing ECC value.

There is a debug feature IBECC Error Injection which helps to debug and verify
IBECC functionality. ECC errors are injected on the write path and cause ECC
errors on the read path.

IBECC Configuration
*******************

There are three IBECC operation modes which can be selected by Bootloader. They
are listed below:

* OPERATION_MODE = 0x0 sets functional mode to protect requests based on
  address range

* OPERATION_MODE = 0x1 sets functional mode to all requests not be protected and
  to ignore range checks

* OPERATION_MODE = 0x2 sets functional mode to protect all requests and ignore
  range checks

IBECC operational mode is configured through BIOS or Bootloader. For operation
mode 0 there are more BIOS configuration options such as memory regions.

Due to high security risk Error Injection capability should not be enabled for
production. Error Injection is only enabled for tests.

IBECC Logging
*************

IBECC logs the following fields:

* Error Address

* Error Syndrome

* Error Type

  * Correctable Error (CE) - error is detected and corrected by IBECC module.

  * Uncorrectable Error (UE) - error is detected by IBECC module and not
    automatically corrected.

The IBECC driver provides error type for the higher-level application to
implement desired policy with respect for handling those memory errors. Error
syndrome is not used in the IBECC driver but provided to higher-level
application.

Usage notes
***********

Exceptional care needs to be taken with Non Maskable Interrupt (NMI). NMI will
arrive at any time, even if the local CPU has disabled interrupts. That means
that no locking mechanism can protect code against an NMI happening. Zephyr's
IPC mechanisms universally use local IRQ locking as the base layer for all
higher-level synchronization primitives. So, you cannot share anything that is
"protected" by a lock with an NMI, because the protection does not work. The
only tool you have available for synchronization in the Zephyr API that works
against an NMI is the atomic layer. This also applies to callback function
which is called by NMI handler.

Configuration option
********************

Related configuration option:

* :kconfig:option:`CONFIG_EDAC_IBECC`
