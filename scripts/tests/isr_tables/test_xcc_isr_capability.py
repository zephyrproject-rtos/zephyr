# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Check XCC's builtin-counter capability using real preprocessors and Kconfig."""

import importlib
import os
import re
import shlex
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[3]


@pytest.fixture(params=['gcc', 'clang'])
def compiler(request):
    candidates = (
        [os.environ.get('ISR_METADATA_CC', ''), 'gcc', 'arm-none-eabi-gcc', 'aarch64-elf-gcc']
        if request.param == 'gcc'
        else ['clang']
    )
    for candidate in candidates:
        command = shlex.split(candidate)
        if not command or not shutil.which(command[0]):
            continue
        version = subprocess.check_output(command + ['--version'], text=True).lower()
        if ('clang' in version) == (request.param == 'clang'):
            return [shutil.which(command[0]), *command[1:]]
    pytest.skip(f'A real {request.param} preprocessor is required')


def probe(tmp_path, compiler, flags, monkeypatch, *, family='xcc'):
    cmake = os.environ.get('CMAKE', shutil.which('cmake'))
    if cmake is None:
        pytest.skip('CMake is required')
    monkeypatch.setenv('ZEPHYR_XCC_HAS_BUILTIN_COUNTER', 'inherited-value')
    script = tmp_path / 'probe.cmake'
    result = tmp_path / 'capability.txt'
    quoted_flags = ' '.join(f'"{flag}"' for flag in [*compiler[1:], *flags])
    script.write_text(f'''
function(set_ifndef name value)
  if(NOT DEFINED ${{name}})
    set(${{name}} "${{value}}" PARENT_SCOPE)
  endif()
endfunction()
set(COMPILER {family})
set(CMAKE_C_COMPILER "{Path(compiler[0]).as_posix()}")
set(CMAKE_BINARY_DIR "{tmp_path.as_posix()}")
set(XTENSA_CORE_LOCAL_C_FLAG {quoted_flags})
include("{ROOT.as_posix()}/cmake/compiler/xcc/generic.cmake")
file(WRITE "{result.as_posix()}" "$ENV{{ZEPHYR_XCC_HAS_BUILTIN_COUNTER}}")
''')
    subprocess.run([cmake, '-P', str(script)], check=True, capture_output=True, text=True)
    return result.read_text()


@pytest.mark.parametrize(
    'flags,expected',
    [
        ([], 'y'),
        (['-U__COUNTER__'], 'n'),
        (['-U__COUNTER__', '-D__COUNTER__=__LINE__'], 'n'),
        (['-U__COUNTER__', '-D__COUNTER__=1'], 'n'),
    ],
)
def test_generic_probes_actual_counter_and_core_flags(
    tmp_path, compiler, monkeypatch, flags, expected
):
    assert probe(tmp_path, compiler, flags, monkeypatch) == expected


def test_shared_compiler_file_does_not_gate_xt_clang(tmp_path, compiler, monkeypatch):
    assert (
        probe(tmp_path, compiler, ['-U__COUNTER__'], monkeypatch, family='xt-clang')
        == 'inherited-value'
    )


@pytest.mark.parametrize(
    'variant,counter,xtensa,expected',
    [
        ('xcc', 'y', True, 'y'),
        ('xcc', 'n', True, 'n'),
        ('xcc', '', True, 'n'),
        ('xcc', 'y', False, 'n'),
        ('xt-clang', 'n', True, 'y'),
    ],
)
def test_actual_kconfig_requires_the_xcc_probe(
    tmp_path, monkeypatch, variant, counter, xtensa, expected
):
    monkeypatch.syspath_prepend(str(ROOT / 'scripts/kconfig'))
    kconfiglib = importlib.import_module('kconfiglib')
    monkeypatch.setenv('ZEPHYR_TOOLCHAIN_VARIANT', variant)
    monkeypatch.setenv('ZEPHYR_XCC_HAS_BUILTIN_COUNTER', counter)
    text = (ROOT / 'arch/Kconfig').read_text()
    block = text[
        text.index('config TOOLCHAIN_SUPPORTS_ISR_TABLES_LOCAL_DECLARATION\n') : text.index(
            'config DYNAMIC_INTERRUPTS\n'
        )
    ]
    profile = ROOT / 'cmake/toolchain' / variant
    capability = re.search(
        r'^config TOOLCHAIN_SUPPORTS_ISR_TABLES_LOCAL_DECLARATION\n[\s\S]*?(?=^if |\Z)',
        (profile / 'Kconfig').read_text(),
        re.M,
    ).group(0)
    path = tmp_path / 'Kconfig'
    path.write_text(
        'mainmenu "XCC ISR capability"\n'
        'config ARCH_HAS_ISR_TABLES_LOCAL_DECLARATION\n\tdef_bool y\n'
        'config GEN_ISR_TABLES\n\tdef_bool y\n'
        f'config XTENSA\n\tdef_bool {"y" if xtensa else "n"}\n'
        + (profile / 'Kconfig.defconfig').read_text()
        + '\n'
        + block
        + '\n'
        + capability
    )
    tree = kconfiglib.Kconfig(str(path), warn_to_stderr=False)
    assert tree.syms['ISR_TABLES_LOCAL_DECLARATION_SUPPORTED'].str_value == expected
    local = tree.syms['ISR_TABLES_LOCAL_DECLARATION']
    assert local.str_value == 'n'
    local.set_value('y')
    assert local.str_value == expected
    assert not tree.warnings
