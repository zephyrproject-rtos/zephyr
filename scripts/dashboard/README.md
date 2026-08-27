# Zephyr Build Dashboard

`dashboard.py` processes a Zephyr build directory and generates a self-contained,
multi-page HTML dashboard that consolidates build information, memory usage, Kconfig
configuration, device tree contents, and system initialization ordering into a single
browsable report.

## Dashboard Pages

| Page | Description |
|------|-------------|
| **Build Summary** | Board name, application path, west command, Zephyr version, toolchain, ELF/binary file sizes, and a high-level memory breakdown (text, rodata, rwdata, bss) |
| **Memory Report** | Hierarchical tree view of RAM, ROM, and combined memory usage by symbol, with optional Plotly sunburst charts and a top-10 largest symbols table |
| **Kconfig** | Searchable table of all Kconfig symbols showing their value, type, source (default / assigned / selected / implied), and the file/line where the value originates |
| **Sys Init** | System initialization levels extracted from the ELF file, with dependency validation against the device tree; any ordering errors are flagged with a warning icon |
| **Device Tree** | Interactive Fancytree browser of the compiled device tree (with node descriptions from bindings) alongside a Pygments-highlighted view of the raw `.dts` source |
| **ELF Stats** | Raw text output of `zephyr.stat` (the `objdump`-style section listing) |

## How It Works

1. **Build artifact discovery** — The script reads `build_info.yml` from the build
   directory to determine the board name, application path, and west command used.

2. **ELF parsing** — `zephyr.elf` is parsed with `pyelftools` to produce a high-level
   memory summary (text, rodata, rwdata, bss, other).

3. **Kconfig trace** — `.config-trace.pickle` (produced by the `traceconfig` CMake
   module) is loaded and converted into a list of `KconfigSymbol` objects that carry
   the symbol name, value, type, and the source that set the value.

4. **Device tree** — `edt.pickle` is loaded to get the extended device tree (EDT)
   with binding descriptions. The same `.dts` file is also parsed directly with
   `dtlib` so that all raw properties are available for display.

5. **Sys-init validation** — The `check_init_priorities` validator (from
   `scripts/build/`) reads the ELF symbol table and checks that device
   initialization order is consistent with device tree dependencies. It also
   extracts the ordered list of initialization functions for display.

6. **Memory reports** — If the output directory does not already contain up-to-date
   `ram_report.json`, `rom_report.json`, and `all_report.json` files, the script
   invokes `scripts/footprint/size_report` to generate them. This step can be
   skipped with `--skip-memory-report` for faster iteration.

7. **HTML rendering** — Jinja2 templates in the `templates/` directory are rendered
   with the collected data and written to the output directory. Static assets
   (Bootstrap, custom CSS/JS, icon font) are copied from `static/` into the output.

## Directory Structure

```
scripts/dashboard/
├── dashboard.py        # Main script
├── icons.json          # Fontello configuration for the custom icon font
├── static/...          # Static assets copied into every generated dashboard
└── templates/...       # Jinja2 HTML templates
```

## Usage

```
python scripts/dashboard/dashboard.py [<build_dir>] [options]
```

| Argument | Description |
|----------|-------------|
| `build_dir` | Path to the Zephyr build directory (default: `build/`) |
| `--output <dir>` | Output directory for the generated HTML (default: `<build>/dashboard`) |
| `--zephyr-base <dir>` | Zephyr base directory (default: current directory) |
| `--skip-memory-report` | Skip the memory report generation step for faster output |
| `-v`, `--verbose` | Print extra debugging information |
| `-q`, `--quiet` | Suppress all console output |
| `--open` | Open the generated dashboard in the default web browser when done |

**Example:**

```bash
python scripts/dashboard/dashboard.py build --zephyr-base . --open
```

## Python Dependencies

All dependencies are available as part of the base Zephyr SDK, the below is
for reference only.

| Package | Purpose |
|---------|---------|
| `jinja2` | HTML template rendering |
| `pyelftools` | ELF file parsing |
| `pygments` | Device tree source syntax highlighting |
| `pyyaml` | Reading `build_info.yml` |
| `plotly` *(optional)* | Sunburst memory charts; omit for text-only memory report |

Install required packages:

```bash
pip install jinja2 pyelftools pygments pyyaml
# Optional, for memory charts:
pip install plotly
```

## Bootstrap Integration

[Bootstrap](https://getbootstrap.com/) is used as the front-end toolkit. To keep
the dashboard standalone and usable offline but at a reasonable size, the
bootstrap JavaScript and CSS files have been reduced to just what we need.
``bootstrap-chop.js`` has all unused Javascript components removed. To add back
any components for new work, merge what is required from ``bootstrap.js`` in
the main distribution.

### Bootstrap CSS

Generating the ``bootstrap-chop.css`` is a little more complicated as it requires
analyzing the built HTML and JavaScript to find out what is used. This
is done using [PurgeCSS](https://purgecss.com/). To update
``bootstrap-chop.css``, use the following steps (after installing PurgeCSS):

1. **Get the full `bootstrap.css` file** - replace the
   ``static/css/bootstrap-chop.css`` in the generated dashboard with the full
   Bootstrap ``bootstrap.css`` file from the main distribution.

2. **Run `purgecss`** - go to the dashboard build directory (containing
   ``index.html``) and run the following command:

   ```
   purgecss --css static/css/bootstrap.css --content *.html static/js/*.js --variables --output .
   ```

   This will generate a new reduced ``bootstrap.css`` file in the current directory.

3. **Update `bootstrap-chop.css`** - copy the newly created
   file into ``static/css/bootstrap-chop.css``.

## Adding New Icons

The dashboard uses a size optimized version of the [Bootstrap icons](https://icons.getbootstrap.com/)
built with [fontello.com](https://fontello.com). Only the icons used in the
dashboard are included, and it is entirely self-hosted to work offline.
Icons are referenced in templates and CSS using Bootstrap Icons class names (e.g.,
`<i class="bi bi-folder"></i>`).

The `icons.json` file at the root of this directory is the fontello project
configuration. It contains the SVG path data and CSS class name for every icon
currently in the font.

### Steps to add a new icon

1. **Open fontello.com** in your browser.

2. **Import the existing configuration** - drag-and-drop the
  `scripts/dashboard/icons.json` file onto the page, or click the
  wrench/settings icon at the top of the fontello page and choose
  *"Import"*, then select `scripts/dashboard/icons.json`.
  All current icons will be loaded and shown as selected.

3. **Find and select the new icon(s)** - Download the
   [Bootstrap icon set](https://github.com/twbs/icons/releases/latest/), which
   includes svg versions of each icon. Drag-and-drop the new icon svg file
   onto the page which will add it to the project.

4. **Download the font bundle** - click the red *"Download webfont"* button. A zip
   archive named `fontello-<hash>.zip` will be downloaded.

5. **Extract the archive** and copy the updated files into the repository:

   - `font/icons.woff2` → `scripts/dashboard/static/font/icons.woff2`
   - `css/icons.css` → `scripts/dashboard/static/css/icons.css`

6. **Update `icons.json`** - merge the new icon svg entries in the
   `config.json` file from the extracted archive into the `scripts/dashboard/icons.json`
   file. This keeps the fontello project file in sync so future contributors
   can re-import it.


## License

Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

SPDX-License-Identifier: Apache-2.0
