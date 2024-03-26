# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Unit tests for the dtsh.autocomp module."""

# Relax pylint a bit for unit tests.
# pylint: disable=missing-class-docstring
# pylint: disable=missing-function-docstring


from typing import List

import os

from dtsh.model import DTPath
from dtsh.shell import DTSh, DTShCommand, DTShArg, DTShFlagHelp
from dtsh.shellutils import (
    DTShFlagPager,
    DTShFlagReverse,
    DTShArgFixedDepth,
    DTShParamDTPath,
)
from dtsh.rl import DTShReadline
from dtsh.autocomp import (
    DTShAutocomp,
    RlStateDTShCommand,
    RlStateDTShOption,
    RlStateDTPath,
    RlStateDTVendor,
    RlStateDTBus,
    RlStateDTAlias,
    RlStateDTChosen,
    RlStateDTLabel,
)

from .dtsh_uthelpers import DTShTests


class MockDTShArg(DTShArg):
    brief = "mock arg"
    longname = "mockarg"

    def __init__(self) -> None:
        super().__init__(argname="arg")

    RL_STATES: List[DTShReadline.CompleterState] = [
        DTShReadline.CompleterState("a", 0),
        DTShReadline.CompleterState("ab", 1),
        DTShReadline.CompleterState("b", 2),
    ]

    def autocomp(
        self, txt: str, sh: "DTSh"
    ) -> List[DTShReadline.CompleterState]:
        return [
            state
            for state in MockDTShArg.RL_STATES
            if state.rlstr.startswith(txt)
        ]


class MockDTShCmd(DTShCommand):
    def __init__(self) -> None:
        super().__init__(
            "mockcmd",
            "mock command",
            [
                DTShFlagReverse(),
                DTShFlagPager(),
                DTShArgFixedDepth(),
                MockDTShArg(),
            ],
            DTShParamDTPath(),
        )


def test_dtshautocomp_complete_dtshcmd() -> None:
    cmd_mock = MockDTShCmd()
    cmd_ls = DTShCommand("ls", "", None, None)
    cmd_tree = DTShCommand("tree", "", None, None)
    cmd_lstree = DTShCommand("lstree", "", None, None)
    sh = DTSh(
        DTShTests.get_sample_dtmodel(), [cmd_mock, cmd_ls, cmd_tree, cmd_lstree]
    )

    assert sorted(
        [RlStateDTShCommand(cmd.name, cmd) for cmd in sh.commands]
    ) == DTShAutocomp.complete_dtshcmd("", sh)

    assert sorted(
        [
            RlStateDTShCommand(cmd_ls.name, cmd_ls),
            RlStateDTShCommand(cmd_lstree.name, cmd_lstree),
        ]
    ) == DTShAutocomp.complete_dtshcmd("l", sh)

    # Unique matches are not hidden.
    assert [
        RlStateDTShCommand(cmd_tree.name, cmd_tree)
    ] == DTShAutocomp.complete_dtshcmd("t", sh)


def test_dtshautocomp_complete_dtshopt() -> None:
    cmd = MockDTShCmd()
    state_h = RlStateDTShOption("-h", DTShFlagHelp())
    state_help = RlStateDTShOption("--help", DTShFlagHelp())
    state_r = RlStateDTShOption("-r", DTShFlagReverse())
    state_pager = RlStateDTShOption("--pager", DTShFlagPager())
    state_depth = RlStateDTShOption("--fixed-depth", DTShArgFixedDepth())
    state_mock = RlStateDTShOption("--mockarg", MockDTShArg())

    # All options.
    assert [
        state_h,
        state_r,
        state_depth,
        state_mock,
        state_pager,
    ] == DTShAutocomp.complete_dtshopt("-", cmd)

    # Only options with a long name.
    assert [
        state_help,
        state_depth,
        state_mock,
        state_pager,
    ] == DTShAutocomp.complete_dtshopt("--", cmd)

    # Should start with "-".
    assert not DTShAutocomp.complete_dtshopt("", cmd)

    # Unique matches are not hidden.
    assert [state_r] == DTShAutocomp.complete_dtshopt("-r", cmd)
    assert [state_pager] == DTShAutocomp.complete_dtshopt("--pager", cmd)


def test_dtshautocomp_complete_dtpath() -> None:
    sh = DTSh(DTShTests.get_sample_dtmodel(), [])

    # All children of the current working branch.
    assert sorted(
        [RlStateDTPath(node.name, node) for node in sh.dt.root.children]
    ) == DTShAutocomp.complete_dtpath("", sh)

    # Complete absolute path.
    assert sorted(
        [
            RlStateDTPath(DTPath.join("/soc", node.name), node)
            for node in sh.dt["/soc"].children
            if node.name.startswith("egu")
        ]
    ) == DTShAutocomp.complete_dtpath("/soc/egu", sh)

    # Complete relative path.
    sh.cd("/soc")
    assert sorted(
        [
            RlStateDTPath(node.name, node)
            for node in sh.dt["/soc"].children
            if node.name.startswith("egu")
        ]
    ) == DTShAutocomp.complete_dtpath("egu", sh)


