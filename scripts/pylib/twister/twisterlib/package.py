#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import tarfile
import json
import os

from twisterlib.statuses import TwisterStatus

class Artifacts:

    def __init__(self, env):
        self.options = env.options

    def make_tarfile(self, output_filename, source_dirs):
        root = os.path.basename(self.options.outdir)
        with tarfile.open(output_filename, "w:bz2") as tar:
            tar.add(self.options.outdir, recursive=False)
            for d in source_dirs:
                f = os.path.relpath(d, self.options.outdir)
                tar.add(d, arcname=os.path.join(root, f))

    def package(self):
        dirs = []
        with open(os.path.join(self.options.outdir, "twister.json"), "r") as json_test_plan:
            jtp = json.load(json_test_plan)
            for t in jtp['testsuites']:
                if t['status'] != TwisterStatus.FILTER:
                    p = t['platform']
                    normalized  = p.replace("/", "_")
                    dirs.append(os.path.join(self.options.outdir, normalized, t['name']))

        dirs.extend(
            [
                os.path.join(self.options.outdir, "twister.json"),
                os.path.join(self.options.outdir, "testplan.json")
                ]
            )
        self.make_tarfile(self.options.package_artifacts, dirs)
