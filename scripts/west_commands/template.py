# Copyright (c) 2025
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import textwrap
from pathlib import Path

from copier import Worker
from west.commands import WestCommand


class Template(WestCommand):
    """Use built-in templates to create new samples, drivers, tests, etc.

    Initial implementation supports creating a new sample using the Copier
    template engine. Additional template types can be added over time.
    """

    def __init__(self):
        super().__init__(
            "template",
            "use built-in templates to create new samples, drivers, tests, etc.",
            "Create Zephyr content from templates (initially: samples)",
            accepts_unknown_args=False,
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=self.description,
            epilog=textwrap.dedent("""
            Examples:

              - Create a new sample and answer prompts interactively:
                    west template --type sample
            """),
        )

        parser.add_argument(
            "-t",
            "--type",
            choices=["sample"],
            default="sample",
            help="Type of template to instantiate (default: sample)",
        )

        # parser.add_argument(
        #     "-o",
        #     "--output",
        #     dest="output",
        #     metavar="DIR",
        #     type=Path,
        #     help=(
        #         "Destination root directory. For samples, this should typically be the "
        #         "'samples' directory. Defaults to <workspace>/zephyr/samples."
        #     ),
        # )

        return parser

    def do_run(self, args, _):
        # Determine template kind (only 'sample' for now)
        template_kind = args.type

        # Resolve workspace and zephyr repository paths
        workspace_topdir = Path(self.topdir)
        zephyr_repo_dir = workspace_topdir / "zephyr"

        if not zephyr_repo_dir.is_dir():
            self.die(f"Could not locate Zephyr repository at {zephyr_repo_dir}")

        templates_root = Path(__file__).parent / "templates"
        if template_kind == "sample":
            copier_template_dir = templates_root / "sample"
            default_output_dir = zephyr_repo_dir / "samples"
        else:
            self.die(f"Unsupported template type: {template_kind}")
            return

        if not copier_template_dir.is_dir():
            self.die(f"Built-in template not found. Expected at: {copier_template_dir}")

        output_dir = default_output_dir
        output_dir.mkdir(parents=True, exist_ok=True)

        with Worker(
            src_path=str(copier_template_dir),
            dst_path=str(output_dir),
            unsafe=True,
            quiet=True,
        ) as worker:
            worker.run_copy()
