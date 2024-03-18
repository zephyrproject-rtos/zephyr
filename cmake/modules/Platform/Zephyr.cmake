# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024, Arduino SA

# Perform the same initialization as the Generic platform, then enable
# dynamic library support if CONFIG_LLEXT is enabled.

include(Platform/Generic)

# Enable dynamic library support when CONFIG_LLEXT is enabled.
if(CONFIG_LLEXT)
  set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS true)
endif()
