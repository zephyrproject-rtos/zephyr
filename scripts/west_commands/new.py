# Copyright (c) 2025 Paulo Santos (pauloxrms@gmail.com).
#
# SPDX-License-Identifier: Apache-2.0

"""west "new" command"""

import argparse
import textwrap
from pathlib import Path
from west.commands import WestCommand

CMAKELISTS_TXT = "CMakeLists.txt"


def text_dedent(text: str) -> str:
    return textwrap.dedent(text.lstrip("\n"))


class New(WestCommand):
    def __init__(self):
        super().__init__(
            "new",
            # Keep this in sync with the string in west-commands.yml.
            "create new zephyr project, module or driver template",
            "Create a new project, module or driver template at a given path.",
            accepts_unknown_args=False,
        )

        self.fname: str | None = None
        self.rname: str | None = None
        self.kconfig_var_name: str | None = None

    def do_add_parser(self, parser_adder) -> argparse.ArgumentParser:
        parser = parser_adder.add_parser(self.name, help=self.help, description=self.description)

        parser.add_argument("name", help="name of the desired new object.")
        parser.add_argument("-m", "--module", help="create a module template.", action="store_true")
        parser.add_argument("-d", "--driver", help="create a driver template.", action="store_true")

        parser.add_argument("-p", "--path", help="creation parent directory.", default=Path.cwd())

        return parser

    def do_run(self, args: argparse.Namespace, unknown: list[str]):
        if args.module and args.driver:
            self.err("Options '--driver' and '--module' given simultaneously.")
            return

        path = Path(args.path).absolute()

        self.rname: str = args.name.lower().strip()
        self.fname = self.rname.replace("-", "_")
        self.kconfig_var_name = self.fname.upper()

        new_type = "project"
        if args.module:
            new_type = "module"
        elif args.driver:
            new_type = "driver"
        self.inf(f"Creating new '{self.rname}' {new_type} @ {path} ...")

        try:
            if args.module:
                self.create_new_module(path / self.fname)
            elif args.driver:
                self.create_new_driver(path)
            else:
                self.create_new_project(path)
        except FileExistsError:
            self.err(f"Could not create '{self.rname}', it already exists.")

    def create_new_module(self, module_dir: Path):
        src_dir = module_dir / "src"
        inc_dir = module_dir / "includes"
        zephyr_dir = module_dir / "zephyr"

        module_dir.mkdir()
        src_dir.mkdir()
        inc_dir.mkdir()
        zephyr_dir.mkdir()

        with open(module_dir / CMAKELISTS_TXT, "w") as file:
            file.write(
                text_dedent(f"""
                if(CONFIG_{self.kconfig_var_name})
                    zephyr_library()
                    zephyr_library_sources(src/{self.fname}.c)
                    zephyr_include_directories(includes)
                endif ()
                """)
            )

        with open(module_dir / "Kconfig", "w") as file:
            file.write(
                text_dedent(f"""
                menuconfig {self.kconfig_var_name}
                    bool "{self.name} module"

                if {self.kconfig_var_name}
                    # [...]
                endif
                """)
            )

        with open(src_dir / f"{self.fname}.c", "w") as file:
            file.write(
                text_dedent(f"""
                #include "{self.fname}.h"

                // [...]
                """)
            )

        with open(inc_dir / f"{self.fname}.h", "w") as file:
            file.write(
                text_dedent(f"""
                #ifndef {self.kconfig_var_name}_H
                #define {self.kconfig_var_name}_H

                // [...]

                #endif /* {self.kconfig_var_name}_H */
                """)
            )

        with open(zephyr_dir / "module.yml", "w") as file:
            file.write(
                text_dedent(f"""
                name: {self.name}
                build:
                  cmake: .
                  kconfig: Kconfig
                """)
            )

    def create_new_driver(self, project_dir: Path):
        drivers_dir = project_dir / "drivers"
        driver_dir = drivers_dir / self.fname
        bindings_dir = project_dir / "dts" / "bindings" / self.fname

        driver_dir.mkdir(parents=True)
        bindings_dir.mkdir(parents=True)

        with open(drivers_dir / CMAKELISTS_TXT, "a") as file:
            file.write(f"\nadd_subdirectory_ifdef(CONFIG_{self.kconfig_var_name} {self.fname})")

        with open(drivers_dir / "Kconfig", "a") as file:
            file.write(f'\nrsource "{self.fname}/Kconfig"')

        with open(driver_dir / CMAKELISTS_TXT, "w") as file:
            file.write(
                text_dedent(f"""
                zephyr_library()
                zephyr_library_sources_ifdef(CONFIG_{self.kconfig_var_name}_DRV {self.fname}_drv.c)
                """)
            )

        with open(driver_dir / "Kconfig", "w") as file:
            file.write(
                text_dedent(f"""
                menuconfig {self.kconfig_var_name}
                    bool "{self.rname.capitalize()} device drivers"
                    help
                      This option enables the {self.rname} drivers class.

                if {self.kconfig_var_name}

                config {self.kconfig_var_name}_INIT_PRIORITY
                    int "{self.rname.capitalize()} device drivers init priority"
                    default KERNEL_INIT_PRIORITY_DEVICE
                    help
                      {self.rname.capitalize()} device drivers init priority.

                module = {self.kconfig_var_name}
                module-str = {self.rname}

                rsource "Kconfig.{self.fname}_drv"

                endif # {self.kconfig_var_name}
                """)
            )

        with open(driver_dir / f"Kconfig.{self.fname}_drv", "w") as file:
            file.write(
                text_dedent(f"""
                config {self.kconfig_var_name}_DRV
                    bool "{self.rname.capitalize()} driver"
                    default y
                    depends on DT_HAS_{self.kconfig_var_name}_DRV_ENABLED
                """)
            )

        with open(driver_dir / f"{self.fname}_drv.c", "w") as file:
            file.write(
                text_dedent(f"""
                #define DT_DRV_COMPAT {self.fname}_drv

                #include <zephyr/device.h>

                #include <zephyr/devicetree.h>
                #include <zephyr/kernel.h>

                struct {self.fname}_drv_data {{
                    // [...]
                }};

                struct {self.fname}_drv_config {{
                    // [...]
                }};

                static DEVICE_API({self.fname}, {self.fname}_drv_api) = {{
                    // [...]
                }};

                static int {self.fname}_drv_init(const struct device *dev)
                {{
                    const struct {self.fname}_drv_config *config = dev->config;
                    struct {self.fname}_drv_data *data = dev->data;

                    // [...]

                    return 0;
                }}

                #define {self.kconfig_var_name}_DRV_DEFINE(inst)                            \\
                    static struct {self.fname}_drv_data data##inst;                         \\
                    static const struct {self.fname}_drv_config config##inst = {{           \\
                    /* [...] */                                                             \\
                    }};                                                                     \\
                                                                                            \\
                    DEVICE_DT_INST_DEFINE(inst, {self.fname}_drv_init, NULL, &data##inst,   \\
                                &config##inst, POST_KERNEL,                                 \\
                                CONFIG_{self.kconfig_var_name}_INIT_PRIORITY,               \\
                                &{self.fname}_drv_api);

                DT_INST_FOREACH_STATUS_OKAY({self.kconfig_var_name}_DRV_DEFINE)
                """)
            )

        with open(bindings_dir / f"{self.fname}_drv.yml", "w") as file:
            file.write(
                text_dedent(f"""
                description: |
                  A {self.rname}-drv driver...

                compatible: "{self.rname}-drv"

                include: base.yaml

                properties:
                  # [...]
                """)
            )

    def create_new_project(self, prj_root_dir: Path):
        project_dir = prj_root_dir / self.fname
        src_dir = project_dir / "src"

        src_dir.mkdir(parents=True)

        with open(project_dir / CMAKELISTS_TXT, "w") as file:
            file.write(
                text_dedent(f"""
                cmake_minimum_required(VERSION 3.20.0)

                find_package(Zephyr REQUIRED HINTS $ENV{{ZEPHYR_BASE}})
                project({self.fname})

                target_sources(app PRIVATE src/main.c)
                """)
            )

        with open(project_dir / "prj.conf", "w") as file:
            file.write("\n")

        with open(src_dir / "main.c", "w") as file:
            file.write(
                text_dedent("""
                #include <zephyr/kernel.h>

                int main(void)
                {{
                    printk("Hello World from west new!\\n");

                    return 0;
                }}
                """)
            )
