#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors

"""Unit tests for zperf_profile.py.

These cover the parts of the profiler that turn a QEMU plugin report into
numbers: parsing, boot subtraction, the PC to function mapping, the
classification rules and the throughput arithmetic. They need no QEMU, no
plugin and no build.

What they deliberately do not cover is everything that drives an emulator:
run_one(), build_one() and cmd_run() are a QEMU command line, a west build and
the loop around them. Asserting a mocked argv against itself would say nothing
about whether the plugin loaded, whether '-d plugin' produced a report or
whether the guest halted, which are the ways those actually fail. The 'verify'
subcommand checks those against QEMU and binutils instead.
"""

import argparse
import contextlib
import json
import os
from unittest import mock

import pytest
import zperf_profile

# A synthetic symbol table covering all three lookup paths: a sized function
# (exact), a zero-sized assembly entry point (inferred through the bounds
# list), and a global STT_NOTYPE such as picolibc's memcpy (also inferred).
# The names are chosen so SYMBOL_GROUPS puts each one in a different group.
SYMS = [
    ("net_pkt_alloc", 0x1000, 0x40, "STT_FUNC", "STB_GLOBAL"),
    ("arch_swap", 0x2000, 0, "STT_FUNC", "STB_LOCAL"),
    ("memcpy", 0x3000, 0, "STT_NOTYPE", "STB_GLOBAL"),
]
LOAD = [(0x1000, 0x4000)]

HEADER = "collected {n} entries in the hash table\npc, tcount, icount, ecount\n"


def _table(symbols=None, load=None, source="<symbols>"):
    return zperf_profile.SymbolTable(
        SYMS if symbols is None else symbols,
        LOAD if load is None else load,
        source,
    )


def _write(tmp_path, name, text):
    path = tmp_path / name
    path.write_text(text)
    return str(path)


#
# parse_report()
#


def test_parse_report_well_formed(tmp_path):
    path = _write(
        tmp_path,
        "r.log",
        HEADER.format(n=3)
        + "0x0000000000100238, 1, 4, 10\n"
        + "0x0000000000100250, 2, 3, 7\n"
        + "0x0000000000100260, 1, 1, 1\n",
    )
    blocks, collected = zperf_profile.parse_report(path)

    assert blocks == {(0x100238, 4): 10, (0x100250, 3): 7, (0x100260, 1): 1}
    assert collected == 3


def test_parse_report_duplicate_keys_accumulate(tmp_path):
    path = _write(
        tmp_path,
        "r.log",
        "pc, tcount, icount, ecount\n0x0000000000100238, 1, 4, 10\n0x100238, 1, 4, 5\n",
    )
    blocks, collected = zperf_profile.parse_report(path)

    assert blocks == {(0x100238, 4): 15}
    assert collected == -1


def test_parse_report_keeps_one_pc_with_two_block_sizes_apart(tmp_path):
    """A block may be re-translated at a different length; that is not the same
    block and its cost per execution differs."""
    path = _write(
        tmp_path,
        "r.log",
        HEADER.format(n=2) + "0x0000000000100238, 1, 4, 10\n0x0000000000100238, 1, 2, 3\n",
    )
    blocks, _ = zperf_profile.parse_report(path)

    assert blocks == {(0x100238, 4): 10, (0x100238, 2): 3}


def test_parse_report_skips_stoptrigger_lines(tmp_path):
    """Plugins share one report stream, so the stop plugin's own output turns up
    in the middle of the block table."""
    path = _write(
        tmp_path,
        "r.log",
        HEADER.format(n=2)
        + "0x0000000000100238, 1, 4, 10\n"
        + "0x0000000000101f00 reached, exiting\n"
        + "stoptrigger: watching 0x101f00\n"
        + "0x0000000000100250, 1, 3, 7\n",
    )
    blocks, _ = zperf_profile.parse_report(path)

    assert blocks == {(0x100238, 4): 10, (0x100250, 3): 7}


@pytest.mark.parametrize(
    "row",
    [
        "0x0000000000100250, 1, 3\n",
        "0x0000000000100250, 1, 3, 7, 9\n",
        "0x0000000000100250\n",
    ],
)
def test_parse_report_skips_rows_with_the_wrong_field_count(tmp_path, row):
    path = _write(tmp_path, "r.log", HEADER.format(n=2) + "0x0000000000100238, 1, 4, 10\n" + row)
    blocks, _ = zperf_profile.parse_report(path)

    assert blocks == {(0x100238, 4): 10}


@pytest.mark.parametrize(
    "text",
    [
        # No "pc," header: not a block report, whatever else it holds.
        "collected 1 entries in the hash table\n0x0000000000100238, 1, 4, 10\n",
        # Header but no rows: the plugin ran and found nothing, or -d plugin
        # was missing and this is somebody else's output.
        HEADER.format(n=0),
        "",
    ],
)
def test_parse_report_rejects_a_non_report(tmp_path, text):
    path = _write(tmp_path, "r.log", text)

    with pytest.raises(SystemExit, match="does not look like a QEMU plugin block report"):
        zperf_profile.parse_report(path)


