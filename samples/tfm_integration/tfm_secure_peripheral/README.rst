.. zephyr:code-sample-category:: tfm_secure_peripheral
   :name: TF-M Secure Peripheral
   :show-listing:

   Own a hardware peripheral inside the TF-M Secure Processing Environment (SPE)
   and drive it from the non-secure application.

Overview
********

TF-M runs in the Secure Processing Environment (SPE), the Zephyr application
runs in the Non-secure Processing Environment (NSPE) and reaches it through the
PSA APIs. On top of the built-in services (Crypto, ITS, PS, Attestation,
Firmware Update), TF-M lets you add your own secure partitions.

These samples extend the :zephyr:code-sample:`tfm_secure_partition` sample to a
partition that owns a hardware peripheral inside the SPE and exposes it to the
NSPE through partition-specific secure services. Assigning a peripheral to the
Secure state is done by a vendor-specific bus firewall, so each sub-directory
holds a platform-specific implementation.
