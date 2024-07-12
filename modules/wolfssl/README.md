Zephyr Project Port
===================

## Overview

This port is for the Zephyr RTOS Project, available [here](https://www.zephyrproject.org/).

It provides the following zephyr code.

- modules/crypto/wolfssl
    - wolfssl library code
- modules/crypto/wolfssl/zephyr/
    - Configuration and CMake files for wolfSSL as a Zephyr module
- modules/crypto/wolfssl/zephyr/samples/wolfssl_test
    - wolfCrypt test application
- modules/crypto/wolfssl/zephyr/samples/wolfssl_bench
    - wolfCrypt benchmark application
- modules/crypto/wolfssl/zephyr/samples/wolfssl_tls_sock
    - socket based sample of TLS
- modules/crypto/wolfssl/zephyr/samples/wolfssl_tls_thread
    - socket based sample of TLS using threads

## How to setup as a Zephyr Module

### Modify your project's west manifest

Add wolfssl as a project to your west.yml:
```
manifest:
  remotes:
    # <your other remotes>
    - name: wolfssl
      url-base: https://github.com/wolfssl

  projects:
    # <your other projects>
    - name: wolfssl
      path: modules/crypto/wolfssl
      revision: master
      remote: wolfssl
```

If you are using the Nordic nRF Connect SDK with Zephyr, the sdk-nrf manifest
file is located at: `vX.X.X/nrf/west.yml`. On OSX the default installation
location for the nRF Connect SDK is at `/opt/nordic/ncs/vX.X.X`.

Update west's modules:

```bash
west update
```

Now west recognizes 'wolfssl' as a module, and will include it's Kconfig and
CMakeFiles.txt in the build system.

If using the Nordic nRF Connect SDK, to get access to a terminal with west
tool access, open "nRF Connect for Desktop", then "Toolchain Manager",
and finally next to the SDK version you are using click the drop down arrow,
then "Open Terminal".

## Build and Run wolfCrypt Test Application

If you want to run build apps without running `west zephyr-export` then it is
possible by setting the `CMAKE_PREFIX_PATH` variable to the location of the
zephyr sdk and building from the `zephyr` directory. For example:

```
CMAKE_PREFIX_PATH=/path/to/zephyr-sdk-<VERSION> west build -p always -b qemu_x86 ../modules/crypto/wolfssl/zephyr/samples/wolfssl_test/
```

build and execute `wolfssl_test`

```
cd [zephyrproject]
west build -p auto -b qemu_x86 modules/crypto/wolfssl/zephyr/samples/wolfssl_test
west build -t run
```

## Build and Run wolfCrypt Benchmark Application

build and execute `wolfssl_benchmark`

```
cd [zephyrproject]
west build -p auto -b qemu_x86 modules/crypto/wolfssl/zephyr/samples/wolfssl_benchmark
west build -t run
```

### Build and Run wolfSSL example `wolfssl_tls_sock`

```
cd [zephyrproject]
west build -p auto -b qemu_x86 modules/crypto/wolfssl/zephyr/samples/wolfssl_tls_sock
west build -t run
```

### Build and Run wolfSSL example `wolfssl_tls_thread`

```
cd [zephyrproject]
west build -p auto -b qemu_x86 modules/crypto/wolfssl/zephyr/samples/wolfssl_tls_thread
west build -t run
```