def test_parse_report_warns_when_truncated(tmp_path, capsys):
    """A stock QEMU plugin caps its output at the 20 hottest blocks. Believing
    such a report silently would understate every total in the profile."""
    path = _write(
        tmp_path,
        "r.log",
        HEADER.format(n=25) + "0x100238, 1, 4, 10\n0x100250, 1, 3, 7\n0x100260, 1, 1, 1\n",
    )
    blocks, collected = zperf_profile.parse_report(path)

    assert len(blocks) == 3
    assert collected == 25
    captured = capsys.readouterr()
    assert "the plugin reported 25 blocks but emitted only 3 rows" in captured.err
    assert captured.out == ""


def test_parse_report_does_not_warn_within_one_row(tmp_path, capsys):
    """The check allows one row of slack, so an off-by-one in the plugin's own
    bookkeeping does not produce a false alarm on every run."""
    path = _write(
        tmp_path,
        "r.log",
        HEADER.format(n=4) + "0x100238, 1, 4, 10\n0x100250, 1, 3, 7\n0x100260, 1, 1, 1\n",
    )
    zperf_profile.parse_report(path)

    assert capsys.readouterr().err == ""


#
# subtract()
#


def test_subtract_per_block():
    full = {(0x10, 4): 10, (0x20, 2): 5, (0x30, 1): 3}
    out = zperf_profile.subtract(full, {(0x10, 4): 4})

    assert out == {(0x10, 4): 6, (0x20, 2): 5, (0x30, 1): 3}


def test_subtract_removes_a_block_that_reaches_zero():
    full = {(0x10, 4): 10, (0x20, 2): 5}
    out = zperf_profile.subtract(full, {(0x20, 2): 5})

    assert (0x20, 2) not in out
    assert out == {(0x10, 4): 10}


def test_subtract_does_not_mutate_its_input():
    full = {(0x10, 4): 10, (0x20, 2): 5}
    zperf_profile.subtract(full, {(0x10, 4): 4, (0x20, 2): 5})

    assert full == {(0x10, 4): 10, (0x20, 2): 5}


def test_subtract_rejects_a_boot_run_that_ran_a_block_more_often():
    """The boot baseline is a prefix of the full run, so it can never execute a
    block more times. If it did, the two runs diverged and nothing downstream
    means anything."""
    with pytest.raises(SystemExit) as excinfo:
        zperf_profile.subtract({(0x10, 4): 3}, {(0x10, 4): 5})

    assert "0x10(+4): 3 < 5" in str(excinfo.value)
    assert "failed for 1 block(s)" in str(excinfo.value)


def test_subtract_rejects_a_boot_block_absent_from_the_full_run():
    with pytest.raises(SystemExit) as excinfo:
        zperf_profile.subtract({}, {(0x99, 1): 1})

    assert "0x99(+1): 0 < 1" in str(excinfo.value)


#
# SymbolTable
#


@pytest.mark.parametrize("pc", [0x1000, 0x1020, 0x103F])
def test_lookup_inside_a_sized_function_is_exact(pc):
    assert _table().lookup(pc) == ("net_pkt_alloc", "exact")


@pytest.mark.parametrize("pc", [0x2000, 0x2FFF])
def test_lookup_in_a_zero_sized_function_is_inferred(pc):
    assert _table().lookup(pc) == ("arch_swap", "inferred")


@pytest.mark.parametrize("pc", [0x3000, 0x3FFF])
def test_lookup_in_a_global_notype_symbol_is_inferred(pc):
    """picolibc's memcpy has no .type directive. Without STT_NOTYPE entry points
    every byte it moves would be charged to whichever symbol precedes it."""
    assert _table().lookup(pc) == ("memcpy", "inferred")


@pytest.mark.parametrize("pc", [0x1040, 0x2000])
def test_lookup_past_a_sized_function_is_unattributed(pc):
    table = _table(symbols=[("net_pkt_alloc", 0x1000, 0x40, "STT_FUNC", "STB_GLOBAL")])

    assert table.lookup(pc) == (zperf_profile.UNKNOWN, "bucket")


@pytest.mark.parametrize("pc", [0x0, 0xFFF, 0x4000, 0xFFFFFFF0])
def test_lookup_outside_the_image_is_firmware(pc):
    """The guest boots through a BIOS that is not in the ELF at all. Charging
    that to the nearest Zephyr symbol would be a lie; it is a separate bucket
    so the boot subtraction can be seen to remove it."""
    assert _table().lookup(pc) == (zperf_profile.FIRMWARE, "bucket")


@pytest.mark.parametrize("reverse", [False, True])
def test_lookup_prefers_a_global_symbol_over_a_local_one(reverse):
    symbols = [
        ("local_fn", 0x1000, 0x10, "STT_FUNC", "STB_LOCAL"),
        ("global_fn", 0x1000, 0x10, "STT_FUNC", "STB_GLOBAL"),
    ]
    if reverse:
        symbols.reverse()

    assert _table(symbols=symbols).lookup(0x1004) == ("global_fn", "exact")


@pytest.mark.parametrize(
    ("binding", "expected"),
    [
        # A local .L label lives inside a function; treating it as a boundary
        # would split that function's cost across two names.
        ("STB_LOCAL", "asm_fn"),
        # A global or weak entry point without a .type directive is a real
        # function and must delimit the one before it.
        ("STB_GLOBAL", "label"),
        ("STB_WEAK", "label"),
    ],
)
def test_a_notype_symbol_is_a_boundary_only_when_it_is_not_local(binding, expected):
    table = _table(
        symbols=[
            ("asm_fn", 0x1000, 0, "STT_FUNC", "STB_GLOBAL"),
            ("label", 0x1080, 0, "STT_NOTYPE", binding),
        ]
    )

    assert table.lookup(0x1090) == (expected, "inferred")


