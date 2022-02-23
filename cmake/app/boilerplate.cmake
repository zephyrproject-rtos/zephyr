# SPDX-License-Identifier: Apache-2.0

######################################
# The use of this file is deprecated #
######################################

# To build a Zephyr application it must start with one of those lines:
#
# find_package(Zephyr)
# find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
#
# The `REQUIRED HINTS $ENV{ZEPHYR_BASE}` variant is required for any application
# inside the Zephyr repository.
#
# Loading of this file directly is deprecated and only kept for backward compatibility.
message(WARNING "Loading of Zephyr boilerplate.cmake directly is deprecated, "
        "please use 'find_package(Zephyr)'"
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
