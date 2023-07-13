# HTTP/2 Server Prototype with Zephyr RTOS

## Overview

This prototype demonstrates the basic structure and setup required for a simple HTTP/2 server using the Zephyr RTOS.

The server provides a public struct, `struct http2_server_config`, which holds the configuration options such as the port and the address family. After configuring the server, `http2_server_init` function is used to initialize the HTTP/2 server. This function sets up the server socket, binds it to the specified address and port, starts listening for incoming connections, initializes necessary data structures, and returns the server socket file descriptor for further operations.

After successful initialization, the `http2_server_start` function is called to start the server. This function is responsible for starting the HTTP/2 server and handling incoming client connections.

## Requirement

### Setting Up QEMU Networking

To enable QEMU networking with the host machine, we will follow the instructions provided in the [Networking with QEMU](https://docs.zephyrproject.org/latest/connectivity/networking/qemu_setup.html#networking-with-qemu) section from the Zephyr documentation

In the net-tools directory:

**Terminal #1:**
```bash
./loop-socat.sh
```

**Terminal #2:**
```bash
sudo ./loop-slip-tap.sh
```

Next, to connect to external networks, we need to add the following rule:

```bash
sudo iptables -t nat -A POSTROUTING -j MASQUERADE -s 192.0.2.1
```

Lastly, we enable IP forwarding with the following command:

```bash
sudo sysctl -w net.ipv4.ip_forward=1
```

## Running the server

To run the application, we should navigate to its directory `samples/net/sockets/http_server` and run the following commands:

```bash
source ~/zephyrproject/.venv/bin/activate
west build -b qemu_x86 .
west build -t run
```

When the server is up, we can make requests to the server using either the HTTP/1.1 or HTTP/2 protocol from the host machine.

**With HTTP/1.1:**

- Using a browser: `http://192.0.2.1:8080/`
- Using curl: `curl -v --compressed http://192.0.2.1:8080/`

**With HTTP/2:**

- Using nghttp client: `nghttp -v --no-dep http://192.0.2.1:8080/`
- Using curl: `curl --http2 -v --compressed http://192.0.2.1:8080/`

## Testing

The features of this HTTP/2 prototype are tested using the Ztest framework. Here's an overview of each test:

- `test_http2_upgrade`: Verifies that the server supports the upgrade from HTTP/1.1 to HTTP/2.
- `test_backward_compatibility`: Tests backward compatibility with HTTP/1.1 clients.
- `test_gen_gz_inc_file`: Tests the integrity of the compressed file by comparing it with the original file.
- `test_http2_support_ipv6`: Tests the server's ability to initialize and listen on an IPv6 address.
- `test_http2_support_ipv4`: Tests the server's ability to initialize and listen on an IPv4 address.
- `test_http2_server_stop`: Tests the server stopping functionality.
- `test_http2_server_init`: Verifies that the server socket is created successfully.
- `test_parse_http2_frames`: Verifies that the frames in the provided buffer are correctly parsed and their details are accurately stored.

To run the test suite, we should navigate to its folder `tests/net/lib/http_server` and then run these commands:

```bash
source ~/zephyrproject/.venv/bin/activate
export ZEPHYR_SDK_INSTALL_DIR=~/.zephyrtools/toolchain/zephyr-sdk-0.15.1/
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
west build -p auto -b native_posix_64 -t run .
~/zephyrproject/zephyr/scripts/twister -W -p native_posix_64 -T
```