def test_a_sized_function_bounds_an_unsized_neighbour():
    table = _table(
        symbols=[
            ("asm_fn", 0x1000, 0, "STT_FUNC", "STB_GLOBAL"),
            ("sized", 0x1100, 0x20, "STT_FUNC", "STB_GLOBAL"),
        ]
    )

    assert table.lookup(0x10F0) == ("asm_fn", "inferred")
    assert table.lookup(0x1100) == ("sized", "exact")
    assert table.lookup(0x1120) == (zperf_profile.UNKNOWN, "bucket")


@pytest.mark.parametrize(
    ("pc", "expected"), [(0xFFF, False), (0x1000, True), (0x1FFF, True), (0x2000, False)]
)
def test_in_image(pc, expected):
    assert _table(load=[(0x1000, 0x2000)]).in_image(pc) is expected


def test_a_table_with_no_function_symbols_exits():
    with pytest.raises(SystemExit, match="fake.elf contains no function symbols"):
        _table(symbols=[], source="fake.elf")


def test_a_table_built_from_symbols_has_no_source_files():
    """Without DWARF the file column is empty and classification falls back to
    the symbol name rules, rather than guessing."""
    assert _table().source_file(0x1000, "/z", "/") is None


#
# attribute()
#

BLOCKS = {(0x1000, 4): 10, (0x1010, 2): 5, (0x2000, 3): 2, (0x0500, 1): 7}


def test_attribute_folds_blocks_into_functions():
    funcs, total, kinds = zperf_profile.attribute(BLOCKS, _table(), "/z", "/")

    assert funcs == {
        "net_pkt_alloc": {"insns": 50, "file": None, "group": "pktbuf"},
        "arch_swap": {"insns": 6, "file": None, "group": "kernel"},
        zperf_profile.FIRMWARE: {"insns": 7, "file": None, "group": "other"},
    }
    assert total == 63
    assert kinds == {"exact": 50, "inferred": 6, "bucket": 7}


def test_attribute_kinds_account_for_every_instruction():
    _funcs, total, kinds = zperf_profile.attribute(BLOCKS, _table(), "/z", "/")

    assert sum(kinds.values()) == total


def test_attribute_accumulates_one_function_seen_at_two_pcs():
    funcs, total, _kinds = zperf_profile.attribute(
        {(0x1000, 4): 10, (0x1010, 2): 5}, _table(), "/z", "/"
    )

    assert list(funcs) == ["net_pkt_alloc"]
    assert funcs["net_pkt_alloc"]["insns"] == 50
    assert total == 50


def test_attribute_charges_a_whole_block_to_the_function_it_starts_in():
    """A translation block ends at a branch, so it rarely straddles two
    functions, and its whole cost goes to the function containing its first
    instruction. Pin that, because it is the approximation the per-function
    numbers rest on."""
    funcs, total, _kinds = zperf_profile.attribute({(0x1030, 8): 1}, _table(), "/z", "/")

    assert list(funcs) == ["net_pkt_alloc"]
    assert total == 8


#
# classify() and _relativise()
#


@pytest.mark.parametrize(
    ("func", "source", "expected"),
    [
        # FILE_GROUPS is first match wins: this path matches the zperf rule and
        # the catch-all "^subsys/net/" rule, and the earlier one must win.
        ("zperf_udp_recv", "subsys/net/lib/zperf/zperf_udp_receiver.c", "harness"),
        ("net_pkt_alloc", "subsys/net/ip/net_pkt.c", "pktbuf"),
        ("calc_chksum", "subsys/net/ip/utils.c", "checksum"),
        ("tcp_in", "subsys/net/ip/tcp.c", "tcp"),
        ("net_eth_send", "subsys/net/l2/ethernet/ethernet.c", "net-other"),
        ("rb_insert", "lib/utils/rb.c", "kernel"),
        # A known source matching no file rule falls through to the symbol
        # rules rather than landing in "other".
        ("net_if_send", "boards/x86/qemu_x86/board.c", "net-other"),
        # No source at all: symbol rules only.
        ("memcpy", None, "libc"),
        ("z_swap", None, "kernel"),
        ("calc_chksum", None, "checksum"),
        # The symbol rules are anchored, so a mid-name match does not count.
        ("my_tcp_helper", None, "other"),
        ("do_thing", "boards/x86/qemu_x86/board.c", "other"),
    ],
)
def test_classify(func, source, expected):
    assert zperf_profile.classify(func, source) == expected


