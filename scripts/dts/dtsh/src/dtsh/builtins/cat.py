# Copyright (c) 2024 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree shell built-in "cat".

Concatenate and output info about a node and its properties.
Think of something like using cat to retrieve information
from the /proc filesystem on Linux.

Unit tests and examples: tests/test_dtsh_builtin_cat.py
"""

from typing import Sequence, List, Optional

from enum import Enum

from dtsh.utils import YAMLFile
from dtsh.model import DTNode, DTNodeProperty, DTModel, DTBinding
from dtsh.modelutils import DTSUtil
from dtsh.io import DTShOutput
from dtsh.shell import DTSh, DTShCommand, DTShFlag, DTShCommandError
from dtsh.shellutils import DTShFlagPager, DTShParamDTPathX
from dtsh.config import DTShConfig

from dtsh.rich.shellutils import DTShFlagLongList
from dtsh.rich.text import TextUtil
from dtsh.rich.theme import DTShTheme
from dtsh.rich.tui import HeadingsContentWriter, View
from dtsh.rich.modelview import (
    ViewPropertyValueTable,
    FormPropertySpec,
    ViewNodeBinding,
    ViewDescription,
    ViewYAMLFile,
)


_dtshconf: DTShConfig = DTShConfig.getinstance()


class CatFlagExpandIncludes(DTShFlag):
    """Whether to expand the contents of included YAML files.

    Requires '-A' or '-Y and '-l'.
    """

    BRIEF = "expand contents of included YAML files"
    LONGNAME = "expand-included"


class CatFlagAllOptions(DTShFlag):
    """Shortcut for '-DBY'.

    Implies rich outut ('-l').
    Not allowed when executing the command for multiple properties.
    """

    BRIEF = "print full summary of node or property"
    SHORTNAME = "A"


class CatOptionDescription(DTShFlag):
    """Print node or property full description from binding."""

    BRIEF = "print node or property description"
    SHORTNAME = "D"


class CatOptionBindings(DTShFlag):
    """Print node binding or property specification."""

    BRIEF = "print node binding or property specification"
    SHORTNAME = "B"


class CatOptionYamlFiles(DTShFlag):
    """Show YAML content of binding or specification files."""

    BRIEF = "show bindings or specifications as YAML"
    SHORTNAME = "Y"


class CatMode(Enum):
    """Command mode."""

    # The command is invoked to dump property values.
    PVALUES = 1

    # The command is invoked in summary mode (-DBYA).
    SUMMARY = 2


class DTShBuiltinCat(DTShCommand):
    """Devicetree shell built-in "cat".

    Concatenate and output info about a node and its properties.
    Think of something like using cat to retrieve information
    from the /proc filesystem on Linux.

    If the user does not set options to explicitly select what to cat,
    the command will print all property values of the node parameter
    or of the current working node.

    Option flags:
    '-D': print node or property description
    '-B': print node binding or property specification
    '-Y': print device binding file or property specification file (YAML)

    Globbing allowed only to print property values.
    Note that "cat $*" is this equivalent to "cat" without
    any option or argument.

    More than one option from -DBY (or -A) allowed only with rich output (-l).

    When -Y is set, --fold-yaml folds YAML files.
    """

    _mode: CatMode = CatMode.PVALUES

    def __init__(self) -> None:
        super().__init__(
            "cat",
            "concat and output info about a node and its properties",
            [
                CatOptionDescription(),
                CatOptionYamlFiles(),
                CatOptionBindings(),
                CatFlagAllOptions(),
                CatFlagExpandIncludes(),
                DTShFlagLongList(),
                DTShFlagPager(),
            ],
            DTShParamDTPathX(),
        )

    @property
    def option_description(self) -> bool:
        return self.with_flag(CatOptionDescription)

    @property
    def option_bindings(self) -> bool:
        return self.with_flag(CatOptionBindings)

    @property
    def option_yaml(self) -> bool:
        return self.with_flag(CatOptionYamlFiles)

    @property
    def flag_show_all(self) -> bool:
        return self.with_flag(CatFlagAllOptions)

    @property
    def flag_longfmt(self) -> bool:
        return self.with_flag(DTShFlagLongList)

    @property
    def flag_expand_includes(self) -> bool:
        return self.with_flag(CatFlagExpandIncludes)

    @property
    def param_xpath(self) -> DTShParamDTPathX:
        return self.with_param(DTShParamDTPathX)

    def with_longfmt(self) -> bool:
        # "-A" implies "-l"
        return self.flag_longfmt or self.flag_show_all

    def parse_argv(self, argv: Sequence[str]) -> None:
        """Overrides DTShCommand.parse_argv()."""
        super().parse_argv(argv)

        # Folding requires some YAML to fold ('-Y' or '-A').
        if self.flag_expand_includes:
            if not (self.flag_show_all or self.option_yaml):
                raise DTShCommandError(
                    self,
                    "option '--fold-yaml' requires '-Y' or '-A",
                )

        # Cat output options parsed from the command line.
        cnt_options: int = sum(
            [self.option_description, self.option_bindings, self.option_yaml]
        )

        if cnt_options or self.flag_show_all:
            # Preconditions for summary.
            self._mode = CatMode.SUMMARY

            if self.param_xpath.is_globexpr():
                raise DTShCommandError(
                    self,
                    "globbing properties, options '-DBYA' not allowed",
                )

            if cnt_options:
                if self.flag_show_all:
                    raise DTShCommandError(
                        self, "options '-DBY' not allowed with '-A'"
                    )
                if (cnt_options > 1) and not self.flag_longfmt:
                    raise DTShCommandError(
                        self, "more than one option from '-DBY' requires '-l'"
                    )
        else:
            self._mode = CatMode.PVALUES

    def execute(self, argv: Sequence[str], sh: DTSh, out: DTShOutput) -> None:
        """Overrides DTShCommand.execute()."""
        super().execute(argv, sh, out)

        param_node: DTNode
        param_props: Optional[List[DTNodeProperty]]
        # Command will fail here if the xpath parameter is invalid
        # (node or property not found).
        param_node, param_props = self.param_xpath.xsplit(self, sh)

        # Setup pager after we could fail.
        if self.with_flag(DTShFlagPager):
            out.pager_enter()

        if self._mode == CatMode.PVALUES:
            if param_props is not None:
                self._cat_pvalues(param_props, param_node, out)
        else:
            self._cat_summary(param_node, param_props, out)

        if self.with_flag(DTShFlagPager):
            out.pager_exit()

    def _cat_pvalues(
        self,
        param_props: List[DTNodeProperty],
        param_node: DTNode,
        out: DTShOutput,
    ) -> None:
        props: List[DTNodeProperty] = param_props
        if not props and not self.param_xpath.is_globexpr():
            # If the user hasn't explicitly selected one or more properties,
            # dump all node property values.
            props = param_node.all_dtproperties()
        if self.with_longfmt():
            self._cat_pvalues_rich(props, param_node, out)
        else:
            self._cat_pvalues_ascii(props, out)

    def _cat_pvalues_ascii(
        self, props: List[DTNodeProperty], out: DTShOutput
    ) -> None:
        for prop in props:
            strval = DTSUtil.mk_property_value(prop)
            out.write(f"{prop.name}: {strval}")

    def _cat_pvalues_rich(
        self, props: List[DTNodeProperty], node: DTNode, out: DTShOutput
    ) -> None:
        view: View.PrintableType
        if props:
            view = ViewPropertyValueTable(props, node.dt)
            view.left_indent(1)
        elif self.param_xpath.is_globexpr():
            view = TextUtil.mk_apologies("No matching properties.")
        else:
            view = TextUtil.mk_apologies("This node does not set any property.")
        out.write(view)

    def _cat_summary(
        self,
        param_node: DTNode,
        param_props: Optional[List[DTNodeProperty]],
        out: DTShOutput,
    ) -> None:
        if param_props:
            prop: DTNodeProperty = param_props[0]
            self._cat_summary_of_property(prop, param_node.dt, out)
        else:
            self._cat_summary_of_node(param_node, out)

    def _cat_summary_of_property(
        self,
        prop: DTNodeProperty,
        dt: DTModel,
        out: DTShOutput,
    ) -> None:
        if self.with_longfmt():
            self._cat_summary_of_property_rich(prop, dt, out)
        else:
            self._cat_summary_of_property_ascii(prop, dt, out)

    def _cat_summary_of_property_rich(
        self,
        prop: DTNodeProperty,
        dt: DTModel,
        out: DTShOutput,
    ) -> None:
        sections: List[HeadingsContentWriter.Section] = []

        if self.flag_show_all or self.option_description:
            sections.append(
                HeadingsContentWriter.Section(
                    "description", ViewDescription(prop.description)
                )
            )

        if self.flag_show_all or self.option_bindings:
            sections.append(
                HeadingsContentWriter.Section(
                    "specification", FormPropertySpec(prop.dtspec, dt)
                )
            )

        if self.flag_show_all or self.option_yaml:
            content: HeadingsContentWriter.ContentType
            fyaml: Optional[YAMLFile] = dt.find_property(prop.dtspec)
            if fyaml:
                path: str = str(fyaml.path)
                if path == prop.path:
                    style = DTShTheme.STYLE_YAML_BINDING
                else:
                    style = DTShTheme.STYLE_YAML_INCLUDE
                content = ViewYAMLFile.create(
                    path,
                    dt.dts.yamlfs,
                    style=style,
                    compact=not self.flag_expand_includes,
                    linktype=_dtshconf.pref_yaml_actionable_type,
                )
            else:
                content = TextUtil.mk_apologies(
                    "Specification file unavailable."
                )
            sections.append(HeadingsContentWriter.Section("YAML", content))

        self._write_rich_sections(sections, out)

    def _cat_summary_of_property_ascii(
        self,
        prop: DTNodeProperty,
        dt: DTModel,
        out: DTShOutput,
    ) -> None:
        # ASCII mode: only one of '-DBY'.
        if self.option_description:
            if prop.description:
                out.write(prop.description)

        elif self.option_bindings:
            parts: List[str] = [prop.dtspec.dttype]
            if prop.dtspec.required:
                parts.append("(required)")
            if prop.dtspec.default is not None:
                strdefault = DTSUtil.mk_value(prop.dtspec.default)
                parts.append(f"default:{strdefault}")
            out.write(" ".join(parts))

        elif self.option_yaml:
            fyaml: Optional[YAMLFile] = dt.find_property(prop.dtspec)
            if fyaml:
                path: str = str(fyaml.path)
                if self.flag_expand_includes:
                    out.write(path)
                else:
                    out.write(fyaml.content)

    def _cat_summary_of_node(
        self,
        node: DTNode,
        out: DTShOutput,
    ) -> None:
        if self.with_longfmt():
            self._cat_summary_of_node_rich(node, out)
        else:
            self._cat_summary_of_node_ascii(node, out)

    def _cat_summary_of_node_ascii(
        self,
        node: DTNode,
        out: DTShOutput,
    ) -> None:
        if self.option_description:
            if node.description:
                out.write(node.description)

        elif self.option_bindings:
            if node.binding:
                binding: DTBinding = node.binding
                parts: List[str] = []
                if binding.compatible:
                    parts.append(binding.compatible)
                if binding.buses:
                    parts.append(",".join(binding.buses))
                if binding.on_bus:
                    parts.append(f"on {binding.on_bus}")

                if binding.cb_depth:
                    cblevel: str
                    if binding.cb_depth == 1:
                        cblevel = "child-binding"
                    elif binding.cb_depth == 2:
                        cblevel = "grandchild-binding"
                    else:
                        cblevel = "great-grandchild-binding"

                    ancestor: Optional[
                        DTBinding
                    ] = node.get_child_binding_ancestor()
                    if ancestor:
                        cbof: str
                        if ancestor.compatible:
                            cbof = f"of {ancestor.compatible}"
                        else:
                            cbof = f"in {ancestor.fyaml.path.name}"

                        parts.append(f"{cblevel} {cbof}")

                    else:
                        # The child-binding itself should have a compatible.
                        parts.append(cblevel)

                out.write(f" {_dtshconf.wchar_dash} ".join(parts))

        elif self.option_yaml:
            if node.binding_path:
                fyaml = YAMLFile(node.binding_path)
                out.write(fyaml.content)

    def _cat_summary_of_node_rich(
        self,
        node: DTNode,
        out: DTShOutput,
    ) -> None:
        sections: List[HeadingsContentWriter.Section] = []

        if self.flag_show_all or self.option_description:
            sections.append(
                HeadingsContentWriter.Section(
                    "description", ViewDescription(node.description)
                )
            )

        content: HeadingsContentWriter.ContentType

        if self.flag_show_all:
            # Show property values only when '-A' is set.
            props: List[DTNodeProperty] = node.all_dtproperties()
            if props:
                content = ViewPropertyValueTable(props, node.dt)
            else:
                content = TextUtil.mk_apologies(
                    "This node does not set any property."
                )
            sections.append(
                HeadingsContentWriter.Section("Properties", content)
            )

        if self.flag_show_all or self.option_bindings:
            if node.binding:
                content = ViewNodeBinding(node)
            else:
                content = TextUtil.mk_apologies("This node has no binding.")
            sections.append(HeadingsContentWriter.Section("Bindings", content))

        if self.flag_show_all or self.option_yaml:
            if node.binding_path:
                content = ViewYAMLFile.create(
                    node.binding_path,
                    node.dt.dts.yamlfs,
                    style=DTShTheme.STYLE_YAML_BINDING,
                    compact=not self.flag_expand_includes,
                    linktype=_dtshconf.pref_yaml_actionable_type,
                )
            else:
                content = TextUtil.mk_apologies("Binding file unavailable.")
            sections.append(HeadingsContentWriter.Section("YAML", content))

        self._write_rich_sections(sections, out)

    def _write_rich_sections(
        self, sections: List[HeadingsContentWriter.Section], out: DTShOutput
    ) -> None:
        if len(sections) == 1:
            # If only on section, just write its content.
            out.write(sections[0].content)
        else:
            # Otherwise, use headings writer.
            HeadingsContentWriter().write_sections(sections, out)
