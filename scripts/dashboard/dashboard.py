# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0

'''
Process a build folder and generate a comprehensive HTML dashboard
including memory use, Kconfig values, devicetree browser, sys-init
ordering, and other general details.

This incorporates results from various other scripts:
- traceconfig
- footprint
- ram_plot/rom_plot
- initlevels
'''

import argparse
import glob
import html
import io
import json
import logging
import os
import pickle
import re
import shutil
import subprocess
import sys
import webbrowser
from datetime import datetime
from pathlib import Path

import jinja2
import yaml
from elftools.elf.constants import SH_FLAGS
from elftools.elf.elffile import ELFFile
from pygments import highlight
from pygments.formatters import HtmlFormatter  # pylint: disable=E0611
from pygments.lexers import DevicetreeLexer  # pylint: disable=E0611
from pygments.styles import get_style_by_name

sys.path.append(str(Path(__file__).parents[1] / "build"))
from check_init_priorities import Validator

sys.path.append(str(Path(__file__).parents[1] / "dts" / "python-devicetree" / "src" / "devicetree"))
from dtlib import DT

sys.path.append(str(Path(__file__).parents[1] / "footprint"))
try:
    from plot import generate_figure
    from plotly.offline.offline import get_plotlyjs
except ImportError:
    generate_figure = None

logger = logging.getLogger('dashboard')

# Online path to the kconfig browser.
KCONFIG_BROWSER = 'https://docs.zephyrproject.org/latest/kconfig.html'

# Set to match the CSS used in the HTML as it is set via CSS variable
# that we cannot access from Python script.
CSS_BS_TABLE_BG = '#f8f9fa'

# Constants for converting from bytes to human-friendly sizes.
MEMORY_UNIT_SIZES = {
    1073741824: 'GB',
    1048576: 'MB',
    1024: 'KB',
    1: 'Bytes',
}


def display_size(byte_cnt):
    '''
    Display a number of bytes in human-readable form with a size indicator.
    '''

    if byte_cnt in [None, '']:
        return ''

    compressed_bytes = byte_cnt
    unit = 'Bytes'
    for unit_size, unit_str in MEMORY_UNIT_SIZES.items():
        if abs(byte_cnt) >= unit_size:
            compressed_bytes = float(byte_cnt) / unit_size
            unit = unit_str
            break

    return f"{compressed_bytes:.1f} {unit}".replace('.0', '')


def elf_memory_summary(elf_file):
    '''
    Get a summary of memory use from the given ELF file.

    This function will parse the given elf file and determine the total memory
    used by read-only, read-write, and executable regions.

    :param elf_file: Path to an elf file.
    :return: A dictionary with keys "bss", "rwdata", "rodata", "text", "other"
    '''

    report = {'bss': 0, 'rodata': 0, 'rwdata': 0, 'text': 0, 'other': 0}

    with open(elf_file, "rb") as fd:
        elf = ELFFile(fd)

        for section in elf.iter_sections():
            flags = section.header['sh_flags']
            size = section.header['sh_size']
            sh_type = section.header['sh_type']

            if sh_type == 'SHT_NOBITS':
                report['bss'] += size
            elif sh_type == 'SHT_PROGBITS':
                if flags & SH_FLAGS.SHF_EXECINSTR:
                    report['text'] += size
                elif flags & SH_FLAGS.SHF_WRITE:
                    report['rwdata'] += size
                elif flags & SH_FLAGS.SHF_ALLOC:
                    report['rodata'] += size
                else:
                    report['other'] += size
            else:
                report['other'] += size

    return report