@pytest.mark.parametrize(
    ("path", "zephyr_base", "topdir", "expected"),
    [
        (
            "/home/u/zp/zephyr/subsys/net/ip/tcp.c",
            "/home/u/zp/zephyr",
            "/home/u/zp",
            "subsys/net/ip/tcp.c",
        ),
        (
            "/home/u/zp/modules/hal/nordic/f.c",
            "/home/u/zp/zephyr",
            "/home/u/zp",
            "modules/hal/nordic/f.c",
        ),
        ("/opt/sdk/lib/gcc/x.c", "/home/u/zp/zephyr", "/home/u/zp", "/opt/sdk/lib/gcc/x.c"),
        # A sibling directory sharing a prefix must not be truncated: the guard
        # is a separator, not a string prefix.
        ("/home/u/zp/zephyr-extra/f.c", "/home/u/zp/zephyr", "/home/u/zp", "zephyr-extra/f.c"),
        # A trailing slash on ZEPHYR_BASE is a realistic environment value and
        # must not defeat the match, or the path keeps a "zephyr/" prefix and
        # stops matching the anchored rules in FILE_GROUPS.
        ("/home/u/zp/zephyr/subsys/x.c", "/home/u/zp/zephyr/", "/home/u/zp", "subsys/x.c"),
        # An empty root must be skipped rather than treated as "/", which would
        # match every absolute path and eat its leading separator.
        ("/opt/sdk/x.c", "", "", "/opt/sdk/x.c"),
    ],
)
def test_relativise(path, zephyr_base, topdir, expected):
    assert zperf_profile._relativise(path, zephyr_base, topdir) == expected


#
# The throughput arithmetic
#


def test_ipb():
    assert zperf_profile._ipb(1000, 250) == 4.0
    assert zperf_profile._ipb(1000, 0) == 0.0
    assert zperf_profile._ipb(0, 250) == 0.0


@pytest.mark.parametrize("ipb", [1.0, 4.0, 12.5, 19.03])
def test_mbps_matches_the_documented_closed_form(ipb):
    """At shift 5 one instruction is 32 ns, so Mbps is exactly 250/ipb. This is
    the identity the whole method rests on."""
    assert zperf_profile._mbps(ipb, 5) == pytest.approx(250.0 / ipb)


@pytest.mark.parametrize(("shift", "expected"), [(0, 2000.0), (4, 125.0), (5, 62.5), (6, 31.25)])
def test_mbps_scales_with_the_icount_shift(shift, expected):
    assert zperf_profile._mbps(4.0, shift) == pytest.approx(expected)


def test_mbps_of_a_run_that_moved_no_payload():
    assert zperf_profile._mbps(0.0, 5) == 0.0


def test_mbps_gain():
    # Removing a quarter of the cost per byte.
    assert zperf_profile._mbps_gain(4.0, 1.0, 5) == pytest.approx(250.0 / 3.0 - 62.5)
    # Removing half of it doubles the throughput, so the gain equals the
    # original figure.
    assert zperf_profile._mbps_gain(4.0, 2.0, 5) == pytest.approx(62.5)


@pytest.mark.parametrize(
    ("ipb", "f_ipb"),
    [
        # A function accounting for the entire run: "free" means infinite
        # throughput, which is not a number worth printing.
        (4.0, 4.0),
        (4.0, 5.0),
        (4.0, 0.0),
        (0.0, 0.0),
    ],
)
def test_mbps_gain_degenerate_cases_are_zero_not_an_exception(ipb, f_ipb):
    assert zperf_profile._mbps_gain(ipb, f_ipb, 5) == 0.0


def test_pct():
    assert zperf_profile._pct(50, 200) == 25.0
    assert zperf_profile._pct(1, 0) == 0.0
    assert zperf_profile._pct(-5.0, 100.0) == -5.0


def test_group_totals():
    profile = {
        "functions": {
            "a": {"insns": 10, "group": "tcp"},
            "b": {"insns": 5, "group": "tcp"},
            "c": {"insns": 2, "group": "libc"},
        }
    }

    assert zperf_profile._group_totals(profile) == {"tcp": 15, "libc": 2}


#
# qemu_command() and _kernel_images()
#


@contextlib.contextmanager
def _fake_ninja(recipe, *, exists=True):
    """Make qemu_command() see *recipe* as ninja's run_qemu command line."""
    run = mock.Mock(return_value=mock.Mock(stdout=recipe))
    # A bool answers every existence check the same way; a list answers them in
    # order, which is how the "missing, then built" path is reached.
    seen = {"return_value": exists} if isinstance(exists, bool) else {"side_effect": exists}
    with (
        mock.patch("zperf_profile.shutil.which", return_value="/usr/bin/ninja"),
        mock.patch("zperf_profile.subprocess.run", run),
        mock.patch("zperf_profile.os.path.exists", **seen),
    ):
        yield run


def test_qemu_command_splits_the_recipe():
    recipe = (
        "cd /b/dir && qemu-system-i386 -m 8 -cpu qemu32 -nographic -no-reboot "
        "-serial mon:stdio -machine q35 -kernel /b/dir/zephyr/zephyr.elf"
    )
    with _fake_ninja(recipe):
        argv = zperf_profile.qemu_command("/b/dir")

    assert argv == [
        "qemu-system-i386",
        "-m",
        "8",
        "-cpu",
        "qemu32",
        "-no-reboot",
        "-machine",
        "q35",
        "-kernel",
        "/b/dir/zephyr/zephyr.elf",
    ]


