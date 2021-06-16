# Copyright (c) 2021 The Linux Foundation
#
# SPDX-License-Identifier: Apache-2.0

import os
import uuid

from west.commands import WestCommand
from west import log

from zspdx.sbom import SBOMConfig, makeSPDX, setupCmakeQuery

SPDX_DESCRIPTION = """\
This command creates an SPDX 2.2 tag-value bill of materials
following the completion of a Zephyr build.

Prior to the build, an empty file must be created at
BUILDDIR/.cmake/api/v1/query/codemodel-v2 in order to enable
the CMake file-based API, which the SPDX command relies upon.
This can be done by calling `west spdx --init` prior to
calling `west build`."""

class ZephyrSpdx(WestCommand):
    def __init__(self):
        super().__init__(
                'spdx',
                'create SPDX bill of materials',
                SPDX_DESCRIPTION)

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(self.name,
                help=self.help,
                description = self.description)

        # If you update these options, make sure to keep the docs in
        # doc/guides/west/zephyr-cmds.rst up to date.
        parser.add_argument('-i', '--init', action="store_true",
                help="initialize CMake file-based API")
        parser.add_argument('-d', '--build-dir',
                help="build directory")
        parser.add_argument('-n', '--namespace-prefix',
                help="namespace prefix")
        parser.add_argument('-s', '--spdx-dir',
                help="SPDX output directory")
        parser.add_argument('--analyze-includes', action="store_true",
                help="also analyze included header files")
        parser.add_argument('--include-sdk', action="store_true",
                help="also generate SPDX document for SDK")

        return parser

    def do_run(self, args, unknown_args):
        log.dbg(f"running zephyr SPDX generator")

        log.dbg(f"  --init is", args.init)
        log.dbg(f"  --build-dir is", args.build_dir)
        log.dbg(f"  --namespace-prefix is", args.namespace_prefix)
        log.dbg(f"  --spdx-dir is", args.spdx_dir)
        log.dbg(f"  --analyze-includes is", args.analyze_includes)
        log.dbg(f"  --include-sdk is", args.include_sdk)

        if args.init:
            do_run_init(args)
        else:
            do_run_spdx(args)

def do_run_init(args):
    log.inf("initializing Cmake file-based API prior to build")

    if not args.build_dir:
        log.die("Build directory not specified; call `west spdx --init --build-dir=BUILD_DIR`")

    # initialize CMake file-based API - empty query file
    query_ready = setupCmakeQuery(args.build_dir)
    if query_ready:
        log.inf("initialized; run `west build` then run `west spdx`")
    else:
        log.err("Couldn't create Cmake file-based API query directory")
        log.err("You can manually create an empty file at $BUILDDIR/.cmake/api/v1/query/codemodel-v2")

def do_run_spdx(args):
    if not args.build_dir:
        log.die("Build directory not specified; call `west spdx --build-dir=BUILD_DIR`")

    # create the SPDX files
    cfg = SBOMConfig()
    cfg.buildDir = args.build_dir
    if args.namespace_prefix:
        cfg.namespacePrefix = args.namespace_prefix
    else:
        # create default namespace according to SPDX spec
        # note that this is intentionally _not_ an actual URL where
        # this document will be stored
        cfg.namespacePrefix = f"http://spdx.org/spdxdocs/zephyr-{str(uuid.uuid4())}"
    if args.spdx_dir:
        cfg.spdxDir = args.spdx_dir
    else:
        cfg.spdxDir = os.path.join(args.build_dir, "spdx")
    if args.analyze_includes:
        cfg.analyzeIncludes = True
    if args.include_sdk:
        cfg.includeSDK = True

    # make sure SPDX directory exists, or create it if it doesn't
    if os.path.exists(cfg.spdxDir):
        if not os.path.isdir(cfg.spdxDir):
            log.err(f'SPDX output directory {cfg.spdxDir} exists but is not a directory')
            return
        # directory exists, we're good
    else:
        # create the directory
        os.makedirs(cfg.spdxDir, exist_ok=False)

    makeSPDX(cfg)
