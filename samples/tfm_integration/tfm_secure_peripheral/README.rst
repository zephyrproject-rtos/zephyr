.. zephyr:code-sample-category:: tfm_secure_peripheral
   :name: TF-M Secure Peripheral
   :show-listing:

   Add custom secure services and peripherals to the TF-M Secure Processing
   Environment (SPE).

Overview
********

TF-M runs in the Secure Processing Environment (SPE), the Zephyr application
runs in the Non-secure Processing Environment (NSPE) and reaches it through the
PSA APIs. On top of the built-in services (Crypto, ITS, PS, Attestation,
Firmware Update), TF-M lets you add your own secure partitions.

These samples extend :zephyr:code-sample:`tfm_secure_partition` to a partition
that owns a hardware peripheral inside the SPE and exposes it to the NSPE over
PSA. Sub-directories are platform specific samples for this usage of peripheral.