def test_qemu_command_asks_ninja_for_the_run_target():
    with _fake_ninja("cd /b && qemu-system-i386 -kernel k.elf") as run:
        zperf_profile.qemu_command("/b")

    assert run.call_args_list[0].args[0] == [
        "/usr/bin/ninja",
        "-C",
        "/b",
        "-t",
        "commands",
        "zephyr/run_qemu",
    ]
    assert run.call_args_list[0].kwargs["check"] is True


def test_qemu_command_uses_the_last_recipe_line():
    """The run target is the last of the recipes ninja prints for it."""
    recipe = "cd /b && cmake -E echo building\ncd /b && qemu-system-i386 -kernel k.elf"
    with _fake_ninja(recipe):
        assert zperf_profile.qemu_command("/b") == ["qemu-system-i386", "-kernel", "k.elf"]


@pytest.mark.parametrize("quote", ["'", '"'])
def test_qemu_command_keeps_a_quoted_space_in_one_token(quote):
    """A build directory containing a space arrives quoted, because the recipe
    is written the way a shell would run it. Splitting on whitespace would tear
    the path in two and QEMU would not find the kernel."""
    image = "/b/my builds/x/zephyr/zephyr.elf"
    recipe = f"cd {quote}/b/my builds/x{quote} && qemu-system-i386 -kernel {quote}{image}{quote}"
    with _fake_ninja(recipe):
        argv = zperf_profile.qemu_command("/b/my builds/x")

    assert argv == ["qemu-system-i386", "-kernel", image]


def test_qemu_command_without_a_cd_prefix():
    with _fake_ninja("qemu-system-arm -M mps2-an385 -kernel z.elf"):
        assert zperf_profile.qemu_command("/b") == [
            "qemu-system-arm",
            "-M",
            "mps2-an385",
            "-kernel",
            "z.elf",
        ]


@pytest.mark.parametrize(
    "recipe", ["cd /b && echo nothing", "cd /b &&", "python3 foo.py", "cd /b && /usr/bin/gdb"]
)
def test_qemu_command_rejects_a_recipe_that_does_not_run_qemu(recipe):
    with _fake_ninja(recipe), pytest.raises(SystemExit, match="Unexpected run_qemu recipe"):
        zperf_profile.qemu_command("/b")


def test_qemu_command_rejects_a_build_without_a_run_target():
    with _fake_ninja(""), pytest.raises(SystemExit, match="No run_qemu target"):
        zperf_profile.qemu_command("/b")


def test_qemu_command_requires_ninja():
    with (
        mock.patch("zperf_profile.shutil.which", return_value=None),
        pytest.raises(SystemExit, match="ninja not found"),
    ):
        zperf_profile.qemu_command("/b")


@pytest.mark.parametrize("flag", ["-serial", "-chardev", "-mon", "-monitor", "-pidfile"])
def test_qemu_command_drops_a_console_flag_and_its_argument(flag):
    """The profiling run is unattended and writes the console to a file, so
    anything that would attach it to a terminal has to go, argument included."""
    with _fake_ninja(f"cd /b && qemu-system-i386 {flag} VALUE -kernel k.elf"):
        argv = zperf_profile.qemu_command("/b")

    assert argv == ["qemu-system-i386", "-kernel", "k.elf"]


def test_qemu_command_drops_nographic_without_eating_the_next_argument():
    with _fake_ninja("cd /b && qemu-system-i386 -nographic -kernel k.elf"):
        assert zperf_profile.qemu_command("/b") == ["qemu-system-i386", "-kernel", "k.elf"]


def test_qemu_command_builds_a_missing_kernel_image():
    """qemu_x86_64 splits the image in two, produced by a target the run target
    depends on. Running QEMU directly skips that dependency."""
    recipe = "cd /b && qemu-system-x86_64 -device loader,file=/b/locore.elf -kernel /b/main.elf"
    with _fake_ninja(recipe, exists=[False, False, True, True]) as run:
        zperf_profile.qemu_command("/b")

    assert run.call_args_list[1].args[0] == ["/usr/bin/ninja", "-C", "/b", "qemu_kernel_target"]


def test_qemu_command_gives_up_when_the_image_is_still_missing():
    with (
        _fake_ninja("cd /b && qemu-system-i386 -kernel k.elf", exists=False),
        pytest.raises(SystemExit, match="Still missing after building qemu_kernel_target"),
    ):
        zperf_profile.qemu_command("/b")


@pytest.mark.parametrize(
    ("argv", "expected"),
    [
        (["-kernel", "/a/zephyr.elf"], ["/a/zephyr.elf"]),
        (["-device", "loader,file=/a/locore.elf,addr=0x1000"], ["/a/locore.elf"]),
        (
            ["-device", "loader,file=/a/locore.elf", "-device", "loader,file=/a/main.elf"],
            ["/a/locore.elf", "/a/main.elf"],
        ),
        (["-kernel", "/a/z.elf", "-device", "loader,file=/a/b.elf"], ["/a/z.elf", "/a/b.elf"]),
        (["-nographic", "-m", "8"], []),
        (["-device", "loader,addr=0x100"], []),
        # A trailing -kernel with no value must not index past the end.
        (["-m", "8", "-kernel"], []),
    ],
)
def test_kernel_images(argv, expected):
    assert zperf_profile._kernel_images(argv) == expected


#
# parse_console()
#