def test_dtshautocomp_complete_dtcompat() -> None:
    sh = DTSh(DTShTests.get_sample_dtmodel(), [])

    # All compatible strings.
    assert sorted(sh.dt.compatible_strings) == [
        state.rlstr for state in DTShAutocomp.complete_dtcompat("", sh)
    ]

    # Complete compatible strings:
    # - arduino,uno-adc, arduino-header-r3
    # - arm,armv7m-itm, arm,armv7m-systick, arm,cortex-m4f, arm,cryptocell-310,
    #   arm,v7m-nvic
    assert sorted(
        [
            compat
            for compat in sh.dt.compatible_strings
            if compat.startswith("arm") or compat.startswith("arduino")
        ]
    ) == [state.rlstr for state in DTShAutocomp.complete_dtcompat("a", sh)]

    # Unique matches are not hidden: arm,cortex-m4f
    assert ["arm,cortex-m4f"] == [
        state.rlstr
        for state in DTShAutocomp.complete_dtcompat("arm,cortex", sh)
    ]


def test_dtshautocomp_complete_dtvendor() -> None:
    sh = DTSh(DTShTests.get_sample_dtmodel(), [])

    # All vendors.
    assert sorted(
        [
            RlStateDTVendor(vendor.prefix, vendor.name)
            for vendor in sh.dt.vendors
        ]
    ) == DTShAutocomp.complete_dtvendor("", sh)

    # Complete vendor prefix.
    assert sorted(
        [
            RlStateDTVendor(vendor.prefix, vendor.name)
            for vendor in sh.dt.vendors
            if vendor.prefix.startswith("a")
        ]
    ) == DTShAutocomp.complete_dtvendor("a", sh)

    # Unique matches are not hidden.
    vendor_arduino = sh.dt.get_vendor("arduino,")
    assert vendor_arduino
    assert [
        RlStateDTVendor(vendor_arduino.prefix, vendor_arduino.name)
    ] == DTShAutocomp.complete_dtvendor("ard", sh)


def test_dtshautocomp_complete_dtbus() -> None:
    sh = DTSh(DTShTests.get_sample_dtmodel(), [])

    # All bus protocols.
    assert sorted(
        [RlStateDTBus(bus) for bus in sh.dt.bus_protocols]
    ) == DTShAutocomp.complete_dtbus("", sh)

    # Complete bus protocol: i2c, i2s.
    assert sorted(
        [
            RlStateDTBus(bus)
            for bus in sh.dt.bus_protocols
            if bus.startswith("i2")
        ]
    ) == DTShAutocomp.complete_dtbus("i2", sh)

    # Unique matches are not hidden: spi.
    assert [
        RlStateDTBus(bus) for bus in sh.dt.bus_protocols if bus == "spi"
    ] == DTShAutocomp.complete_dtbus("s", sh)


def test_dtshautocomp_complete_dtalias() -> None:
    sh = DTSh(DTShTests.get_sample_dtmodel(), [])

    # All aliased nodes.
    assert sorted(
        [
            RlStateDTAlias(alias, node)
            for alias, node in sh.dt.aliased_nodes.items()
        ]
    ) == DTShAutocomp.complete_dtalias("", sh)

    # Complete alias name: led0, led1, led2, led3.
    assert sorted(
        [
            RlStateDTAlias(alias, node)
            for alias, node in sh.dt.aliased_nodes.items()
            if alias.startswith("led")
        ]
    ) == DTShAutocomp.complete_dtalias("l", sh)

    # Unique matches are not hidden: watchdog0
    assert [
        RlStateDTAlias(alias, node)
        for alias, node in sh.dt.aliased_nodes.items()
        if alias == "watchdog0"
    ] == DTShAutocomp.complete_dtalias("w", sh)


def test_dtshautocomp_complete_dtchosen() -> None:
    sh = DTSh(DTShTests.get_sample_dtmodel(), [])

    # All chosen nodes.
    assert sorted(
        [
            RlStateDTChosen(chosen, node)
            for chosen, node in sh.dt.chosen_nodes.items()
        ]
    ) == DTShAutocomp.complete_dtchosen("", sh)

    # Complete chosen parameter name: zephyr,flash, zephyr,flash-controller.
    assert sorted(
        [
            RlStateDTChosen(chosen, node)
            for chosen, node in sh.dt.chosen_nodes.items()
            if chosen.startswith("zephyr,flash")
        ]
    ) == DTShAutocomp.complete_dtchosen("zephyr,f", sh)

    # Unique matches are not hidden: zephyr,uart-mcumgr
    assert [
        RlStateDTChosen(chosen, node)
        for chosen, node in sh.dt.chosen_nodes.items()
        if chosen == "zephyr,uart-mcumgr"
    ] == DTShAutocomp.complete_dtchosen("zephyr,u", sh)


