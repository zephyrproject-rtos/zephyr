# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(EXTRA_CONF_FILE
        ../../../samples/bluetooth/hci_uart_async/prj.conf
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(test_samples_bluetooth_hci_uart_async)
target_sources(app PRIVATE
        ../../../samples/bluetooth/hci_uart_async/src/hci_uart_async.c
        src/test_hci_uart_async.c
)
