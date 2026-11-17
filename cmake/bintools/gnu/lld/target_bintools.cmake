# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# llvm-lld may set invalid LMA addresses on zero-sized sections carrying LOAD flag.
# This causes objcopy --gap-fill <pattern> to misbehave, so clear --gap-fill flag.
# This makes GNU objcopy behave similar to LLVM objcopy, which doesn't support gap-fill.
set_property(TARGET bintools PROPERTY elfconvert_flag_gapfill "")