class ZephyrVersion:
    '''
    Class representing the version of Zephyr kernel. See
    https://docs.zephyrproject.org/latest/project/release_process.html
    for what the fields represent.
    Data is loaded from the VERSION file in the Zephyr base.
    '''

    FIELDS = ['version_major', 'version_minor', 'patchlevel', 'version_tweak', 'extraversion']

    def __init__(self, zephyr_base):
        self.version_major = None
        self.version_minor = None
        self.patchlevel = None
        self.version_tweak = None
        self.extraversion = None

        version_file = zephyr_base / 'VERSION'

        try:
            with open(version_file, encoding='utf-8') as f:
                version_data = f.readlines()
        except OSError as e:
            logger.error(f"Unable to read version information from {version_file}: {e}")
            return

        for version_line in version_data:
            field, value = version_line.split('=')
            field = field.strip().lower()
            value = value.strip()
            if field in ZephyrVersion.FIELDS:
                setattr(self, field, value)

    def __str__(self):
        version = f"{self.version_major}.{self.version_minor}"
        if self.patchlevel and self.patchlevel != '0':
            version += f'.{self.patchlevel}'
        if self.extraversion and self.extraversion != '0':
            version += f'-{self.extraversion}'
        if self.version_tweak and self.version_tweak != '0':
            version += f'+{self.version_tweak}'
        return version


class ZephyrToolchain:
    '''
    Container for the Zephyr toolchain information.
    Extracted from the CMakeCCompiler.cmake file.
    '''

    def __init__(self, build_path):
        self.cmake_c_compiler_id = None
        self.cmake_c_compiler_version = None

        cmake_glob = build_path / 'CMakeFiles' / '*' / 'CMakeCCompiler.cmake'
        cmake_file = glob.glob(str(cmake_glob))
        if cmake_file:
            cmake_file = cmake_file[0]
        else:
            return

        try:
            with open(cmake_file, encoding='utf-8') as f:
                lines = f.readlines()
        except OSError as e:
            logger.error(f"Unable to load CMakeCCompiler.cmake file {cmake_file}: {e}")
            return

        cmake_c_compiler_id_re = re.compile(r'\s*set\(CMAKE_C_COMPILER_ID\s+"(.*)"\)')
        cmake_c_compiler_version_re = re.compile(r'\s*set\(CMAKE_C_COMPILER_VERSION\s+"(.*)"\)')

        for line in lines:
            if match := cmake_c_compiler_id_re.match(line):
                self.cmake_c_compiler_id = match.group(1)
            if match := cmake_c_compiler_version_re.match(line):
                self.cmake_c_compiler_version = match.group(1)
            if self.cmake_c_compiler_id and self.cmake_c_compiler_version:
                break

    def __str__(self):
        return f"{self.cmake_c_compiler_id} {self.cmake_c_compiler_version}"


class KconfigSymbol:
    '''
    Class representing a KconfigSymbol and its value in a given build.
    '''

    def __init__(self, name, visible, sym_type, value, src, loc, build):
        self.sym_type = sym_type
        self.visible = bool(visible == 'y')
        self.name = name
        self.value = value
        if sym_type == "string" and self.value is not None:
            self.value = f'"{self.value}"'
        self.src = src
        self.loc = loc
        self.build = build

    def loc_html(self):
        '''
        Return an HTML representation of the symbol location.
        '''
        if self.loc and self.src in ['default', 'assign']:
            fn = os.path.relpath(self.loc[0], self.build.zephyr_base)
            disp_fn = os.path.normpath(fn).replace("../", "")
            sym_loc = f'{disp_fn}:{self.loc[1]}'
        elif self.loc and self.src in ['select', 'imply']:
            if len(self.loc) > 1:
                sym_loc = " || ".join(f"({html.escape(loc)})" for loc in self.loc)
            else:
                sym_loc = html.escape(self.loc[0])
            sym_loc = f"<code>{sym_loc}</code>"
        elif self.loc is None and self.src == "default":
            sym_loc = "<i>(implicit)</i>"
        else:
            sym_loc = ''
        return sym_loc

    def src_html(self):
        '''
        Return an HTML representation of the symbol source tag.
        '''
        if self.src == "unset":
            return ''
        if self.src == "default":
            return '<span class="badge bg-secondary">default</span>'
        if self.src == "assign":
            return '<span class="badge bg-primary">assigned</span>'
        if self.src == "select":
            return '<span class="badge bg-info">selected</span>'
        if self.src == "imply":
            return '<span class="badge bg-warning">implied</span>'
        return self.src