def test_dtshautocomp_complete_dtlabel() -> None:
    sh = DTSh(DTShTests.get_sample_dtmodel(), [])

    # The leading "&" is preserved when present in the completion scope
    # (auto-complete for devicetree paths).
    assert [
        RlStateDTLabel(label, node)
        for label, node in [
            ("&i2c0", DTShTests.get_sample_dtmodel()["/soc/i2c@40003000"]),
            (
                "&i2c0_default",
                DTShTests.get_sample_dtmodel()["/pin-controller/i2c0_default"],
            ),
            (
                "&i2c0_sleep",
                DTShTests.get_sample_dtmodel()["/pin-controller/i2c0_sleep"],
            ),
            ("&i2c1", DTShTests.get_sample_dtmodel()["/soc/i2c@40004000"]),
            (
                "&i2c1_default",
                DTShTests.get_sample_dtmodel()["/pin-controller/i2c1_default"],
            ),
            (
                "&i2c1_sleep",
                DTShTests.get_sample_dtmodel()["/pin-controller/i2c1_sleep"],
            ),
        ]
    ] == DTShAutocomp.complete_dtlabel("&i2c", sh)

    # A leading "&" is not enforced when not present in the completion scope
    # (auto-complete for labels).
    assert [
        RlStateDTLabel(label, node)
        for label, node in [
            ("i2c0", DTShTests.get_sample_dtmodel()["/soc/i2c@40003000"]),
            (
                "i2c0_default",
                DTShTests.get_sample_dtmodel()["/pin-controller/i2c0_default"],
            ),
            (
                "i2c0_sleep",
                DTShTests.get_sample_dtmodel()["/pin-controller/i2c0_sleep"],
            ),
        ]
    ] == DTShAutocomp.complete_dtlabel("i2c0", sh)


def test_dtshautocomp_complete_fspath() -> None:
    with DTShTests.from_there("fs"):
        # All file and directories in $PWD.
        assert [
            f"A{os.sep}",
            f"a{os.sep}",
            f"foo{os.sep}",
            "A.txt",
            "a.txt",
            "ab.txt",
        ] == [state.rlstr for state in DTShAutocomp.complete_fspath("")]

        # Complete path in current directory.
        assert [f"a{os.sep}", "a.txt", "ab.txt"] == [
            state.rlstr for state in DTShAutocomp.complete_fspath("a")
        ]

        # Complete path in sub-directory.
        assert [f"a{os.sep}x{os.sep}"] == [
            state.rlstr for state in DTShAutocomp.complete_fspath(f"a{os.sep}")
        ]

        # Unique matches are not hidden.
        assert [f"foo{os.sep}"] == [
            state.rlstr for state in DTShAutocomp.complete_fspath("foo")
        ]


def test_dtshautocomp_complete() -> None:
    cmd_mock = MockDTShCmd()
    cmd_ls = DTShCommand("ls", "", None, None)
    sh = DTSh(DTShTests.get_sample_dtmodel(), [cmd_mock, cmd_ls])
    autocomp = DTShAutocomp(sh)

    # Command line: "|"
    # Expects: all commands
    assert sorted(
        [RlStateDTShCommand(cmd.name, cmd) for cmd in sh.commands]
    ) == autocomp.complete("", "", 0, 0)

    # Command line: " |"
    # Expects: all commands
    assert sorted(
        [RlStateDTShCommand(cmd.name, cmd) for cmd in sh.commands]
    ) == autocomp.complete("", "", 1, 1)

    # Command line: " | "
    # Expects: all commands
    assert sorted(
        [RlStateDTShCommand(cmd.name, cmd) for cmd in sh.commands]
    ) == autocomp.complete("", " ", 1, 1)

    # Command line: "l|"
    # Expects: ls
    assert [RlStateDTShCommand(cmd_ls.name, cmd_ls)] == autocomp.complete(
        "l", "l", 0, 1
    )

    # Command line: " l|"
    # Expects: ls
    assert [RlStateDTShCommand(cmd_ls.name, cmd_ls)] == autocomp.complete(
        "l", " l", 1, 2
    )

    # Command line: "mockcmd --mockarg |"
    # Expects: possible argument values
    assert sorted(MockDTShArg.RL_STATES) == autocomp.complete(
        "", "mockcmd --mockarg ", 18, 18
    )

    # Command line: "mockcmd --mockarg|"
    # Expects: the option name as unique match.
    assert [RlStateDTShOption("--mockarg", MockDTShArg())] == autocomp.complete(
        "--mockarg", "mockcmd --mockarg", 8, 17
    )

    # Command line: "mockcmd --mockarg > |"
    # Expects: won't auto-complete redirection, ">" the argument value,
    # should auto-compelete with nodes.
    assert sorted(
        [RlStateDTPath(node.name, node) for node in sh.dt.root.children]
    ) == autocomp.complete("", "mockcmd --mockarg > ", 20, 20)

    # Command line: " ls > |"
    # Expect: test file-system entries.
    with DTShTests.from_there("fs"):
        assert [
            f"A{os.sep}",
            f"a{os.sep}",
            f"foo{os.sep}",
            "A.txt",
            "a.txt",
            "ab.txt",
        ] == [state.rlstr for state in autocomp.complete("", "ls > ", 5, 5)]

        # Command line: " ls >|"
        # Expects: missing space after ">".
        assert [] == autocomp.complete("", "ls >", 3, 3)