def test_parse_console_extracts_results_and_byte_counts(tmp_path):
    path = _write(
        tmp_path,
        "c.log",
        "*** Booting Zephyr OS build v4.4.0 ***\n"
        "[00:00:00.017,000] <inf> net_zperf: Binding to 127.0.0.1\n"
        "ZPERF-RESULT tcp4_mbps=91.25\n"
        "ZPERF-INFO tcp4_bytes=1048576 tcp4_us=1001300\n"
        "ZPERF-RESULT udp4_mbps=120.5\n"
        "ZPERF-INFO udp4_bytes=2097152 udp4_us=1000800\n"
        "ZPERF-RESULT ignored\n"
        "ZPERF-DONE\n",
    )

    assert zperf_profile.parse_console(path) == (
        {"tcp4": 91.25, "udp4": 120.5},
        {"tcp4": 1048576, "udp4": 2097152},
    )


def test_parse_console_takes_the_last_value_for_a_label(tmp_path):
    path = _write(tmp_path, "c.log", "ZPERF-RESULT tcp4_mbps=1.0\nZPERF-RESULT tcp4_mbps=2.0\n")

    assert zperf_profile.parse_console(path)[0] == {"tcp4": 2.0}


def test_parse_console_tolerates_undecodable_bytes(tmp_path):
    """The console log carries the shell's escape sequences and whatever the
    guest emitted before the UART settled."""
    path = tmp_path / "c.log"
    path.write_bytes(b"\xff\xfe \x1b[1;32muart:~$ ZPERF-RESULT tcp4_mbps=5.0\n")

    assert zperf_profile.parse_console(str(path)) == ({"tcp4": 5.0}, {})


def test_parse_console_without_a_file():
    assert zperf_profile.parse_console("/nonexistent/console.log") == ({}, {})


#
# render_diff()
#


def _profile(total, byts, funcs):
    return {
        "total_insns": total,
        "bytes": byts,
        "functions": {n: {"insns": i, "group": "other", "file": None} for n, i in funcs.items()},
    }


def test_render_diff_reports_more_instructions_per_byte_as_a_regression(capsys):
    ok = zperf_profile.render_diff(
        {"udp4": _profile(1000, 250, {"f": 1000})},
        {"udp4": _profile(1100, 250, {"f": 1100})},
        1.0,
        5,
    )

    assert ok is False
    out = capsys.readouterr().out
    assert "ipb 4.000 -> 4.400  (+10.00%)  REGRESSION" in out


def test_render_diff_reports_fewer_instructions_per_byte_as_an_improvement(capsys):
    ok = zperf_profile.render_diff(
        {"udp4": _profile(1000, 250, {"f": 1000})},
        {"udp4": _profile(900, 250, {"f": 900})},
        1.0,
        5,
    )

    assert ok is True
    out = capsys.readouterr().out
    assert "(-10.00%)  OK" in out
    assert "REGRESSION" not in out


def test_render_diff_accepts_a_change_within_tolerance(capsys):
    ok = zperf_profile.render_diff(
        {"udp4": _profile(1000, 250, {"f": 1000})},
        {"udp4": _profile(1005, 250, {"f": 1005})},
        1.0,
        5,
    )

    assert ok is True
    assert "(+0.50%)  OK" in capsys.readouterr().out


def test_render_diff_accepts_an_identical_profile_at_zero_tolerance(capsys):
    profile = {"udp4": _profile(1000, 250, {"f": 1000})}
    ok = zperf_profile.render_diff(profile, profile, 0.0, 5)

    assert ok is True
    assert "(+0.00%)  OK" in capsys.readouterr().out


def test_render_diff_annotates_functions_that_appeared_and_vanished(capsys):
    zperf_profile.render_diff(
        {"udp4": _profile(150, 250, {"stays": 100, "removed_fn": 50})},
        {"udp4": _profile(170, 250, {"stays": 100, "brand_new": 70})},
        100.0,
        5,
    )

    out = capsys.readouterr().out
    assert "brand_new" in out
    assert "+70  NEW" in out
    assert "-50  GONE" in out


def test_render_diff_omits_functions_that_did_not_move(capsys):
    zperf_profile.render_diff(
        {"udp4": _profile(150, 250, {"unchanged_fn": 100, "moved_fn": 50})},
        {"udp4": _profile(170, 250, {"unchanged_fn": 100, "moved_fn": 70})},
        100.0,
        5,
    )

    out = capsys.readouterr().out
    assert "moved_fn" in out
    assert "unchanged_fn" not in out


def test_render_diff_honours_top(capsys):
    base = _profile(0, 250, {f"f{i}": 0 for i in range(5)})
    cur = _profile(0, 250, {f"f{i}": (i + 1) * 100 for i in range(5)})
    zperf_profile.render_diff({"udp4": base}, {"udp4": cur}, 100.0, 2)

    out = capsys.readouterr().out
    assert "f4" in out
    assert "f3" in out
    assert "f2" not in out


@pytest.mark.parametrize(
    ("missing_from", "expected"), [("current", "baseline"), ("baseline", "current")]
)
def test_render_diff_flags_a_transfer_present_on_only_one_side(capsys, missing_from, expected):
    both = {"tcp4": _profile(1000, 250, {"f": 1000}), "udp6": _profile(1000, 250, {"f": 1000})}
    one = {"tcp4": _profile(1000, 250, {"f": 1000})}
    baseline, current = (both, one) if missing_from == "current" else (one, both)

    ok = zperf_profile.render_diff(baseline, current, 1.0, 5)

    assert ok is False
    out = capsys.readouterr().out
    assert "=== udp6 ===" in out
    assert f"only present in {expected}" in out


