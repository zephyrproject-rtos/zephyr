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

    @staticmethod
    def _safe_artifact_relpath(*parts):
        candidate = os.path.normpath(os.path.join(*parts))
        if os.path.isabs(candidate) or candidate.startswith(os.pardir):
            return None
        return candidate

    @staticmethod
    def _get_safe_artifact_dir(build_root, candidate):
        artifact_dir = os.path.abspath(os.path.join(build_root, candidate))
        if os.path.commonpath([build_root, artifact_dir]) != build_root:
            return None
        return artifact_dir

    def get_testsuite_artifact_dir(self, testsuite):
        """Return the existing build artifact directory for a testsuite."""
        normalized = testsuite['platform'].replace("/", "_")
        toolchain_normalized = testsuite['toolchain'].replace("/", "_")
        build_root = os.path.abspath(
            os.path.join(self.options.outdir, normalized, toolchain_normalized)
        )

        if self.options.detailed_test_id:
            candidate = self._safe_artifact_relpath(testsuite['name'])
            if candidate is None:
                raise ValueError(
                    "Invalid testsuite artifact name "
                    f"'{testsuite['name']}' for build root '{build_root}'"
                )
        else:
            testsuite_path = testsuite.get('path')
            if testsuite_path is None:
                raise ValueError(
                    f"Missing testsuite artifact path for '{testsuite['name']}' in '{build_root}'"
                )

            candidate = self._safe_artifact_relpath(testsuite_path, testsuite['name'])
            if candidate is None:
                raise ValueError(
                    "Invalid testsuite artifact path "
                    f"'{testsuite_path}' or name '{testsuite['name']}' "
                    f"for build root '{build_root}'"
                )

        artifact_dir = self._get_safe_artifact_dir(build_root, candidate)
        if artifact_dir is not None and os.path.isdir(artifact_dir):
            return artifact_dir

        raise ValueError(
            "Could not find testsuite artifact directory "
            f"for '{testsuite['name']}' in '{build_root}'"
        )

    def package(self):
        """Package the test artifacts into a tarball."""
        dirs = []
        with open(
            os.path.join(self.options.outdir, "twister.json"), encoding='utf-8'
        ) as json_test_plan:
            jtp = json.load(json_test_plan)
            for t in jtp['testsuites']:
                if t['status'] != TwisterStatus.FILTER:
                    dirs.append(self.get_testsuite_artifact_dir(t))

        dirs.extend(
            [
                os.path.join(self.options.outdir, "twister.json"),
                os.path.join(self.options.outdir, "testplan.json"),
            ]
        )
        self.make_tarfile(self.options.package_artifacts, dirs)