class ZephyrDashboard:
    '''
    Class for collecting and processing Zephyr build artifacts to create the
    HTML dashboard.
    '''

    def __init__(
        self,
        zephyr_base,
        build_path,
        output_path,
        kernel_bin_name="zephyr",
        skip_memory_report=False,
    ):
        self.zephyr_base = Path(zephyr_base)
        self.kernel_bin_name = kernel_bin_name
        self.build_path = Path(build_path) if build_path else self.zephyr_base / "build"
        self.output_path = Path(output_path) if output_path else self.build_path / "dashboard"

        self.elf_file = self.build_path / "zephyr" / f"{self.kernel_bin_name}.elf"
        self.dts_file = self.build_path / "zephyr" / "zephyr.dts"
        self.kconfig_file = self.build_path / "zephyr" / ".config"

        # Read details from the build_info.yml file.
        with open(self.build_path / "build_info.yml", encoding='utf-8') as fd:
            self.build_info = yaml.safe_load(fd)
        try:
            self.application = self.build_info['cmake']['application']['source-dir']
            base_prefix = '^' + str(self.zephyr_base.absolute()).replace('\\', '/') + '/'
            self.application = re.sub(base_prefix, '', self.application)
        except KeyError:
            self.application = None
        try:
            self.board = self.build_info['cmake']['board']['name']
        except KeyError:
            self.board = None
        try:
            self.command = self.build_info['west']['command']
            self.command = re.sub(r'\S*west', 'west', self.command)
        except KeyError:
            self.command = None
        try:
            self.topdir = Path(self.build_info['west']['topdir'])
        except KeyError:
            self.topdir = self.zephyr_base

        # Get some details on the build binary file - may not exist in all
        # builds (like QEMU).
        try:
            bin_file = self.build_path / "zephyr" / f"{self.kernel_bin_name}.bin"
            bin_stats = bin_file.stat()
            self.bin_size_str = display_size(bin_stats.st_size)
        except OSError:
            self.bin_size_str = None

        # Get some details on the build ELF file.
        elf_stats = self.elf_file.stat()
        self.elf_date_str = datetime.fromtimestamp(elf_stats.st_mtime).strftime("%Y-%m-%d %H:%M:%S")
        self.elf_size_str = display_size(elf_stats.st_size)

        self.toolchain = ZephyrToolchain(self.build_path)
        self.zephyr_version = ZephyrVersion(self.zephyr_base)

        self._init_kconfigs()
        self._init_sysinit()

        self.memory_summary = elf_memory_summary(self.elf_file)

        # Create the memory reports if they are stale.
        self.skip_memory_report = skip_memory_report
        if not self.skip_memory_report:
            memory_report = self.output_path / "all_report.json"
            if not os.path.isfile(memory_report) or os.path.getmtime(
                memory_report
            ) < os.path.getmtime(self.elf_file):
                self._create_memory_reports()

    def _init_sysinit(self):
        '''
        Get the sys-init levels and validate against DT dependencies.
        '''
        validator_log = io.StringIO()
        stream_handler = logging.StreamHandler(validator_log)
        stream_handler.setFormatter(logging.Formatter("%(levelname)s: %(message)s"))

        validator_logger = logging.getLogger('check_init_priorities')
        validator_logger.propagate = False
        validator_logger.addHandler(stream_handler)
        validator_logger.setLevel(logging.WARNING)

        with open(self.elf_file, 'rb') as fd:
            validator = Validator(self.elf_file, 'edt.pickle', validator_logger, fd)
            validator.check_edt()

        self.sys_init_errors = validator_log.getvalue().splitlines()
        self.sys_init_levels = validator.initlevels

    def _init_kconfigs(self):
        '''
        Load the kconfig trace information from the pickled data file and convert
        to a list of KconfigSymbol objects.
        '''
        pickle_path = self.build_path / "zephyr" / ".config-trace.pickle"
        with open(pickle_path, 'rb') as f:
            trace_data = pickle.load(f)
        self.kconfigs = [KconfigSymbol(*sym, build=self) for sym in trace_data]
        self.kconfigs.sort(key=lambda kc: kc.name)

    def _create_memory_reports(self):
        '''
        Use the footprint size_report tool to create the ram/rom/all JSON data.
        '''
        logger.info("Creating memory reports (may take a few minutes).")

        # Create the output location for the report files.
        self.output_path.mkdir(parents=True, exist_ok=True)

        cmd = [
            sys.executable,
            str(self.zephyr_base / "scripts" / "footprint" / "size_report"),
            '-k',
            str(self.elf_file),
            '-z',
            str(self.zephyr_base.absolute()),
            '--json',
            str(self.output_path / '{target}_report.json'),
            '--quiet',
            '--output',
            '.',
            'rom',
            'ram',
            'all',
        ]

        logger.debug(' '.join(cmd))
        try:
            subprocess.check_call(cmd)
        except subprocess.CalledProcessError as e:
            logger.error(
                f"Failed generating memory size report (exit code {e.returncode}). "
                + f"Command:\n{' '.join(e.cmd)}"
            )

    def _load_memory_report(self, mem_type):
        '''
        Load the memory report of the given type from an existing json file.

        :param type: Which report, "ram", "rom", or "all"
        :return: Dictionary of the loaded memory report.
        '''
        fname = self.output_path / f"{mem_type}_report.json"
        if not os.path.isfile(fname):
            logger.error(f'Memory report file "{fname}" not found.')
            return None

        with open(fname, encoding="utf-8") as fd:
            return json.load(fd)

    def _symbols_by_size(self, report, limit=None):
        '''
        Sort the symbols from a memory report by their size.

        :param report: A memory report as created by the Zephyr size_report script.
        :param limit: How many results to return.

        :return: A list of all the symbols with the largest first.
        '''

        if not report:
            return []

        symbol_list = []

        def flatten(symbols, lst):
            for symbol in symbols:
                children = symbol.get('children')
                if children:
                    flatten(children, lst)
                elif symbol['name'][0] != '(':
                    lst.append(symbol)

        flatten(report['symbols']['children'], symbol_list)

        symbol_list = sorted(symbol_list, key=lambda s: s['size'], reverse=True)

        if limit:
            symbol_list = symbol_list[0:limit]

        return symbol_list

    def _memory_report_template_context(self, mem_type, ctxt):
        '''
        Add the HTML template context for the memory report of the given type.
        '''

        report = self._load_memory_report(mem_type)

        def create_node(symbol, expanded=False):
            size = symbol['size']
            return (
                {
                    "expanded": expanded,
                    "data": {
                        "name": symbol['name'],
                        "size": size,
                        "displaySize": display_size(size),
                        "memoryType": [loc.upper() for loc in symbol.get('loc', [])],
                    },
                }
                if size
                else None
            )

        def add_children(node, children):
            if not children:
                return
            node['children'] = []
            for symbol in sorted(children, key=lambda c: c['name']):
                child_node = create_node(symbol)
                if not child_node:
                    continue
                node['children'].append(child_node)
                add_children(child_node, symbol.get('children'))

        def create_tree(report):
            tree = {}
            symbol = report.get('symbols')
            if symbol:
                tree['size'] = symbol['size']
                tree['tree'] = []
                node = create_node(symbol, expanded=True)
                tree['tree'].append(node)
                add_children(node, symbol.get('children'))
            return tree

        if report:
            tree = create_tree(report)
            ctxt[f'{mem_type}_report'] = json.dumps(tree)
            ctxt[f'{mem_type}_report_size'] = tree['size']

            if generate_figure:
                figure = generate_figure(report)
                figure.update_layout(paper_bgcolor=CSS_BS_TABLE_BG, height=600)
                ctxt[f'{mem_type}_plot'] = figure.to_html(full_html=False, include_plotlyjs=False)
            else:
                ctxt[f'{mem_type}_plot'] = None
        else:
            ctxt[f'{mem_type}_report'] = None
            ctxt[f'{mem_type}_plot'] = None

        if mem_type == 'all':
            ctxt['top_ten'] = self._symbols_by_size(report, 10)

    def _safe_relpath(self, filename):
        '''
        Returns a relative path to the file from the top dir, or
        the absolute path if a relative path cannot be established
        (on Windows with files in different drives, for example).
        '''
        try:
            return os.path.relpath(filename, start=self.topdir)
        except ValueError:
            return filename

    def _edt_fancytree(self):
        '''
        Create the Fancytree data object for the EDT tree.
        '''

        edt_pickle_path = self.build_path / "zephyr" / 'edt.pickle'
        with open(edt_pickle_path, "rb") as f:
            edt = pickle.load(f)

        # Load the same DTS file into a dtlib object so we can iterate over
        # all properties (not just ones that edtlib tracks).
        dt = DT(edt.dts_path, base_dir=self.topdir)

        label2path = {}

        def create_tree_node(dt_node, expanded=False):
            edt_node = edt.get_node(dt_node.path)

            return {
                "id": dt_node.path,
                "expanded": expanded,
                "edtNode": {
                    "isProperty": False,
                    "name": dt_node.name,
                    "labels": dt_node.labels,
                    "filename": self._safe_relpath(dt_node.filename),
                    "lineno": dt_node.lineno,
                    "description": edt_node.description if edt_node else "",
                },
            }

        def create_prop_tree_node(dt_prop, edt_prop):
            # Extract the original text value from the string conversion of the
            # property. No other way to get it from this object. Also remove
            # the trailing semi-colon and extra spaces around <>.
            value = re.sub('^.*= ', '', str(dt_prop))[:-1].replace('< ', '<').replace(' >', '>')

            # Find all the label references and create a look-up table to the node path.
            # This will be used on the render side to create links.
            for m in re.finditer(r'&(\w+)', value):
                try:
                    label2path[m.group(1)] = dt.label2node[m.group(1)].path
                except KeyError:
                    continue

            return {
                "id": dt_prop.node.path + f':{dt_prop.name}',
                "edtNode": {
                    "isProperty": True,
                    "name": dt_prop.name,
                    "value": value,
                    "filename": self._safe_relpath(dt_prop.filename),
                    "lineno": dt_prop.lineno,
                    "description": edt_prop.description if edt_prop else "",
                    "typeSpec": edt_prop.type if edt_prop else "",
                },
            }

        def add_children(tree_node, parent):
            tree_node['children'] = []

            edt_node = edt.get_node(parent.path)

            for name, dt_prop in parent.props.items():
                prop_node = create_prop_tree_node(dt_prop, edt_node.props.get(name))
                tree_node['children'].append(prop_node)

            for _, dt_node in parent.nodes.items():
                child_tree_node = create_tree_node(dt_node)
                tree_node['children'].append(child_tree_node)
                add_children(child_tree_node, dt_node)

        tree = {'tree': [], 'label2path': label2path}
        tree_node = create_tree_node(dt.root, expanded=True)
        tree['tree'].append(tree_node)
        add_children(tree_node, dt.root)

        return tree

    def create_html(self):
        '''
        Generate all the HTML output.
        '''

        templates_path = Path(__file__).parents[0] / "templates"
        template_loader = jinja2.FileSystemLoader(searchpath=str(templates_path))
        env = jinja2.Environment(
            loader=template_loader, autoescape=True, trim_blocks=True, lstrip_blocks=True
        )
        env.filters['display_size'] = display_size

        # Create the output directory tree.
        self.output_path.mkdir(parents=True, exist_ok=True)

        # Copy the static tree from the dashboard location as well as
        # doc/_static for the logo images.
        rpt_static = self.output_path / "static"
        doc_static = self.zephyr_base / "doc" / "_static"
        shutil.copytree(Path(__file__).parent.resolve() / 'static', rpt_static, dirs_exist_ok=True)
        img_dir = rpt_static / "img"
        img_dir.mkdir(exist_ok=True)
        shutil.copy2(doc_static / "images" / "logo.svg", rpt_static / "img")
        shutil.copy2(doc_static / "images" / "favicon.png", rpt_static / "img")
        shutil.copy2(doc_static / "css" / "light.css", rpt_static / "css")

        # Create the pygments CSS file.
        pygments_style = get_style_by_name('default')
        pygments_formatter = HtmlFormatter(style=pygments_style)
        pygments_css = pygments_formatter.get_style_defs('.highlight')
        with open(rpt_static / "css" / "pygments.css", 'w', encoding="utf-8") as fd:
            fd.write(pygments_css)

        # Create the plotly.js file if plotly was found.
        if generate_figure:
            with open(rpt_static / "js" / "plotly.js", 'w', encoding="utf-8") as fd:
                fd.write(get_plotlyjs())

        # ---------------------------------
        # HTML: index.html
        # ---------------------------------

        template = env.get_template("index.html")
        context = {'build': self, 'title': 'Build Summary', 'current': 'home'}
        fname = self.output_path / "index.html"
        logger.info(f"Creating {fname}")
        with open(fname, 'w', encoding="utf-8") as fd:
            fd.write(template.render(**context))

        # ---------------------------------
        # HTML: kconfig.html
        # ---------------------------------

        template = env.get_template("kconfig.html")
        context = {
            'build': self,
            'title': 'Kconfig',
            'current': 'kconfig',
            'kconfig_browser': KCONFIG_BROWSER,
        }
        fname = os.path.join(self.output_path, 'kconfig.html')
        logger.debug(f"Creating {fname}")
        with open(fname, 'w', encoding="utf-8") as fd:
            fd.write(template.render(**context))

        # ---------------------------------
        # HTML: sysinit.html
        # ---------------------------------

        template = env.get_template("sysinit.html")
        context = {'build': self, 'title': 'Sys Init', 'current': 'sysinit'}
        fname = os.path.join(self.output_path, 'sysinit.html')
        logger.debug(f"Creating {fname}")
        with open(fname, 'w', encoding="utf-8") as fd:
            fd.write(template.render(**context))

        # ---------------------------------
        # HTML: dts.html
        # ---------------------------------

        try:
            with open(self.dts_file, encoding="utf-8") as fd:
                dts_contents = fd.read()
        except OSError as e:
            logger.warning(f'Unable to load device tree file at {self.dts_file}: {e}')
            dts_contents = None

        if dts_contents:
            dts_contents = highlight(dts_contents, DevicetreeLexer(), pygments_formatter)

        edt_tree = self._edt_fancytree()

        template = env.get_template("dts.html")
        context = {
            'build': self,
            'title': 'Device Tree',
            'current': 'dts',
            'file_contents': dts_contents,
            'file_path': self.dts_file.absolute(),
            'edt_tree': json.dumps(edt_tree),
        }
        fname = self.output_path / "dts.html"
        logger.debug(f"Creating {fname}")
        with open(fname, 'w', encoding='utf-8') as fd:
            fd.write(template.render(**context))

        # ---------------------------------
        # HTML: memoryreport.html
        # ---------------------------------

        if not self.skip_memory_report:
            template = env.get_template("memoryreport.html")
            context = {
                'build': self,
                'title': 'Memory Report',
                'current': 'memory',
                'include_plotly_js': bool(generate_figure),
            }
            self._memory_report_template_context('all', context)
            self._memory_report_template_context('ram', context)
            self._memory_report_template_context('rom', context)
            fname = self.output_path / 'memoryreport.html'
            logger.debug(f"Creating {fname}")
            with open(fname, 'w', encoding="utf-8") as fd:
                fd.write(template.render(**context))

        # ---------------------------------
        # HTML: elfstats.html
        # ---------------------------------

        fname = self.build_path / "zephyr" / f"{self.kernel_bin_name}.stat"
        try:
            with open(fname, encoding="utf-8") as fd:
                file_contents = fd.read()
        except OSError as e:
            logger.warning(f'Unable to load zephyr elf stat file at {fname}: {e}')
            file_contents = ''

        template = env.get_template("textviewer.html")
        context = {
            'build': self,
            'title': 'ELF Stats',
            'current': 'elfstats',
            'file_contents': file_contents,
            'file_path': fname.absolute(),
        }
        fname = self.output_path / "elfstats.html"
        logger.debug(f"Creating {fname}")
        with open(fname, 'w', encoding='utf-8') as fd:
            fd.write(template.render(**context))

    def clean_up(self):
        '''
        Clean-up any temporary files.
        '''
        for mem_type in ['ram', 'rom', 'all']:
            fname = self.output_path / f"{mem_type}_report.json"
            fname.unlink(missing_ok=True)

    def open_browser(self):
        '''
        Open the default web browser to the index page of the dashboard.
        '''

        fname = self.output_path / "index.html"
        webbrowser.open(str(fname))


