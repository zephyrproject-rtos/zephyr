"""
Zephyr Extension
################

Copyright (c) 2023 The Linux Foundation
SPDX-License-Identifier: Apache-2.0

Introduction
============

This extension adds a new ``zephyr`` domain for handling the documentation of various entities
specific to the Zephyr RTOS project (ex. code samples).

Directives
----------

- ``zephyr:code-sample::`` - Defines a code sample.
  The directive takes an ID as the main argument, and accepts ``:name:`` (human-readable short name
  of the sample) and ``:relevant-api:`` (a space separated list of Doxygen group(s) for APIs the
  code sample is a good showcase of) as options.
  The content of the directive is used as the description of the code sample.

  Example:

  ```
  .. zephyr:code-sample:: blinky
     :name: Blinky
     :relevant-api: gpio_interface

     Blink an LED forever using the GPIO API.
 ```

Roles
-----

- ``:zephyr:code-sample:`` - References a code sample.
  The role takes the ID of the code sample as the argument. The role renders as a link to the code
  sample, and the link text is the name of the code sample (or a custom text if an explicit name is
  provided).

  Example:

  ```
  Check out :zephyr:code-sample:`sample-foo` for an example of how to use the foo API. You may
  also be interested in :zephyr:code-sample:`this one <sample-bar>`.
  ```

"""
from typing import Any, Dict, Iterator, List, Tuple

from breathe.directives.content_block import DoxygenGroupDirective
from docutils import nodes
from docutils.nodes import Node
from docutils.parsers.rst import Directive, directives
from sphinx import addnodes
from sphinx.domains import Domain, ObjType
from sphinx.roles import XRefRole
from sphinx.transforms import SphinxTransform
from sphinx.transforms.post_transforms import SphinxPostTransform
from sphinx.util import logging
from sphinx.util.nodes import NodeMatcher, make_refnode

__version__ = "0.1.0"

logger = logging.getLogger(__name__)


class CodeSampleNode(nodes.Element):
    pass


class RelatedCodeSamplesNode(nodes.Element):
    pass


class ConvertCodeSampleNode(SphinxTransform):
    default_priority = 100

    def apply(self):
        matcher = NodeMatcher(CodeSampleNode)
        for node in self.document.traverse(matcher):
            self.convert_node(node)

    def convert_node(self, node):
        """
        Transforms a `CodeSampleNode` into a `nodes.section` named after the code sample name.

        Moves all sibling nodes that are after the `CodeSampleNode` in the documement under this new
        section.
        """
        parent = node.parent
        siblings_to_move = []
        if parent is not None:
            index = parent.index(node)
            siblings_to_move = parent.children[index + 1 :]

            # Create a new section
            new_section = nodes.section(ids=[node["id"]])
            new_section += nodes.title(text=node["name"])

            # Move existing content from the custom node to the new section
            new_section.extend(node.children)

            # Move the sibling nodes under the new section
            new_section.extend(siblings_to_move)

            # Replace the custom node with the new section
            node.replace_self(new_section)

            # Remove the moved siblings from their original parent
            for sibling in siblings_to_move:
                parent.remove(sibling)


class ProcessRelatedCodeSamplesNode(SphinxPostTransform):
    default_priority = 5  # before ReferencesResolver

    def run(self, **kwargs: Any) -> None:
        matcher = NodeMatcher(RelatedCodeSamplesNode)
        for node in self.document.traverse(matcher):
            id = node["id"]  # the ID of the node is the name of the doxygen group for which we
            # want to list related code samples

            code_samples = self.env.domaindata["zephyr"]["code-samples"].values()
            # Filter out code samples that don't reference this doxygen group
            code_samples = [
                code_sample for code_sample in code_samples if id in code_sample["relevant-api"]
            ]

            if len(code_samples) > 0:
                admonition = nodes.admonition()
                admonition += nodes.title(text="Related code samples")
                admonition["collapsible"] = "" # used by sphinx-immaterial theme
                admonition["classes"].append("related-code-samples")
                admonition["classes"].append("dropdown") # used by sphinx-togglebutton extension
                sample_ul = nodes.bullet_list()
                for code_sample in sorted(code_samples, key=lambda x: x["name"]):
                    sample_para = nodes.paragraph()
                    sample_xref = addnodes.pending_xref(
                        "",
                        refdomain="zephyr",
                        reftype="code-sample",
                        reftarget=code_sample["id"],
                        refwarn=True,
                    )
                    sample_xref += nodes.inline(text=code_sample["name"])
                    sample_para += sample_xref
                    sample_para += nodes.inline(text=" - ")
                    sample_para += nodes.inline(text=code_sample["description"].astext())
                    sample_li = nodes.list_item()
                    sample_li += sample_para
                    sample_ul += sample_li
                admonition += sample_ul

                # replace node with the newly created admonition
                node.replace_self(admonition)
            else:
                # remove node if there are no code samples
                node.replace_self([])


