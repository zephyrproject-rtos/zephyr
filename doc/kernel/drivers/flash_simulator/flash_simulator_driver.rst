Flash Simulator Driver Documentation
=====================================

Overview
--------

The Flash Simulator Driver is a Zephyr-based implementation that simulates a flash memory device. It provides functionality to read, write, and erase flash memory and includes statistics tracking for various operations. This driver is designed for testing and simulation purposes, particularly in environments where actual flash hardware is not available.

Features
--------

- Simulates a flash memory device with configurable parameters.
- Supports read, write, and erase operations.
- Tracks statistics for reads, writes, erases, and other metrics.
- Configurable to operate in RAM or as a file-based simulator.

Configuration
-------------

Device Tree Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~

The driver uses Device Tree (DT) configurations to derive its settings:

- `erase_block_size`: The size of the erase unit.
- `write_block_size`: The size of the program unit.
- `erase_value`: The value used to indicate erased memory.

Compilation Flags
~~~~~~~~~~~~~~~~~

- `CONFIG_ARCH_POSIX`: Enables POSIX architecture support for native simulation.
- `CONFIG_FLASH_SIMULATOR_STATS`: Enables statistics tracking.
- `CONFIG_FLASH_PAGE_LAYOUT`: Configures page layout support.

Default Configuration
~~~~~~~~~~~~~~~~~~~~~

The driver uses the following default values when not specified:

.. code-block:: c

    #define DEFAULT_FLASH_FILE_PATH "flash.bin"

API Reference
-------------

Initialization
~~~~~~~~~~~~~~~

.. code-block:: c

   static int flash_init(const struct device *dev)

Initializes the flash simulator. Registers the statistics and performs mock initialization.

**Parameters:**

- `dev`: Pointer to the device structure.

**Returns:**

- `0` on success, negative error code on failure.

Read Operation
~~~~~~~~~~~~~~

.. code-block:: c

   static int flash_sim_read(const struct device *dev, const off_t offset, void *data, const size_t len)

Reads data from the simulated flash memory.

**Parameters:**

- `dev`: Pointer to the device structure.
- `offset`: Starting address for the read operation.
- `data`: Pointer to the buffer where read data will be stored.
- `len`: Number of bytes to read.

**Returns:**

- `0` on success, negative error code on failure.

Write Operation
~~~~~~~~~~~~~~~

.. code-block:: c

   static int flash_sim_write(const struct device *dev, const off_t offset, const void *data, const size_t len)

Writes data to the simulated flash memory.

**Parameters:**

- `dev`: Pointer to the device structure.
- `offset`: Starting address for the write operation.
- `data`: Pointer to the data buffer to write.
- `len`: Number of bytes to write.

**Returns:**

- `0` on success, negative error code on failure.

Erase Operation
~~~~~~~~~~~~~~~

.. code-block:: c

   static int flash_sim_erase(const struct device *dev, const off_t offset, const size_t len)

Erases a section of the simulated flash memory.

**Parameters:**

- `dev`: Pointer to the device structure.
- `offset`: Starting address for the erase operation.
- `len`: Number of bytes to erase.

**Returns:**

- `0` on success, negative error code on failure.

Get Parameters
~~~~~~~~~~~~~~~

.. code-block:: c

   static const struct flash_parameters *flash_sim_get_parameters(const struct device *dev)

Retrieves the parameters of the flash simulator.

**Parameters:**

- `dev`: Pointer to the device structure.

**Returns:**

- Pointer to the flash parameters structure.

Statistics
~~~~~~~~~~~

The driver tracks various statistics, including:

- Total bytes read and written.
- Number of read and write calls.
- Erase cycles for units.
- Time spent on read, write, and erase operations.

Usage Example
-------------

Here is an example of how to use the flash simulator driver in an application:

.. code-block:: c

    #include <zephyr/device.h>
    #include <zephyr/drivers/flash.h>

    void main(void) {
        const struct device *flash_dev = device_get_binding("FLASH_SIMULATOR");
        uint8_t data[128];
        int ret;

        // Erase the first 128 bytes
        ret = flash_erase(flash_dev, 0, 128);
        if (ret < 0) {
            printk("Erase failed: %d\n", ret);
            return;
        }

        // Write data to flash
        memset(data, 0xAA, sizeof(data));
        ret = flash_write(flash_dev, 0, data, sizeof(data));
        if (ret < 0) {
            printk("Write failed: %d\n", ret);
            return;
        }

        // Read back the data
        uint8_t read_data[128];
        ret = flash_read(flash_dev, 0, read_data, sizeof(read_data));
        if (ret < 0) {
            printk("Read failed: %d\n", ret);
            return;
        }

        // Validate the read data
        if (memcmp(data, read_data, sizeof(data)) == 0) {
            printk("Data read successfully matches written data!\n");
        } else {
            printk("Data mismatch!\n");
        }
    }

Conclusion
----------

The Flash Simulator Driver is a powerful tool for simulating flash memory operations in a controlled environment. Its flexibility and detailed statistics make it suitable for testing applications that interact with flash storage without requiring physical flash hardware.

License
-------

This driver is licensed under the Apache 2.0 License. Please refer to the license file for more information.