def parse_args():
    """
    Parse command line arguments.
    """

    parser = argparse.ArgumentParser(allow_abbrev=False)

    parser.add_argument("build", help="Build directory", nargs='?', default="build")
    parser.add_argument("--output", help="Output directory for index.html")
    parser.add_argument("--zephyr-base", default=".", help="Zephyr base directory")
    parser.add_argument("--kernel-bin-name", default="zephyr", help="Kernel binary name")
    parser.add_argument(
        "--skip-memory-report",
        action="store_true",
        help="Skip the memory report to generate faster",
    )
    parser.add_argument(
        "--no-clean", action="store_true", help="Do not remove temporary memory map json data"
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Print extra debugging information"
    )
    parser.add_argument("-q", "--quiet", action="store_true", help="Print nothing to the console.")
    parser.add_argument(
        "--open", action="store_true", help="Open the default web browser to the dashboard."
    )
    return parser.parse_args()


def main():
    '''
    Main function when invoked from the command-line.
    '''

    args = parse_args()

    if args.verbose:
        logging.basicConfig(level=logging.DEBUG)
        logger.setLevel(logging.DEBUG)
    elif not args.quiet:
        logging.basicConfig(level=logging.INFO)
        logger.setLevel(logging.INFO)

    output_path = args.output
    zephyr_base = args.zephyr_base
    build_path = args.build

    build = ZephyrDashboard(
        zephyr_base,
        build_path,
        output_path,
        kernel_bin_name=args.kernel_bin_name,
        skip_memory_report=args.skip_memory_report,
    )
    build.create_html()

    if not args.no_clean:
        build.clean_up()

    if args.open:
        build.open_browser()


