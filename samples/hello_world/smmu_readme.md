# Zephyr Run SMMU demo

## Pre

In our public RFC, there might be some different details between RFC and
implementation as this is in a very early stage. See
[#Issues60289](github.com/zephyrproject-rtos/zephyr/issues/60289)

Here are the simple instructions for building the Zephyr, assuming the users
are familiar with the Zephyr project.

- Following the official instructions [Getting Started](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) to start
- Download the FVP: [Armv-A Base RevC AEM FVP](https://developer.arm.com/-/media/Files/downloads/ecosystem-models/FVP_Base_RevC-2xAEMvA_11.22_14_Linux64.tgz?rev=838d36a4f4884b9f8d18eb215e3fc13c&hash=598281951FEF577A12677DF1313A09A294A95D8B)
- Following the instructions to start with [FVP base RevC board](https://docs.zephyrproject.org/latest/boards/arm64/fvp_base_revc_2xaemv8a/doc/index.html)

## Development plan

- Phase 1 adds basic SMMU driver, a borrowed page table walker, ahci example for
  Cortex A profile devices
- Phase 2 enhances page table walker, completes the page table management.

## Build

- Build the Zephyr with the following command:
  ```sh
  west build -b fvp_base_revc_2xaemv8a samples/hello_world
  ```

## Run

- Run the Zephyr with the following command:
  ```sh
  west build -t run
  ```

## Example

- Here is an example of using the new APIs for device isolation:
  ```c
  #include <zephyr/device.h>
  #include <zephyr/drivers/dma.h>
  #include <zephyr/logging/log.h>

  LOG_MODULE_REGISTER(smmu_demo, CONFIG_DMA_LOG_LEVEL);

  void main(void)
  {
      const struct device *dma_dev;
      uint32_t sid = 0;
      uintptr_t base = 0x80000000;
      size_t size = 0x1000;

      dma_dev = device_get_binding("DMA_0");
      if (!dma_dev) {
          LOG_ERR("Failed to get DMA device");
          return;
      }

      if (deviso_ctx_alloc(dma_dev, sid) != 0) {
          LOG_ERR("Failed to allocate context");
          return;
      }

      if (deviso_map(dma_dev, sid, base, size) != 0) {
          LOG_ERR("Failed to map DMA buffer");
          deviso_ctx_free(dma_dev, sid);
          return;
      }

      // Perform DMA operations...

      if (deviso_unmap(dma_dev, sid, base, size) != 0) {
          LOG_ERR("Failed to unmap DMA buffer");
      }

      if (deviso_ctx_free(dma_dev, sid) != 0) {
          LOG_ERR("Failed to free context");
      }
  }
  ```

## Output

- The expected output should be similar to the following:
  ```sh
  *** Booting Zephyr OS build zephyr-v2.7.0-1234-gabcd1234 ***
  Hello World! arm64
  ```

## Debug

- To debug the SMMU demo, use the following command:
  ```sh
  west debug
  ```

## Conclusion

- This readme provides instructions for building, running, and debugging the
  SMMU demo in Zephyr. It also includes an example of using the new APIs for
  device isolation.

## Integrating the Subsystem with Existing Zephyr DMA Drivers

The proposed subsystem will integrate with existing Zephyr DMA drivers in the following ways:

* **Subsystem API Integration**: The proposed subsystem will provide a set of APIs that DMA drivers can use to register devices, allocate contexts, and map/unmap DMA buffers. These APIs will be independent of the specific isolation technologies and can be easily extended to support multiple technologies such as SMMU, IOMMU, etc. This will allow existing DMA drivers to leverage the subsystem without significant changes.
  
* **Device Tree Source (DTS) Integration**: The subsystem will require essential information to be described in the DTS, such as hardware device isolation info, DMA devices, and the relationship between Dev Isolation and DMA devices. The DTS interface will follow the standard of the DT bindings for PCI and non-PCI devices. This will ensure that the subsystem can be integrated with existing DMA drivers that use the DTS for configuration.

* **Memory Blocks Allocator Integration**: The subsystem's APIs can be integrated into the Memory Blocks Allocator, which is used by Zephyr DMA device drivers to allocate DMA buffers. This integration will allow DMA drivers to leverage the hardware-level isolation technologies without any changes. The subsystem will provide flexibility for different use cases by keeping the APIs independently available.

* **Use Case Support**: The subsystem will support various use cases among threads, DMA devices, and device isolation technologies. This includes single-thread and multi-thread scenarios, as well as platforms with multiple SMMUs. The subsystem's design will ensure that existing DMA drivers can be adapted to use the hardware-level isolation technologies for enhanced security.

* **DMA Driver Modifications**: Some existing DMA drivers may require slight modifications to integrate with the proposed subsystem. These modifications will mainly involve using the new APIs provided by the subsystem to register devices, allocate contexts, and map/unmap DMA buffers.

By providing a flexible and extensible framework, the proposed subsystem will enable existing Zephyr DMA drivers to leverage hardware-level isolation technologies for improved security and protection against buggy or malicious DMA devices.

## Handling Multiple DMA Devices on a Single Thread

The subsystem will handle multiple DMA devices on a single thread by providing a flexible and extensible framework that allows DMA drivers to register devices, allocate contexts, and map/unmap DMA buffers. Here are the key points:

* **Subsystem API Integration**: The subsystem provides a set of APIs that DMA drivers can use to register devices, allocate contexts, and map/unmap DMA buffers. These APIs are independent of the specific isolation technologies and can be easily extended to support multiple technologies such as SMMU, IOMMU, etc. This allows multiple DMA devices to be managed within a single thread.

* **Device Tree Source (DTS) Integration**: The subsystem requires essential information to be described in the DTS, such as hardware device isolation info, DMA devices, and the relationship between Dev Isolation and DMA devices. The DTS interface follows the standard of the DT bindings for PCI and non-PCI devices. This ensures that multiple DMA devices can be configured and managed within a single thread.

* **Memory Blocks Allocator Integration**: The subsystem's APIs can be integrated into the Memory Blocks Allocator, which is used by Zephyr DMA device drivers to allocate DMA buffers. This integration allows multiple DMA devices to leverage the hardware-level isolation technologies without any changes to the DMA drivers.

* **Use Case Support**: The subsystem supports various use cases among threads, DMA devices, and device isolation technologies. This includes single-thread and multi-thread scenarios, as well as platforms with multiple SMMUs. The subsystem's design ensures that multiple DMA devices can be managed within a single thread, providing flexibility and security.

* **DMA Driver Modifications**: Some existing DMA drivers may require slight modifications to integrate with the proposed subsystem. These modifications will mainly involve using the new APIs provided by the subsystem to register devices, allocate contexts, and map/unmap DMA buffers. This ensures that multiple DMA devices can be managed within a single thread.

By providing a flexible and extensible framework, the subsystem will enable multiple DMA devices to be managed within a single thread, leveraging hardware-level isolation technologies for improved security and protection against buggy or malicious DMA devices.

## Security Implications of the Proposed Subsystem

The proposed subsystem has several security implications that need to be considered:

* **Memory access control**: The subsystem aims to restrict DMA devices' memory access within the expected memory boundaries. This helps prevent buggy or malicious DMA devices from accessing unauthorized memory regions, thereby enhancing system security.

* **Integration with existing DMA drivers**: The subsystem will integrate with existing Zephyr DMA drivers by providing APIs for device registration, context allocation, and buffer mapping/unmapping. This integration ensures that DMA drivers can leverage hardware-level isolation technologies without significant changes, improving overall security.

* **Device Tree Source (DTS) integration**: The subsystem requires essential information to be described in the DTS, such as hardware device isolation info, DMA devices, and the relationship between Dev Isolation and DMA devices. This ensures that the subsystem can be integrated with existing DMA drivers that use the DTS for configuration, providing a secure and consistent approach.

* **Memory Blocks Allocator integration**: The subsystem's APIs can be integrated into the Memory Blocks Allocator, allowing DMA drivers to leverage hardware-level isolation technologies without any changes. This integration provides flexibility for different use cases and enhances security by preventing unauthorized memory access.

* **Use case support**: The subsystem supports various use cases among threads, DMA devices, and device isolation technologies. This includes single-thread and multi-thread scenarios, as well as platforms with multiple SMMUs. The subsystem's design ensures that multiple DMA devices can be managed within a single thread, providing flexibility and security.

* **DMA driver modifications**: Some existing DMA drivers may require slight modifications to integrate with the proposed subsystem. These modifications will mainly involve using the new APIs provided by the subsystem to register devices, allocate contexts, and map/unmap DMA buffers. This ensures that multiple DMA devices can be managed within a single thread, leveraging hardware-level isolation technologies for improved security.

* **Potential vulnerabilities**: The subsystem introduces new APIs and integration points, which may introduce potential vulnerabilities if not properly implemented and tested. It is crucial to ensure that the subsystem is thoroughly tested and reviewed to identify and mitigate any security risks.

* **Performance trade-offs**: Implementing hardware-level isolation technologies may introduce performance overhead due to the additional context allocation, buffer mapping/unmapping, and memory access control. It is essential to balance security and performance to ensure that the subsystem provides adequate protection without significantly impacting system performance.

* **Compliance with security standards**: The subsystem should comply with relevant security standards and best practices to ensure that it provides robust protection against potential threats. This includes following secure coding practices, conducting regular security assessments, and addressing any identified vulnerabilities promptly.

By addressing these security implications, the proposed subsystem can enhance the overall security of the Zephyr system by preventing unauthorized memory access and protecting against buggy or malicious DMA devices.

## Handling DMA Devices with Different Isolation Technologies

The subsystem will handle DMA devices with different isolation technologies by providing a flexible and extensible framework that allows for the integration of various hardware-level isolation technologies. Here are the key points:

* **Subsystem API Integration**: The subsystem provides a set of APIs that are independent of the specific isolation technologies. These APIs can be easily extended to support multiple technologies such as SMMU, IOMMU, etc. This allows DMA drivers to leverage different isolation technologies without significant changes. The APIs are defined in `samples/hello_world/smmu_readme.md`.

* **Device Tree Source (DTS) Integration**: The subsystem requires essential information to be described in the DTS, such as hardware device isolation info, DMA devices, and the relationship between Dev Isolation and DMA devices. The DTS interface follows the standard of the DT bindings for PCI and non-PCI devices. This ensures that the subsystem can be integrated with various isolation technologies. Examples of DTS bindings can be found in `dts/bindings/dma/adi,max32-dma.yaml` and `dts/bindings/dma/arm,dma-pl330.yaml`.

* **Memory Blocks Allocator Integration**: The subsystem's APIs can be integrated into the Memory Blocks Allocator, allowing DMA drivers to leverage hardware-level isolation technologies without any changes. This integration provides flexibility for different use cases and enhances security by preventing unauthorized memory access.

* **Use Case Support**: The subsystem supports various use cases among threads, DMA devices, and device isolation technologies. This includes single-thread and multi-thread scenarios, as well as platforms with multiple SMMUs. The subsystem's design ensures that multiple DMA devices can be managed within a single thread, providing flexibility and security.

* **DMA Driver Modifications**: Some existing DMA drivers may require slight modifications to integrate with the proposed subsystem. These modifications will mainly involve using the new APIs provided by the subsystem to register devices, allocate contexts, and map/unmap DMA buffers. This ensures that multiple DMA devices can be managed within a single thread, leveraging hardware-level isolation technologies for improved security.

By providing a flexible and extensible framework, the subsystem will enable DMA devices with different isolation technologies to be managed effectively, leveraging hardware-level isolation technologies for improved security and protection against buggy or malicious DMA devices.
