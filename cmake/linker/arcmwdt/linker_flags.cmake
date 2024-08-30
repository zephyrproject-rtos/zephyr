# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0

set_property(TARGET linker PROPERTY cpp_base -Hcplus)

# Extra warnings options for twister run
set_property(TARGET linker PROPERTY warnings_as_errors -Wl,--fatal-warnings)

check_set_linker_property(TARGET linker PROPERTY sort_alignment -Wl,--sort-section=alignment)