def fix_pygments_devicetree_lexer():
    '''
    Fix the Pygments devicetree lexer to avoid a catastrophic backtracking pattern issue.

    Pygments (at least up to 2.19.2) devicetree lexer has a bug when parsing properties
    with a large number of spaces in them. The regex used results in a "catastropic
    backtracking pattern" and the time taken grows exponentially with the number of
    spaces. The cleanly indented dts that Zephyr creates hits this problem.

    *** Remove this code when the bug is fixed in Pygments. ***
    '''
    from pygments.token import Name

    ws = DevicetreeLexer._ws
    bad_expr = r'[a-zA-Z_][\w-]*(?=(?:\s*,\s*[a-zA-Z_][\w-]*|(?:' + ws + r'))*\s*[=;])'
    fixed_expr = r'[a-zA-Z_][\w-]*(?=(?:\s*,' + ws + r'[a-zA-Z_][\w-]*)*' + ws + r'[=;])'

    for idx, statement in enumerate(DevicetreeLexer.tokens['statements']):
        if statement[0] == bad_expr:
            logger.info("Fixing buggy pygments Devicetree lexer.")
            DevicetreeLexer.tokens['statements'][idx] = (fixed_expr, Name)
            break


if __name__ == "__main__":
    fix_pygments_devicetree_lexer()
    main()
