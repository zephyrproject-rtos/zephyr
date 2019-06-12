# SPDX-License-Identifier: Apache-2.0

# The intention with this file is, to have a common placeholder for macros
# which does not fit into any of the categories defined by the existing
# target_xxx.cmake files and which have a fairly high commonality between
# toolchains.
#
# See root CMakeLists.txt for description and expectations of this macro

macro(toolchain_cc_nocommon)

  zephyr_compile_options(-fno-common)

endmacro()
