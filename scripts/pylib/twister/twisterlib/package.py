#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import json
import os
import tarfile

from twisterlib.statuses import TwisterStatus


class Artifacts:
    """Package the test artifacts into a tarball."""
    def __init__(self, env):
        self.options = env.options

    def make_tarfile(self, output_filename, source_dirs):
        """Create a tarball from the test artifacts."""
        root = os.path.basename(self.options.outdir)
        with tarfile.open(output_filename, "w:bz2") as tar:
            tar.add(self.options.outdir, recursive=False)
            for d in source_dirs:
                f = os.path.relpath(d, self.options.outdir)
                tar.add(d, arcname=os.path.join(root, f))

    def package(self):
        """Package the test artifacts into a tarball."""
        dirs = []
        with open(
            os.path.join(self.options.outdir, "twister.json"), encoding='utf-8'
        ) as json_test_plan:
            jtp = json.load(json_test_plan)
            for t in jtp['testsuites']:
                if t['status'] != TwisterStatus.FILTER:
                    p = t['platform']
                    normalized  = p.replace("/", "_")
                    dirs.append(
                        os.path.join(
                            self.options.outdir, normalized, t['toolchain'], t['name']
                        )
                    )

        dirs.extend(
            [
                os.path.join(self.options.outdir, "twister.json"),
                os.path.join(self.options.outdir, "testplan.json")
                ]
            )
        self.make_tarfile(self.options.package_artifacts, dirs)
