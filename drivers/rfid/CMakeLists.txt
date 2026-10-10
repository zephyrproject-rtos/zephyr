# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_property(ALLOW_EMPTY TRUE)

# zephyr-keep-sorted-start
add_subdirectory_ifdef(CONFIG_MFRC522 mfrc522)
# zephyr-keep-sorted-stop
