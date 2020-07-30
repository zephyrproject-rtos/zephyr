# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources(ieee802154_native_posix.c
                       native_posix_802154_filter.c
                       native_posix_802154_frame_parser.c
                       native_posix_802154_pib.c
                       ack_generator/native_posix_802154_ack_data.c
                       ack_generator/native_posix_802154_ack_generator.c
                       ack_generator/native_posix_802154_enh_ack_generator.c
                       ack_generator/native_posix_802154_imm_ack_generator.c)