def test_render_diff_with_no_payload_does_not_divide_by_zero(capsys):
    ok = zperf_profile.render_diff(
        {"udp4": _profile(1000, 0, {"f": 1000})},
        {"udp4": _profile(1000, 0, {"f": 1000})},
        1.0,
        5,
    )

    assert ok is True
    assert "(+0.00%)" in capsys.readouterr().out


#
# validate_path() and validate_qemu_arg_path()
#


def test_validate_path_accepts_a_relative_path_inside_the_base(tmp_path, monkeypatch):
    monkeypatch.chdir(tmp_path)
    (tmp_path / "sub").mkdir()
    (tmp_path / "sub" / "f.txt").write_text("x")

    resolved = zperf_profile.validate_path("sub/f.txt", str(tmp_path), for_write=False)

    assert resolved == os.path.realpath(tmp_path / "sub" / "f.txt")


def test_validate_path_accepts_an_absolute_path_inside_the_base(tmp_path):
    (tmp_path / "f.txt").write_text("x")

    resolved = zperf_profile.validate_path(str(tmp_path / "f.txt"), str(tmp_path), for_write=False)

    assert resolved == os.path.realpath(tmp_path / "f.txt")


@pytest.mark.parametrize("path", ["../outside", "/etc/passwd", "sub/../../outside"])
def test_validate_path_rejects_an_escape(tmp_path, monkeypatch, path):
    monkeypatch.chdir(tmp_path)

    with pytest.raises(SystemExit, match="outside the permitted base directory"):
        zperf_profile.validate_path(path, str(tmp_path), for_write=False)


def test_validate_path_rejects_an_escape_through_a_symlink(tmp_path, monkeypatch):
    """Resolution follows symlinks, so a link out of the tree is not a way past
    the check."""
    monkeypatch.chdir(tmp_path)
    (tmp_path / "link").symlink_to(tmp_path.parent)

    with pytest.raises(SystemExit, match="outside the permitted base directory"):
        zperf_profile.validate_path("link/x", str(tmp_path), for_write=False)


@pytest.mark.parametrize("path", ["a\x00b", ""])
def test_validate_path_rejects_an_unusable_path(tmp_path, path):
    with pytest.raises(SystemExit, match="Invalid path"):
        zperf_profile.validate_path(path, str(tmp_path), for_write=False)


def test_validate_path_rejects_a_base_directory_that_is_not_there(tmp_path):
    with pytest.raises(SystemExit, match="does not exist"):
        zperf_profile.validate_path("f.txt", str(tmp_path / "nope"), for_write=False)


def test_validate_path_for_write_accepts_a_file_that_does_not_exist_yet(tmp_path):
    resolved = zperf_profile.validate_path(
        str(tmp_path / "new.json"), str(tmp_path), for_write=True
    )

    assert resolved == os.path.realpath(tmp_path / "new.json")


def test_validate_path_for_write_rejects_a_missing_parent(tmp_path):
    with pytest.raises(SystemExit, match="is not a directory"):
        zperf_profile.validate_path(str(tmp_path / "a/b/c.json"), str(tmp_path), for_write=True)


def test_validate_path_for_write_rejects_a_directory(tmp_path):
    (tmp_path / "d").mkdir()

    with pytest.raises(SystemExit, match="it is a directory"):
        zperf_profile.validate_path(str(tmp_path / "d"), str(tmp_path), for_write=True)


@pytest.mark.parametrize("name", ["lib plugin.so", "lib,plugin.so", "lib\tplugin.so"])
def test_validate_qemu_arg_path_rejects_a_path_qemu_would_split(tmp_path, name):
    """The plugin argument is comma separated and CMake splits QEMU_EXTRA_FLAGS
    on whitespace, so such a path would be silently torn into several
    arguments rather than failing."""
    with pytest.raises(SystemExit, match="QEMU argument splitting would break"):
        zperf_profile.validate_qemu_arg_path(str(tmp_path / name), str(tmp_path), for_write=False)


def test_validate_qemu_arg_path_accepts_an_ordinary_path(tmp_path):
    resolved = zperf_profile.validate_qemu_arg_path(
        str(tmp_path / "libhotblocks.so"), str(tmp_path), for_write=False
    )

    assert resolved == os.path.realpath(tmp_path / "libhotblocks.so")


#
# build_profile(): everything above, wired together
#

GOLDEN_REPORT = (
    HEADER.format(n=4)
    + "0x0000000000001000, 1, 4, 20\n"
    + "0x0000000000002000, 1, 3, 10\n"
    + "0x0000000000003000, 1, 2, 5\n"
    + "0x0000000000000500, 1, 1, 8\n"
)
GOLDEN_BOOT = HEADER.format(n=2) + "0x0000000000001000, 1, 4, 5\n0x0000000000000500, 1, 1, 8\n"
GOLDEN_CONSOLE = (
    "*** Booting Zephyr OS ***\n"
    "ZPERF-INFO tcp4_bytes=25 tcp4_us=3200\n"
    "ZPERF-RESULT tcp4_mbps=62.5\n"
)


