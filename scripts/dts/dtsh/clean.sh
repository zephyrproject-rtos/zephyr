# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0
#!/usr/bin/env sh

find -depth -type d -name "*_cache" -exec rm -rv {} \;
find -depth -type d -name "*.egg-info" -exec rm -rv {} \;

find src/ -depth -type d -name "__pycache__" -exec rm -rv {} \;
find tests/ -depth -type d -name "__pycache__" -exec rm -rv {} \;
