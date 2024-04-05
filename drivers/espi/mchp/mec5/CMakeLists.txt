# Copyright Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ESPI_FLASH_CHANNEL flash_chan.c)
zephyr_library_sources_ifdef(CONFIG_ESPI_OOB_CHANNEL oob_chan.c)
zephyr_library_sources_ifdef(CONFIG_ESPI_MEC5_ACPI_EC_OS hd_acpi_ec_os.c)
zephyr_library_sources_ifdef(CONFIG_ESPI_MEC5_ACPI_EC_HCMD hd_acpi_ec_hcmd.c)
zephyr_library_sources_ifdef(CONFIG_ESPI_MEC5_KBC hd_kbc.c)
zephyr_library_sources_ifdef(CONFIG_ESPI_MEC5_SHM_EMI hd_shm_emi.c)
zephyr_library_sources_ifdef(CONFIG_ESPI_MEC5_MAILBOX hd_mailbox.c)
zephyr_library_sources_ifdef(CONFIG_ESPI_MEC5_BDP hd_bdp.c)
zephyr_library_sources_ifdef(CONFIG_ESPI_MEC5_HOST_UART hd_uart.c)
zephyr_library_include_directories(
  ${ZEPHYR_BASE}/drivers/espi
)
