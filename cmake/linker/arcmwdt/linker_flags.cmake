# SPDX-License-Identifier: Apache-2.0

# Extra warnings options for twister run
set_property(TARGET linker PROPERTY warnings_as_errors -Wl,--fatal-warnings)