class CodeSampleDirective(Directive):
    """
    A directive for creating a code sample node in the Zephyr documentation.
    """

    required_arguments = 1  # ID
    optional_arguments = 0
    option_spec = {"name": directives.unchanged, "relevant-api": directives.unchanged}
    has_content = True

    def run(self):
        code_sample_id = self.arguments[0]
        env = self.state.document.settings.env
        code_samples = env.domaindata["zephyr"]["code-samples"]

        if code_sample_id in code_samples:
            logger.warning(
                f"Code sample {code_sample_id} already exists. "
                f"Other instance in {code_samples[code_sample_id]['docname']}",
                location=(env.docname, self.lineno),
            )

        name = self.options.get("name", code_sample_id)
        relevant_api_list = self.options.get("relevant-api", "").split()

        # Create a node for description and populate it with parsed content
        description_node = nodes.container(ids=[f"{code_sample_id}-description"])
        self.state.nested_parse(self.content, self.content_offset, description_node)

        code_sample = {
            "id": code_sample_id,
            "name": name,
            "description": description_node,
            "relevant-api": relevant_api_list,
            "docname": env.docname,
        }

        domain = env.get_domain("zephyr")
        domain.add_code_sample(code_sample)

        # Create an instance of the custom node
        code_sample_node = CodeSampleNode()
        code_sample_node["id"] = code_sample_id
        code_sample_node["name"] = name

        return [code_sample_node]


class ZephyrDomain(Domain):
    """Zephyr domain"""

    name = "zephyr"
    label = "Zephyr Project"

    roles = {
        "code-sample": XRefRole(innernodeclass=nodes.inline, warn_dangling=True),
    }

    directives = {"code-sample": CodeSampleDirective}

    object_types: Dict[str, ObjType] = {
        "code-sample": ObjType("code sample", "code-sample"),
    }

    initial_data: Dict[str, Any] = {"code-samples": {}}

    def clear_doc(self, docname: str) -> None:
        self.data["code-samples"] = {
            sample_id: sample_data
            for sample_id, sample_data in self.data["code-samples"].items()
            if sample_data["docname"] != docname
        }

    def merge_domaindata(self, docnames: List[str], otherdata: Dict) -> None:
        self.data["code-samples"].update(otherdata["code-samples"])

    def get_objects(self):
        for _, code_sample in self.data["code-samples"].items():
            yield (
                code_sample["name"],
                code_sample["name"],
                "code sample",
                code_sample["docname"],
                code_sample["id"],
                1,
            )

    # used by Sphinx Immaterial theme
    def get_object_synopses(self) -> Iterator[Tuple[Tuple[str, str], str]]:
        for _, code_sample in self.data["code-samples"].items():
            yield (
                (code_sample["docname"], code_sample["id"]),
                code_sample["description"].astext(),
            )

    def resolve_xref(self, env, fromdocname, builder, type, target, node, contnode):
        if type == "code-sample":
            code_sample_info = self.data["code-samples"].get(target)
            if code_sample_info:
                if not node.get("refexplicit"):
                    contnode = [nodes.Text(code_sample_info["name"])]

                return make_refnode(
                    builder,
                    fromdocname,
                    code_sample_info["docname"],
                    code_sample_info["id"],
                    contnode,
                    code_sample_info["description"].astext(),
                )

    def add_code_sample(self, code_sample):
        self.data["code-samples"][code_sample["id"]] = code_sample


class CustomDoxygenGroupDirective(DoxygenGroupDirective):
    """Monkey patch for Breathe's DoxygenGroupDirective."""

    def run(self) -> List[Node]:
        nodes = super().run()
        return [RelatedCodeSamplesNode(id=self.arguments[0]), *nodes]


def setup(app):
    app.add_domain(ZephyrDomain)

    app.add_transform(ConvertCodeSampleNode)
    app.add_post_transform(ProcessRelatedCodeSamplesNode)

    # monkey-patching of Breathe's DoxygenGroupDirective
    app.add_directive("doxygengroup", CustomDoxygenGroupDirective, override=True)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
