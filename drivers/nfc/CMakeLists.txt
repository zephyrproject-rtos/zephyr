# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_NFC_PN53X pn53x)
add_subdirectory_ifdef(CONFIG_NFC_MFRC522 mfrc522)
add_subdirectory_ifdef(CONFIG_NFC_EMUL nfc_emul)
