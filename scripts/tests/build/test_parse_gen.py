#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""
Pytest suite for parse_secure_calls.py and gen_secure_calls.py.

Tests the code-generation pipeline end-to-end on host, without a target build.
Run with:  pytest scripts/tests/build/test_parse_gen.py -v
from the Zephyr root.
"""

import json
import os
import subprocess
import sys
import textwrap
from pathlib import Path

import pytest

ZEPHYR_BASE = Path(__file__).resolve().parents[3]
PARSE_SCRIPT = ZEPHYR_BASE / "scripts" / "build" / "parse_secure_calls.py"
GEN_SCRIPT   = ZEPHYR_BASE / "scripts" / "build" / "gen_secure_calls.py"
PYTHON       = sys.executable


def run_parse(tmp_path, header_content):
    """Write a header, run parse_secure_calls.py, return parsed JSON list."""
    hdr = tmp_path / "test_decls.h"
    hdr.write_text(header_content)
    json_out = tmp_path / "sc.json"
    result = subprocess.run(
        [PYTHON, str(PARSE_SCRIPT),
         "--include", str(tmp_path),
         "--json-file", str(json_out)],
        capture_output=True, text=True,
    )
    assert result.returncode == 0, f"parse failed:\n{result.stderr}"
    return json.loads(json_out.read_text())


def run_gen(tmp_path, parsed_json, target):
    """Run gen_secure_calls.py with the given target, return output dir Path."""
    json_file = tmp_path / "sc.json"
    json_file.write_text(json.dumps(parsed_json))
    out_dir = tmp_path / f"gen_{target}"
    list_h  = tmp_path / "secure_call_list.h"
    result = subprocess.run(
        [PYTHON, str(GEN_SCRIPT),
         "--json-file",        str(json_file),
         "--target",           target,
         "--base-output",      str(out_dir),
         "--secure-call-list", str(list_h)],
        cwd=str(tmp_path),
        capture_output=True, text=True,
    )
    assert result.returncode == 0, f"gen --target {target} failed:\n{result.stderr}"
    return out_dir, list_h


class TestParse:
    def test_finds_secure_call(self, tmp_path):
        data = run_parse(tmp_path, "__secure_call int foo(int a, int b);")
        assert len(data) == 1
        (match_group, _fn, emit) = data[0]
        assert match_group[0].strip() == "int foo"
        assert "int a" in match_group[1]
        assert emit is True

    def test_finds_always_inline(self, tmp_path):
        data = run_parse(tmp_path,
                         "__secure_call_always_inline int bar(void);")
        assert len(data) == 1
        assert "int bar" in data[0][0][0]

    def test_void_return(self, tmp_path):
        data = run_parse(tmp_path,
                         "__secure_call void fill(uint8_t *buf, size_t len);")
        assert len(data) == 1
        func, args = data[0][0]
        assert func.strip() == "void fill"
        assert "uint8_t *buf" in args

    def test_zero_args(self, tmp_path):
        data = run_parse(tmp_path, "__secure_call int nop(void);")
        assert len(data) == 1
        func, args = data[0][0]
        assert "nop" in func
        assert args.strip() == "void"

    def test_multiple_functions(self, tmp_path):
        src = textwrap.dedent("""\
            __secure_call int alpha(int x);
            __secure_call void beta(void);
            __secure_call int gamma(uint8_t *p, size_t n);
        """)
        data = run_parse(tmp_path, src)
        names = [d[0][0].split()[-1] for d in data]
        assert names == ["alpha", "beta", "gamma"]

    def test_ignores_non_decorated(self, tmp_path):
        src = textwrap.dedent("""\
            int plain_func(int x);
            static inline int inline_func(void);
            __secure_call int decorated(int a);
        """)
        data = run_parse(tmp_path, src)
        assert len(data) == 1
        assert "decorated" in data[0][0][0]

    def test_emit_false_for_scan_dirs(self, tmp_path):
        """Files from --scan dirs have emit=False."""
        scan_dir = tmp_path / "scan"
        scan_dir.mkdir()
        (scan_dir / "scan_decls.h").write_text(
            "__secure_call int scanned(int a);"
        )
        json_out = tmp_path / "sc.json"
        result = subprocess.run(
            [PYTHON, str(PARSE_SCRIPT),
             "--scan",      str(scan_dir),
             "--json-file", str(json_out)],
            capture_output=True, text=True,
        )
        assert result.returncode == 0
        data = json.loads(json_out.read_text())
        assert len(data) == 1
        assert data[0][2] is False  # emit=False

    def test_idempotent_output(self, tmp_path):
        """Running twice with same input must not modify the JSON file."""
        src = "__secure_call int foo(int a);"
        run_parse(tmp_path, src)
        json_file = tmp_path / "sc.json"
        mtime1 = json_file.stat().st_mtime
        run_parse(tmp_path, src)
        mtime2 = json_file.stat().st_mtime
        assert mtime1 == mtime2, "JSON file was rewritten despite identical input"

    def test_skips_toolchain_common_h(self, tmp_path):
        """toolchain/common.h defines __secure_call itself — must not be scanned."""
        tc_dir = tmp_path / "toolchain"
        tc_dir.mkdir()
        (tc_dir / "common.h").write_text(
            "#define __secure_call static inline\n"
            "__secure_call int should_not_appear(void);"
        )
        data = run_parse(tmp_path,
                         "__secure_call int real_func(int x);")
        names = [d[0][0].split()[-1] for d in data]
        assert "should_not_appear" not in names
        assert "real_func" in names


class TestGenNS:
    def _get_wrapper(self, tmp_path, src):
        parsed = run_parse(tmp_path, src)
        out_dir, _ = run_gen(tmp_path, parsed, "ns")
        return (out_dir / "test_decls.h").read_text()

    def test_extern_mrsh_declaration(self, tmp_path):
        content = self._get_wrapper(
            tmp_path, "__secure_call int foo(int a, int b);")
        assert "extern int z_secure_mrsh_foo(uintptr_t arg0, uintptr_t arg1);" in content

    def test_extern_impl_declaration(self, tmp_path):
        content = self._get_wrapper(
            tmp_path, "__secure_call int foo(int a, int b);")
        assert "extern int z_secure_impl_foo(int a, int b);" in content

    def test_static_inline_wrapper(self, tmp_path):
        content = self._get_wrapper(
            tmp_path, "__secure_call int foo(int a, int b);")
        assert "static inline int foo(int a, int b)" in content

    def test_ns_config_guard(self, tmp_path):
        content = self._get_wrapper(
            tmp_path, "__secure_call int foo(int a);")
        assert "CONFIG_TRUSTED_EXECUTION_NONSECURE" in content

    def test_union_packing(self, tmp_path):
        content = self._get_wrapper(
            tmp_path, "__secure_call int foo(int a, int b);")
        assert "union { uintptr_t x; int val; } p0 = { .val = a };" in content
        assert "union { uintptr_t x; int val; } p1 = { .val = b };" in content

    def test_lock_unlock_calls(self, tmp_path):
        content = self._get_wrapper(
            tmp_path, "__secure_call int foo(int a);")
        assert "z_secure_call_lock();" in content
        assert "z_secure_call_unlock();" in content

    def test_single_image_fallback(self, tmp_path):
        content = self._get_wrapper(
            tmp_path, "__secure_call int foo(int a);")
        assert "compiler_barrier();" in content
        assert "z_secure_impl_foo(a);" in content

    def test_void_return_no_ret_variable(self, tmp_path):
        content = self._get_wrapper(
            tmp_path, "__secure_call void bar(uint8_t *p, size_t n);")
        assert "void bar(" in content
        # void path calls mrsh but does not assign _ret
        assert "_ret" not in content

    def test_void_return_no_return_in_fallback(self, tmp_path):
        content = self._get_wrapper(
            tmp_path, "__secure_call void bar(uint8_t *p, size_t n);")
        # fallback must not have 'return z_secure_impl_bar'
        lines = [l.strip() for l in content.splitlines()]
        assert "return z_secure_impl_bar(p, n);" not in lines

    def test_zero_args(self, tmp_path):
        content = self._get_wrapper(
            tmp_path, "__secure_call int nop(void);")
        assert "static inline int nop(void)" in content
        assert "z_secure_mrsh_nop()" in content

    def test_pointer_arg_union(self, tmp_path):
        content = self._get_wrapper(
            tmp_path, "__secure_call int read(const uint8_t *buf, size_t len);")
        assert "union { uintptr_t x; const uint8_t * val; } p0" in content
        assert "union { uintptr_t x; size_t val; } p1" in content

    def test_include_guard(self, tmp_path):
        content = self._get_wrapper(
            tmp_path, "__secure_call int foo(int a);")
        assert "#ifndef Z_INCLUDE_SECURE_CALLS_TEST_DECLS_H" in content
        assert "#define Z_INCLUDE_SECURE_CALLS_TEST_DECLS_H" in content

    def test_includes_secure_call_headers(self, tmp_path):
        content = self._get_wrapper(
            tmp_path, "__secure_call int foo(int a);")
        assert "#include <zephyr/secure_call_list.h>" in content
        assert "#include <zephyr/secure_call.h>" in content

    def test_too_many_args_produces_error(self, tmp_path):
        """Functions with >4 args must emit a #error in the NS wrapper."""
        src = "__secure_call int fat(int a, int b, int c, int d, int e);"
        content = self._get_wrapper(tmp_path, src)
        assert "#error" in content
        assert "Phase 1 limit" in content

    def test_secure_call_list_defines(self, tmp_path):
        src = textwrap.dedent("""\
            __secure_call int alpha(int x);
            __secure_call void beta(void);
        """)
        parsed = run_parse(tmp_path, src)
        _, list_h = run_gen(tmp_path, parsed, "ns")
        content = list_h.read_text()
        assert "K_SECURE_CALL_ALPHA" in content
        assert "K_SECURE_CALL_BETA" in content
        assert "K_SECURE_CALL_LIMIT" in content


