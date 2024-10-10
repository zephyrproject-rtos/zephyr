# Copyright (c) 2024 CISPA Helmholtz Center for Information Security gGmbH
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(${SOC_SERIES})
# need to force the linker to keep the symbol, such that the Ariane testbench can find it
set(CMAKE_EXE_LINKER_FLAGS "-u tohost" CACHE INTERNAL "" FORCE)
