# Copyright (c) 2018,2020 Intel Corporation
# Copyright (c) 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

import _compliance_context as _ctx
from yamllint import config, linter


class YAMLLint(_ctx.ComplianceTest):
    """
    YAMLLint
    """

    name = "YAMLLint"
    doc = "Check YAML files with YAMLLint."

    def run(self):
        config_file = _ctx.ZEPHYR_BASE / ".yamllint"

        for file in _ctx.get_files(filter="d"):
            if Path(file).suffix not in ['.yaml', '.yml']:
                continue

            yaml_config = config.YamlLintConfig(file=config_file)

            if file.startswith(".github/"):
                # Tweak few rules for workflow files.
                yaml_config.rules["line-length"] = False
                yaml_config.rules["truthy"]["allowed-values"].extend(['on', 'off'])
            elif file == ".codecov.yml":
                yaml_config.rules["truthy"]["allowed-values"].extend(['yes', 'no'])

            with open(file) as fp:
                for p in linter.run(fp, yaml_config):
                    self.fmtd_failure(
                        'warning', f'YAMLLint ({p.rule})', file, p.line, col=p.column, desc=p.desc
                    )
