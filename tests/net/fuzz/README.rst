.. SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
.. SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
.. SPDX-License-Identifier: Apache-2.0

Networking fuzz harnesses
##########################

libFuzzer harnesses for the networking stack's parsers of
attacker-controlled input, one subdirectory per library under test, split
again where a library has more than one side to fuzz. Unlike an ordinary
ztest suite, a harness here has no pass/fail assertions of its own:
Twister only builds it (``build_only: true``), and the campaign itself is
run by hand against the harness's own seed corpus, as described in each
harness's own README.

:file:`common/gen_corpus.py` is shared by every harness in this tree: it
turns a harness's ``seeds.txt`` (hex, since a DHCP/TCP/... message is
mostly NUL bytes and the tree takes no binary files) into the directory of
binary seed files libFuzzer wants.
