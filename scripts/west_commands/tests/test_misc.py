# Copyright (c) 2023 Ampere Computing
# SPDX-License-Identifier: Apache-2.0

from logging import WARNING

from runners import get_runner_cls

def test_deprecated_name(caplog):
    # The deprecated name should result in the same class,
    # but a warning should be logged.
    assert get_runner_cls('misc') is get_runner_cls('misc-flasher')
    warning = 'runner name "misc-flasher" is deprecated, use "misc" instead'
    assert ('runners', WARNING, warning) in caplog.record_tuples
