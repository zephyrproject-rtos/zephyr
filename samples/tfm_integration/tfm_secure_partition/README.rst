.. _tfm_secure_partition:

TF-M Secure Partition Sample
############################

Overview
********

A Secure Partition is an isolated module that resides in TF-M. It exposes a number of functions or
"secure services" to other partitions and/or to the non-secure firmware. TF-M already contains
standard partitions such as crypto, protected_storage, or firmware_update, but it's also possible to
create your own partitions.

This sample creates a dummy secure partition and secure service for TF-M and instructs the TF-M
build system to build it into the secure firmware. The dummy secure service is then called in the
main file (in the non-secure firmware).

This dummy partition has a single secure service, which can index one of 5 dummy secrets inside the
partition, and retrieve a hash of the secret.

The partition is located in the ``dummy_partition`` directory. It contains the partition sources,
build files and build configuration files. The partition is built by the TF-M build system, refer to
:ref:`tfm_build_system` for more details.

For more information on how to add custom secure partitions refer to TF-M's guide:
https://tf-m-user-guide.trustedfirmware.org/integration_guide/services/tfm_secure_partition_addition.html

When adapting this partition for your own purposes, please change all occurrences of
"dummy_partition", "DUMMY_PARTITION", "dp", and "DP" to your own partition name. Also, look through
both the secure and non-secure CMakeLists.txt file and make relevant changes, as well as the yaml
files inside "partition".

Building and Running
********************

This sample can be built with or without CONFIG_TFM_IPC, since it contains code for both.

On Target
=========

Refer to :ref:`tfm_ipc` for detailed instructions.

On QEMU
=======

Refer to :ref:`tfm_ipc` for detailed instructions.

Sample Output
=============

   .. code-block:: console

      *** Booting Zephyr OS build zephyr-v3.0.0-3061-g9bb87f4d46e9  ***
      Digest: be45cb2605bf36bebde684841a28f0fd43c69850a3dce5fedba69928ee3a8991
      Digest: 1452c8f04245d355722fdbfb03c69bcfd380b7dff911a3e425013397251f6a4e
      Digest: d3b4349010abb691b9584b6fd6b41ec54596ef7b98d853fb4f5bfa690f50f222
      Digest: 5afbcfede855ca834ff5b4e8a44a32206a51381f3cf52f5001a3241f017ac41a
      Digest: 983318380c325099da63de2e7ca57c1630693b28b4754e08817533295dbfcfbb
      No valid secret for key, received expected error code
