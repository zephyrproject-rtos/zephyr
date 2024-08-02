# Copyright (c) 2019 Nordic Semiconductor ASA
# Copyright (c) 2019 Linaro Limited
# SPDX-License-Identifier: BSD-3-Clause

"""
Shared internal code. Do not use outside of the package.
"""

from typing import Any, Callable

def _slice_helper(node: Any,    # avoids a circular import with dtlib
                  prop_name: str, size: int, size_hint: str,
                  err_class: Callable[..., Exception]):
    # Splits node.props[prop_name].value into 'size'-sized chunks,
    # returning a list of chunks. Raises err_class(...) if the length
    # of the property is not evenly divisible by 'size'. The argument
    # to err_class is a string which describes the error.
    #
    # 'size_hint' is a string shown on errors that gives a hint on how
    # 'size' was calculated.

    raw = node.props[prop_name].value
    if len(raw) % size:
        raise err_class(
            f"'{prop_name}' property in {node!r} has length {len(raw)}, "
            f"which is not evenly divisible by {size} (= {size_hint}). "
            "Note that #*-cells properties come either from the parent node or "
            "from the controller (in the case of 'interrupts').")

    return [raw[i:i + size] for i in range(0, len(raw), size)]
