.. _disk_nvme:

NVMe
####

NVMe is a standardized logical device interface on PCIe bus exposing storage devices.

NVMe controllers and disks are supported. Disks can be accessed via the :ref:`Disk Access API <disk_access_api>` they expose
and thus be used through the :ref:`File System API <file_system_api>`.

Driver design
*************

The driver is sliced up in 3 main parts:
- NVMe controller :zephyr_file:`drivers/disk/nvme/nvme_controller.c`
- NVMe commands :zephyr_file:`drivers/disk/nvme/nvme_cmd.c`
- NVMe namespace :zephyr_file:`drivers/disk/nvme/nvme_namespace.c`

Where the NVMe controller is the root of the device driver. This is the one that will get device driver instances.
Note that this is only what DTS describes: the NVMe controller, and none of its namespaces (disks).
The NVMe command is the generic logic used to communicate with the controller and the namespaces it exposes.
Finally the NVMe namespace is the dedicated part to deal with an actual namespace which, in turn, enables applications
accessing each ones through the Disk Access API :zephyr_file:`drivers/disk/nvme/nvme_disk.c`.

If a controller exposes more than 1 namespace (disk), it will be possible to raise the amount of built-in namespace support
by tweaking the configuration option CONFIG_NVME_MAX_NAMESPACES (see below).

Each exposed disk, via it's related disk_info structure, will be distinguished by its name which is inherited from
it's related namespace. As such, the disk name follows NVMe naming which is nvme<k>n<n> where k is the controller number
and n the namespame number. Most of the time, if only one NVMe disk is plugged into the system, one will see 'nvme0n0' as
an exposed disk.

NVMe configuration
******************

DTS
===

Any board exposing an NVMe disk should provide a DTS overlay to enable its use whitin Zephyr

.. code-block:: devicetree

    #include <zephyr/dt-bindings/pcie/pcie.h>
    / {
        pcie0 {
            #address-cells = <1>;
            #size-cells = <1>;
            compatible = "intel,pcie";
            ranges;

            nvme0: nvme0 {
                compatible = "nvme-controller";
                vendor-id = <VENDOR_ID>;
                device-id = <DEVICE_ID>;
                status = "okay";
            };
        };
    };

Where VENDOR_ID and DEVICE_ID are the ones from the exposed NVMe controller.

Options
=======

* :kconfig:option:`CONFIG_NVME`

Note that NVME requires the target to support PCIe multi-vector MSI-X in order to function.

* :kconfig:option:`CONFIG_NVME_MAX_NAMESPACES`
