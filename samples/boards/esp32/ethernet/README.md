# ESP32 Ethernet Sample

This sample application demonstrates Ethernet connectivity on an ESP32
device using Zephyr RTOS. It includes configuration files and source code
necessary to set up and test Ethernet functionality.

## Table of Contents

1. [Overview](#overview)
2. [File Structure](#file-structure)
3. [Prerequisites](#prerequisites)
4. [Building and Flashing](#building-and-flashing)
5. [Configuration](#configuration)
6. [Testing](#testing)
7. [License](#license)

## Overview <a name="overview"></a>

This sample showcases Ethernet functionality on the ESP32 platform using
Zephyr RTOS. It includes basic configurations and source code to initialize
Ethernet and perform network-related tasks.

## File Structure <a name="file-structure"></a>

The directory contains the following files:

1. `eth_dts.overlay`: Device Tree overlay file for Ethernet configuration.
2. `main.c`: Sample application code for Ethernet testing.
3. `CMakeLists.txt`: CMake build configuration for the project.
4. `prj.conf`: Project configuration settings.
5. `sample.yaml`: YAML file describing the sample application.

Here's a brief description of each file:

- `eth_dts.overlay`: Configuration file for Ethernet settings.
- `main.c`: Sample code for Ethernet testing.
- `CMakeLists.txt`: Build configuration for CMake.
- `prj.conf`: Project configuration settings.
- `sample.yaml`: YAML file describing the sample application.

## Prerequisites <a name="prerequisites"></a>

Before you can use this sample, ensure you have the following:

- [Install Zephyr RTOS](https://www.zephyrproject.org/docs/getting_started/index.html) 
  on your development machine.
- An ESP32 development board.
- Network connectivity (e.g., Ethernet connection).

## Building and Flashing <a name="building-and-flashing"></a>

To build and flash the sample application to your ESP32 board, follow these
steps:

1. Configure your development environment for ESP32 and Zephyr as per your
   setup.

2. Open a terminal and navigate to the `esp32_ethernet` directory:

   ```bash
   cd path/to/samples/boards/esp32/ethernet


3. To build the project using CMake, open a terminal and navigate to the
   'esp32_ethernet' directory, then run the following command:
   
   ```bash
   west build -b esp32_ethernet_kit

4. To flash the built image to your ESP32 board, run the following command:

   ```bash
   west flash


## Configuration <a name="configuration"></a>

You can customize the sample's configuration by modifying the `prj.conf`
file. Below are key configurations that you can modify to suit your network
configuration needs:

- `CONFIG_ETH_ESP32`: Enables Ethernet support.
- `CONFIG_NETWORKING`: Enables the networking stack.
- `CONFIG_NET_L2_ETHERNET`: Enables Ethernet L2 layer.
- `CONFIG_NET_IPV6`: Enables IPv6 (disabled by default).
- `CONFIG_NET_IPV4`: Enables IPv4.
- `CONFIG_NET_DHCPV4`: Enables DHCP for IPv4 address configuration.
- `CONFIG_NET_LOG`: Enables network logging.
- `CONFIG_NET_SHELL`: Enables the network shell for testing purposes.
- `CONFIG_NET_CONFIG_SETTINGS`: Initializes DHCP settings.
- `CONFIG_DNS_RESOLVER`: Enables DNS resolver support.
- `CONFIG_DNS_SERVER_IP_ADDRESSES`: Sets DNS server IP addresses (e.g.,
  Google DNS is used by default).

To configure your network settings, modify the `prj.conf` file according to
your specific requirements.

## Testing <a name="testing"></a>

To test the Ethernet functionality, follow these steps:

1. **Run the sample application** on your ESP32 board.

2. **Connect the board to an Ethernet network** using an Ethernet cable.

3. **Observe the network-related activities, such as**:
   - DHCP address assignment.
   - DNS resolution.

## License <a name="license"></a>

This sample application is provided under the [Apache License 2.0](https://www.apache.org/licenses/LICENSE-2.0).
 You can find the full text of the license in the individual source code files.

**Note:** This sample application was created by Grant Ramsay in 2022 
and is released under the Apache License 2.0.
