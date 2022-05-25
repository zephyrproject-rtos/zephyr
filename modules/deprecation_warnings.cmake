# Copyright 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# This file is included to warn the user about any deprecated modules
# they are using. To create a warning, follow the pattern:
#
#     if(CONFIG_FOO)
#       message(WARNING "The foo module is deprecated. <More information>")
#     endif()
#
# This is done in a separate CMake file because the modules.cmake file
# in this same directory is evaluated before Kconfig runs.

# (No modules are currently deprecated.)