class TestGenS:
    def _get_entries(self, tmp_path, src):
        parsed = run_parse(tmp_path, src)
        out_dir, _ = run_gen(tmp_path, parsed, "s")
        return (out_dir / "secure_call_entries.c").read_text()

    def test_cmse_nonsecure_entry_attribute(self, tmp_path):
        content = self._get_entries(
            tmp_path, "__secure_call int foo(int a, int b);")
        assert "__attribute__((cmse_nonsecure_entry))" in content

    def test_entry_function_signature(self, tmp_path):
        content = self._get_entries(
            tmp_path, "__secure_call int foo(int a, int b);")
        assert "int z_secure_mrsh_foo(uintptr_t arg0, uintptr_t arg1)" in content

    def test_vrfy_extern_declaration(self, tmp_path):
        content = self._get_entries(
            tmp_path, "__secure_call int foo(int a, int b);")
        assert "extern int z_secure_vrfy_foo(int a, int b);" in content

    def test_arg_unmarshal(self, tmp_path):
        content = self._get_entries(
            tmp_path, "__secure_call int foo(int a, int b);")
        assert "union { uintptr_t x; int val; } p0;" in content
        assert "p0.x = arg0;" in content
        assert "union { uintptr_t x; int val; } p1;" in content
        assert "p1.x = arg1;" in content

    def test_calls_vrfy(self, tmp_path):
        content = self._get_entries(
            tmp_path, "__secure_call int foo(int a, int b);")
        assert "z_secure_vrfy_foo(p0.val, p1.val);" in content

    def test_return_from_vrfy(self, tmp_path):
        content = self._get_entries(
            tmp_path, "__secure_call int foo(int a);")
        assert "return z_secure_vrfy_foo(" in content

    def test_void_no_return(self, tmp_path):
        content = self._get_entries(
            tmp_path, "__secure_call void bar(uint8_t *p, size_t n);")
        assert "void z_secure_mrsh_bar(" in content
        # void path calls vrfy but does not return its value
        lines = [l.strip() for l in content.splitlines()]
        assert "return z_secure_vrfy_bar(p0.val, p1.val);" not in lines
        assert "z_secure_vrfy_bar(p0.val, p1.val);" in lines

    def test_zero_args(self, tmp_path):
        content = self._get_entries(
            tmp_path, "__secure_call int nop(void);")
        assert "int z_secure_mrsh_nop(void)" in content
        assert "return z_secure_vrfy_nop();" in content

    def test_arm_secure_firmware_guard(self, tmp_path):
        content = self._get_entries(
            tmp_path, "__secure_call int foo(int a);")
        assert "#if defined(CONFIG_ARM_SECURE_FIRMWARE)" in content
        assert "#endif /* CONFIG_ARM_SECURE_FIRMWARE */" in content

    def test_includes_handler_header(self, tmp_path):
        content = self._get_entries(
            tmp_path, "__secure_call int foo(int a);")
        assert "#include <zephyr/internal/secure_call_handler.h>" in content

    def test_too_many_args_raises(self, tmp_path):
        """Functions with >4 args on S side must raise an exception."""
        src = "__secure_call int fat(int a, int b, int c, int d, int e);"
        parsed = run_parse(tmp_path, src)
        json_file = tmp_path / "sc_fat.json"
        json_file.write_text(json.dumps(parsed))
        out_dir = tmp_path / "gen_s_fat"
        result = subprocess.run(
            [PYTHON, str(GEN_SCRIPT),
             "--json-file",   str(json_file),
             "--target",      "s",
             "--base-output", str(out_dir)],
            capture_output=True, text=True,
        )
        assert result.returncode != 0
        assert "Phase 1 limit" in result.stderr or "Phase 1 limit" in result.stdout

    def test_multiple_functions_in_one_file(self, tmp_path):
        src = textwrap.dedent("""\
            __secure_call int alpha(int x);
            __secure_call void beta(void);
            __secure_call int gamma(uint8_t *p, size_t n);
        """)
        content = self._get_entries(tmp_path, src)
        assert "z_secure_mrsh_alpha" in content
        assert "z_secure_mrsh_beta" in content
        assert "z_secure_mrsh_gamma" in content

    def test_non_emit_functions_excluded(self, tmp_path):
        """Functions from --scan dirs (emit=False) must not appear in S entries."""
        scan_dir = tmp_path / "scan"
        scan_dir.mkdir()
        (scan_dir / "scan_decls.h").write_text(
            "__secure_call int scanned_only(int a);"
        )
        json_out = tmp_path / "sc.json"
        subprocess.run(
            [PYTHON, str(PARSE_SCRIPT),
             "--scan",      str(scan_dir),
             "--json-file", str(json_out)],
            check=True, capture_output=True,
        )
        parsed = json.loads(json_out.read_text())
        out_dir, _ = run_gen(tmp_path, parsed, "s")
        content = (out_dir / "secure_call_entries.c").read_text()
        assert "scanned_only" not in content