def _golden(tmp_path, boot=True, console=True):
    report = _write(tmp_path, "tcp4.report", GOLDEN_REPORT)
    boot_path = _write(tmp_path, "tcp4.boot.report", GOLDEN_BOOT) if boot else None
    console_path = _write(tmp_path, "tcp4.console", GOLDEN_CONSOLE) if console else None
    with mock.patch.object(zperf_profile.SymbolTable, "from_elf", return_value=_table()):
        return zperf_profile.build_profile(
            "tcp4", report, "zephyr.elf", boot_path, console_path, "qemu_x86", 5, "/z", "/"
        )


def test_build_profile_golden(tmp_path):
    """A report, its boot baseline and the console, end to end.

    The numbers are chosen so the profile is self consistent: after the boot
    prefix comes off, 100 instructions carried 25 payload bytes, which is 4.0
    instructions per byte, which at shift 5 is exactly the 62.5 Mbps the guest
    itself reported.
    """
    assert _golden(tmp_path) == {
        "schema": 1,
        "label": "tcp4",
        "platform": "qemu_x86",
        "icount_shift": 5,
        "bytes": 25,
        "measured_mbps": 62.5,
        "total_insns": 100,
        "boot_insns": 28,
        "kinds": {"exact": 60, "inferred": 40},
        "functions": {
            "net_pkt_alloc": {"insns": 60, "file": None, "group": "pktbuf"},
            "arch_swap": {"insns": 30, "file": None, "group": "kernel"},
            "memcpy": {"insns": 10, "file": None, "group": "libc"},
        },
    }


def test_build_profile_reconstructs_the_measured_throughput(tmp_path):
    """The claim the whole method rests on: the instruction count and the
    throughput the guest measured are the same number in different units."""
    profile = _golden(tmp_path)
    ipb = zperf_profile._ipb(profile["total_insns"], profile["bytes"])

    assert zperf_profile._mbps(ipb, profile["icount_shift"]) == profile["measured_mbps"]


def test_build_profile_boot_subtraction_removes_the_firmware(tmp_path):
    """Before subtraction the BIOS the guest boots through dominates the
    unattributed bucket; after it, nothing should be left there."""
    with_boot = _golden(tmp_path)
    without_boot = _golden(tmp_path, boot=False)

    assert without_boot["kinds"]["bucket"] == 8
    assert "bucket" not in with_boot["kinds"]
    assert without_boot["total_insns"] == 128
    assert with_boot["boot_insns"] == 28


def test_build_profile_without_a_console(tmp_path):
    profile = _golden(tmp_path, console=False)

    assert profile["bytes"] == 0
    assert profile["measured_mbps"] is None


def test_build_profile_round_trips_through_json(tmp_path):
    """The diff subcommand reads back exactly what run saved, so the profile
    has to survive a JSON round trip unchanged."""
    profile = _golden(tmp_path)

    assert json.loads(json.dumps(profile, sort_keys=True)) == profile


#
# render() and cmd_diff()
#


def test_render_reports_the_reconstructed_throughput(tmp_path, capsys):
    zperf_profile.render(_golden(tmp_path), 25)

    out = capsys.readouterr().out
    assert "=== tcp4 (qemu_x86) ===" in out
    assert "payload           25 B   ipb    4.000" in out
    assert "boot subtracted 28" in out
    assert "measured   62.500 Mbps   from profile   62.500 Mbps" in out
    assert "attribution  exact 60.00%   inferred 40.00%   unattributed  0.00%" in out
    for group in ("pktbuf", "kernel", "libc"):
        assert group in out


def test_render_honours_top(tmp_path, capsys):
    zperf_profile.render(_golden(tmp_path), 1)

    out = capsys.readouterr().out
    assert "net_pkt_alloc" in out
    # arch_swap is out of the per-function table; only its group is rolled up.
    assert "arch_swap" not in out


def test_render_survives_a_run_that_measured_nothing(capsys):
    zperf_profile.render(
        {"label": "x", "total_insns": 0, "bytes": 0, "functions": {}, "kinds": {}}, 5
    )

    assert "=== x" in capsys.readouterr().out


def _diff_args(tmp_path, baseline, current, tolerance=1.0):
    for name, profiles in (("base.json", baseline), ("cur.json", current)):
        (tmp_path / name).write_text(json.dumps(profiles))
    return argparse.Namespace(
        baseline=str(tmp_path / "base.json"),
        current=str(tmp_path / "cur.json"),
        base_dir=str(tmp_path),
        tolerance=tolerance,
        top=5,
    )


def test_cmd_diff_exits_zero_when_nothing_moved(tmp_path):
    profiles = {"udp4": _profile(1000, 250, {"f": 1000})}

    assert zperf_profile.cmd_diff(_diff_args(tmp_path, profiles, profiles)) == 0


def test_cmd_diff_exits_non_zero_on_a_regression(tmp_path, capsys):
    args = _diff_args(
        tmp_path,
        {"udp4": _profile(1000, 250, {"f": 1000})},
        {"udp4": _profile(1100, 250, {"f": 1100})},
    )

    assert zperf_profile.cmd_diff(args) == 1
    assert "At least one transfer regressed beyond the tolerance." in capsys.readouterr().out
