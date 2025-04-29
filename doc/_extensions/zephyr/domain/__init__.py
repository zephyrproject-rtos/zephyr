"""
Zephyr Extension
################

Copyright (c) 2023-2025 The Linux Foundation
SPDX-License-Identifier: Apache-2.0

This extension adds a new ``zephyr`` domain for handling the documentation of various entities
specific to the Zephyr RTOS project (ex. code samples).

Directives
----------

- ``zephyr:code-sample::`` - Defines a code sample.
- ``zephyr:code-sample-category::`` - Defines a category for grouping code samples.
- ``zephyr:code-sample-listing::`` - Shows a listing of code samples found in a given category.
- ``zephyr:board-catalog::`` - Shows a listing of boards supported by Zephyr.
- ``zephyr:board::`` - Flags a document as being the documentation page for a board.
- ``zephyr:board-supported-hw::`` - Shows a table of supported hardware features for all the targets
  of the board documented in the current page.
- ``zephyr:board-supported-runners::`` - Shows a table of supported runners for the board documented
  in the current page.

Roles
-----

- ``:zephyr:code-sample:`` - References a code sample.
- ``:zephyr:code-sample-category:`` - References a code sample category.
- ``:zephyr:board:`` - References a board.

"""

import json
import re
import sys
from collections.abc import Iterator
from os import path
from pathlib import Path
from typing import Any

from anytree import ChildResolverError, Node, PreOrderIter, Resolver, search
from docutils import nodes
from docutils.parsers.rst import directives, roles
from docutils.statemachine import StringList
from sphinx import addnodes
from sphinx.application import Sphinx
from sphinx.domains import Domain, ObjType
from sphinx.environment import BuildEnvironment
from sphinx.roles import XRefRole
from sphinx.transforms import SphinxTransform
from sphinx.transforms.post_transforms import SphinxPostTransform
from sphinx.util import logging
from sphinx.util.docutils import SphinxDirective, switch_source_input
from sphinx.util.nodes import NodeMatcher, make_refnode
from sphinx.util.parsing import nested_parse_to_nodes
from sphinx.util.template import SphinxRenderer

from zephyr.doxybridge import DoxygenGroupDirective
from zephyr.gh_utils import gh_link_get_url

__version__ = "0.2.0"


sys.path.insert(0, str(Path(__file__).parents[4] / "scripts/dts/python-devicetree/src"))
sys.path.insert(0, str(Path(__file__).parents[4] / "scripts/west_commands"))
sys.path.insert(0, str(Path(__file__).parents[3] / "_scripts"))

from gen_boards_catalog import get_catalog

ZEPHYR_BASE = Path(__file__).parents[4]
TEMPLATES_DIR = Path(__file__).parent / "templates"
RESOURCES_DIR = Path(__file__).parent / "static"

# Load and parse binding types from text file
BINDINGS_TXT_PATH = ZEPHYR_BASE / "dts" / "bindings" / "binding-types.txt"
ACRONYM_PATTERN = re.compile(r'([a-zA-Z0-9-]+)\s*\((.*?)\)')
BINDING_TYPE_TO_DOCUTILS_NODE = {}


def parse_text_with_acronyms(text):
    """Parse text that may contain acronyms into a list of nodes."""
    result = nodes.inline()
    last_end = 0

    for match in ACRONYM_PATTERN.finditer(text):
        # Add any text before the acronym
        if match.start() > last_end:
            result += nodes.Text(text[last_end : match.start()])

        # Add the acronym
        abbr, explanation = match.groups()
        result += nodes.abbreviation(abbr, abbr, explanation=explanation)
        last_end = match.end()

    # Add any remaining text
    if last_end < len(text):
        result += nodes.Text(text[last_end:])

    return result


with open(BINDINGS_TXT_PATH) as f:
    for line in f:
        line = line.strip()
        if not line or line.startswith('#'):
            continue

        key, value = line.split('\t', 1)
        BINDING_TYPE_TO_DOCUTILS_NODE[key] = parse_text_with_acronyms(value)

logger = logging.getLogger(__name__)


class CodeSampleNode(nodes.Element):
    pass


class RelatedCodeSamplesNode(nodes.Element):
    pass


class CodeSampleCategoryNode(nodes.Element):
    pass


class CodeSampleListingNode(nodes.Element):
    pass


class BoardNode(nodes.Element):
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

        Moves all sibling nodes that are after the `CodeSampleNode` in the document under this new
        section.

        Adds a "See Also" section at the end with links to all relevant APIs as per the samples's
        `relevant-api` attribute.
        """
        parent = node.parent
        siblings_to_move = []
        if parent is not None:
            index = parent.index(node)
            siblings_to_move = parent.children[index + 1 :]

            # Create a new section
            new_section = nodes.section(ids=[node["id"]])
            new_section += nodes.title(text=node["name"])

            gh_link = gh_link_get_url(self.app, self.env.docname)
            gh_link_button = nodes.raw(
                "",
                f"""
                <a href="{gh_link}/.." class="btn btn-info fa fa-github"
                    target="_blank" style="text-align: center;">
                    Browse source code on GitHub
                </a>
                """,
                format="html",
            )
            new_section += nodes.paragraph("", "", gh_link_button)

            # Move the sibling nodes under the new section
            new_section.extend(siblings_to_move)

            # Replace the custom node with the new section
            node.replace_self(new_section)

            # Remove the moved siblings from their original parent
            for sibling in siblings_to_move:
                parent.remove(sibling)

            # Add a "See Also" section at the end with links to relevant APIs
            if node["relevant-api"]:
                see_also_section = nodes.section(ids=["see-also"])
                see_also_section += nodes.title(text="See also")

                for api in node["relevant-api"]:
                    desc_node = addnodes.desc()
                    desc_node["domain"] = "c"
                    desc_node["objtype"] = "group"

                    title_signode = addnodes.desc_signature()
                    api_xref = addnodes.pending_xref(
                        "",
                        refdomain="c",
                        reftype="group",
                        reftarget=api,
                        refwarn=True,
                    )
                    api_xref += nodes.Text(api)
                    title_signode += api_xref
                    desc_node += title_signode
                    see_also_section += desc_node

                new_section += see_also_section

            # Set sample description as the meta description of the document for improved SEO
            meta_description = nodes.meta()
            meta_description["name"] = "description"
            meta_description["content"] = node.children[0].astext()
            node.document += meta_description

            # Similarly, add a node with JSON-LD markup (only renders in HTML output) describing
            # the code sample.
            json_ld = nodes.raw(
                "",
                f"""<script type="application/ld+json">
                {json.dumps({
                    "@context": "http://schema.org",
                    "@type": "SoftwareSourceCode",
                    "name": node['name'],
                    "description": node.children[0].astext(),
                    "codeSampleType": "full",
                    "codeRepository": gh_link_get_url(self.app, self.env.docname)
                })}
                </script>""",
                format="html",
            )
            node.document += json_ld


class ConvertCodeSampleCategoryNode(SphinxTransform):
    default_priority = 100

    def apply(self):
        matcher = NodeMatcher(CodeSampleCategoryNode)
        for node in self.document.traverse(matcher):
            self.convert_node(node)

    def convert_node(self, node):
        # move all the siblings of the category node underneath the section it contains
        parent = node.parent
        siblings_to_move = []
        if parent is not None:
            index = parent.index(node)
            siblings_to_move = parent.children[index + 1 :]

            node.children[0].extend(siblings_to_move)
            for sibling in siblings_to_move:
                parent.remove(sibling)

        # note document as needing toc patching
        self.document["needs_toc_patch"] = True

        # finally, replace the category node with the section it contains
        node.replace_self(node.children[0])


class ConvertBoardNode(SphinxTransform):
    default_priority = 100

    def apply(self):
        matcher = NodeMatcher(BoardNode)
        for node in self.document.traverse(matcher):
            self.convert_node(node)

    def convert_node(self, node):
        parent = node.parent
        siblings_to_move = []
        if parent is not None:
            index = parent.index(node)
            siblings_to_move = parent.children[index + 1 :]

            new_section = nodes.section(ids=[node["id"]])
            new_section += nodes.title(text=node["full_name"])

            # create a sidebar with all the board details
            sidebar = nodes.sidebar(classes=["board-overview"])
            new_section += sidebar
            sidebar += nodes.title(text="Board Overview")

            if node["image"] is not None:
                figure = nodes.figure()
                # set a scale of 100% to indicate we want a link to the full-size image
                figure += nodes.image(uri=f"/{node['image']}", scale=100)
                figure += nodes.caption(text=node["full_name"])
                sidebar += figure

            field_list = nodes.field_list()
            sidebar += field_list

            details = [
                ("Name", nodes.literal(text=node["id"])),
                ("Vendor", node["vendor"]),
                ("Architecture", ", ".join(node["archs"])),
                ("SoC", ", ".join(node["socs"])),
            ]

            for property_name, value in details:
                field = nodes.field()
                field_name = nodes.field_name(text=property_name)
                field_body = nodes.field_body()
                if isinstance(value, nodes.Node):
                    field_body += value
                else:
                    field_body += nodes.paragraph(text=value)
                field += field_name
                field += field_body
                field_list += field

            gh_link = gh_link_get_url(self.app, self.env.docname)
            gh_link_button = nodes.raw(
                "",
                f"""
                <div id="board-github-link">
                    <a href="{gh_link}/../.." class="btn btn-info fa fa-github"
                        target="_blank">
                        Browse board sources
                    </a>
                </div>
                """,
                format="html",
            )
            sidebar += gh_link_button

            # Move the sibling nodes under the new section
            new_section.extend(siblings_to_move)

            # Replace the custom node with the new section
            node.replace_self(new_section)

            # Remove the moved siblings from their original parent
            for sibling in siblings_to_move:
                parent.remove(sibling)


class CodeSampleCategoriesTocPatching(SphinxPostTransform):
    default_priority = 5  # needs to run *before* ReferencesResolver

    def output_sample_categories_list_items(self, tree, container: nodes.Node):
        list_item = nodes.list_item()
        compact_paragraph = addnodes.compact_paragraph()
        # find docname for tree.category["id"]
        docname = self.env.domaindata["zephyr"]["code-samples-categories"][tree.category["id"]][
            "docname"
        ]
        reference = nodes.reference(
            "",
            "",
            *[nodes.Text(tree.category["name"])],
            internal=True,
            refuri=docname,
            anchorname="",
            classes=["category-link"],
        )
        compact_paragraph += reference
        list_item += compact_paragraph

        sorted_children = sorted(tree.children, key=lambda x: x.category["name"])

        # add bullet list for children (if there are any, i.e. there are subcategories or at least
        # one code sample in the category)
        if sorted_children or any(
            code_sample.get("category") == tree.category["id"]
            for code_sample in self.env.domaindata["zephyr"]["code-samples"].values()
        ):
            bullet_list = nodes.bullet_list()
            for child in sorted_children:
                self.output_sample_categories_list_items(child, bullet_list)

            for code_sample in sorted(
                [
                    code_sample
                    for code_sample in self.env.domaindata["zephyr"]["code-samples"].values()
                    if code_sample.get("category") == tree.category["id"]
                ],
                key=lambda x: x["name"].casefold(),
            ):
                li = nodes.list_item()
                sample_xref = nodes.reference(
                    "",
                    "",
                    *[nodes.Text(code_sample["name"])],
                    internal=True,
                    refuri=code_sample["docname"],
                    anchorname="",
                    classes=["code-sample-link"],
                )
                sample_xref["reftitle"] = code_sample["description"].astext()
                compact_paragraph = addnodes.compact_paragraph()
                compact_paragraph += sample_xref
                li += compact_paragraph
                bullet_list += li

            list_item += bullet_list

        container += list_item

    def run(self, **kwargs: Any) -> None:
        if not self.document.get("needs_toc_patch"):
            return

        code_samples_categories_tree = self.env.domaindata["zephyr"]["code-samples-categories-tree"]

        category = search.find(
            code_samples_categories_tree,
            lambda node: hasattr(node, "category") and node.category["docname"] == self.env.docname,
        )

        bullet_list = nodes.bullet_list()
        self.output_sample_categories_list_items(category, bullet_list)

        self.env.tocs[self.env.docname] = bullet_list


class ProcessCodeSampleListingNode(SphinxPostTransform):
    default_priority = 5  # needs to run *before* ReferencesResolver

    def output_sample_categories_sections(self, tree, container: nodes.Node, show_titles=False):
        if show_titles:
            section = nodes.section(ids=[tree.category["id"]])

            link = make_refnode(
                self.env.app.builder,
                self.env.docname,
                tree.category["docname"],
                targetid=None,
                child=nodes.Text(tree.category["name"]),
            )
            title = nodes.title("", "", link)
            section += title
            container += section
        else:
            section = container

        # list samples from this category
        list = create_code_sample_list(
            [
                code_sample
                for code_sample in self.env.domaindata["zephyr"]["code-samples"].values()
                if code_sample.get("category") == tree.category["id"]
            ]
        )
        section += list

        sorted_children = sorted(tree.children, key=lambda x: x.name)
        for child in sorted_children:
            self.output_sample_categories_sections(child, section, show_titles=True)

    def run(self, **kwargs: Any) -> None:
        matcher = NodeMatcher(CodeSampleListingNode)

        for node in self.document.traverse(matcher):
            self.env.domaindata["zephyr"]["has_code_sample_listing"][self.env.docname] = True

            code_samples_categories = self.env.domaindata["zephyr"]["code-samples-categories"]
            code_samples_categories_tree = self.env.domaindata["zephyr"][
                "code-samples-categories-tree"
            ]

            container = nodes.container()
            container["classes"].append("code-sample-listing")

            if self.env.app.builder.format == "html" and node["live-search"]:
                search_input = nodes.raw(
                    "",
                    """
                    <div class="cs-search-bar">
                      <input type="text" class="cs-search-input"
                             placeholder="Filter code samples..." onkeyup="filterSamples(this)">
                      <i class="fa fa-search"></i>
                    </div>
                    """,
                    format="html",
                )
                container += search_input

            for category in node["categories"]:
                if category not in code_samples_categories:
                    logger.error(
                        f"Category {category} not found in code samples categories",
                        location=(self.env.docname, node.line),
                    )
                    continue

                category_node = search.find(
                    code_samples_categories_tree,
                    lambda node, category=category: hasattr(node, "category")
                    and node.category["id"] == category,
                )
                self.output_sample_categories_sections(category_node, container)

            node.replace_self(container)


def create_code_sample_list(code_samples):
    """
    Creates a bullet list (`nodes.bullet_list`) of code samples from a list of code samples.

    The list is alphabetically sorted (case-insensitive) by the code sample name.
    """

    ul = nodes.bullet_list(classes=["code-sample-list"])

    for code_sample in sorted(code_samples, key=lambda x: x["name"].casefold()):
        li = nodes.list_item()

        sample_xref = addnodes.pending_xref(
            "",
            refdomain="zephyr",
            reftype="code-sample",
            reftarget=code_sample["id"],
            refwarn=True,
        )
        sample_xref += nodes.Text(code_sample["name"])
        li += nodes.inline("", "", sample_xref, classes=["code-sample-name"])

        li += nodes.inline(
            text=code_sample["description"].astext(),
            classes=["code-sample-description"],
        )

        ul += li

    return ul


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
                admonition["classes"].append("related-code-samples")
                admonition["classes"].append("dropdown")  # used by sphinx-togglebutton extension
                admonition["classes"].append("toggle-shown")  # show the content by default

                sample_list = create_code_sample_list(code_samples)
                admonition += sample_list

                # replace node with the newly created admonition
                node.replace_self(admonition)
            else:
                # remove node if there are no code samples
                node.replace_self([])


class CodeSampleDirective(SphinxDirective):
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
        code_sample_node["relevant-api"] = relevant_api_list
        code_sample_node += description_node

        return [code_sample_node]


class CodeSampleCategoryDirective(SphinxDirective):
    required_arguments = 1  # Category ID
    optional_arguments = 0
    option_spec = {
        "name": directives.unchanged,
        "show-listing": directives.flag,
        "live-search": directives.flag,
        "glob": directives.unchanged,
    }
    has_content = True  # Category description
    final_argument_whitespace = True

    def run(self):
        env = self.state.document.settings.env
        id = self.arguments[0]
        name = self.options.get("name", id)

        category_node = CodeSampleCategoryNode()
        category_node["id"] = id
        category_node["name"] = name
        category_node["docname"] = env.docname

        description_node = self.parse_content_to_nodes()
        category_node["description"] = description_node

        code_sample_category = {
            "docname": env.docname,
            "id": id,
            "name": name,
        }

        # Add the category to the domain
        domain = env.get_domain("zephyr")
        domain.add_code_sample_category(code_sample_category)

        # Fake a toctree directive to ensure the code-sample-category directive implicitly acts as
        # a toctree and correctly mounts whatever relevant documents under it in the global toc
        lines = [
            name,
            "#" * len(name),
            "",
            ".. toctree::",
            "   :titlesonly:",
            "   :glob:",
            "   :hidden:",
            "   :maxdepth: 1",
            "",
            f"   {self.options['glob']}" if "glob" in self.options else "   */*",
            "",
        ]
        stringlist = StringList(lines, source=env.docname)

        with switch_source_input(self.state, stringlist):
            parsed_section = nested_parse_to_nodes(self.state, stringlist)[0]

        category_node += parsed_section

        parsed_section += description_node

        if "show-listing" in self.options:
            listing_node = CodeSampleListingNode()
            listing_node["categories"] = [id]
            listing_node["live-search"] = "live-search" in self.options
            parsed_section += listing_node

        return [category_node]


class CodeSampleListingDirective(SphinxDirective):
    has_content = False
    required_arguments = 0
    optional_arguments = 0
    option_spec = {
        "categories": directives.unchanged_required,
        "live-search": directives.flag,
    }

    def run(self):
        code_sample_listing_node = CodeSampleListingNode()
        code_sample_listing_node["categories"] = self.options.get("categories").split()
        code_sample_listing_node["live-search"] = "live-search" in self.options

        return [code_sample_listing_node]


class BoardDirective(SphinxDirective):
    has_content = False
    required_arguments = 1
    optional_arguments = 0

    def run(self):
        # board_name is passed as the directive argument
        board_name = self.arguments[0]

        boards = self.env.domaindata["zephyr"]["boards"]
        vendors = self.env.domaindata["zephyr"]["vendors"]

        if board_name not in boards:
            logger.warning(
                f"Board {board_name} does not seem to be a valid board name.",
                location=(self.env.docname, self.lineno),
            )
            return []
        elif "docname" in boards[board_name]:
            logger.warning(
                f"Board {board_name} is already documented in {boards[board_name]['docname']}.",
                location=(self.env.docname, self.lineno),
            )
            return []
        else:
            self.env.domaindata["zephyr"]["has_board"][self.env.docname] = True
            board = boards[board_name]
            # flag board in the domain data as now having a documentation page so that it can be
            # cross-referenced etc.
            board["docname"] = self.env.docname

            board_node = BoardNode(id=board_name)
            board_node["full_name"] = board["full_name"]
            board_node["vendor"] = vendors.get(board["vendor"], board["vendor"])
            board_node["revision_default"] = board["revision_default"]
            board_node["supported_features"] = board["supported_features"]
            board_node["archs"] = board["archs"]
            board_node["socs"] = board["socs"]
            board_node["image"] = board["image"]
            board_node["supported_runners"] = board["supported_runners"]
            board_node["flash_runner"] = board["flash_runner"]
            board_node["debug_runner"] = board["debug_runner"]
            return [board_node]


class BoardCatalogDirective(SphinxDirective):
    has_content = False
    required_arguments = 0
    optional_arguments = 0

    def run(self):
        if self.env.app.builder.format == "html":
            self.env.domaindata["zephyr"]["has_board_catalog"][self.env.docname] = True

            domain_data = self.env.domaindata["zephyr"]
            renderer = SphinxRenderer([TEMPLATES_DIR])
            rendered = renderer.render(
                "board-catalog.html",
                {
                    "boards": domain_data["boards"],
                    "vendors": domain_data["vendors"],
                    "socs": domain_data["socs"],
                    "hw_features_present": self.env.app.config.zephyr_generate_hw_features,
                },
            )
            return [nodes.raw("", rendered, format="html")]
        else:
            return [nodes.paragraph(text="Board catalog is only available in HTML.")]


class BoardSupportedHardwareDirective(SphinxDirective):
    """A directive for showing the supported hardware features of a board."""

    has_content = False
    required_arguments = 0
    optional_arguments = 0

    def run(self):
        env = self.env
        docname = env.docname

        matcher = NodeMatcher(BoardNode)
        board_nodes = list(self.state.document.traverse(matcher))
        if not board_nodes:
            logger.warning(
                "board-supported-hw directive must be used in a board documentation page.",
                location=(docname, self.lineno),
            )
            return []

        board_node = board_nodes[0]
        supported_features = board_node["supported_features"]
        result_nodes = []

        paragraph = nodes.paragraph()
        paragraph += nodes.Text("The ")
        paragraph += nodes.literal(text=board_node["id"])
        paragraph += nodes.Text(" board supports the hardware features listed below.")
        result_nodes.append(paragraph)

        if not env.app.config.zephyr_generate_hw_features:
            note = nodes.admonition()
            note += nodes.title(text="Note")
            note["classes"].append("warning")
            note += nodes.paragraph(
                text="The list of supported hardware features was not generated. Run a full "
                "documentation build for the required metadata to be available."
            )
            result_nodes.append(note)
            return result_nodes

        html_contents = """<div class="legend admonition">
  <dl class="supported-hardware field-list">
    <dt>
      <span class="location-chip onchip">on-chip</span> /
      <span class="location-chip onboard">on-board</span>
    </dt>
    <dd>
      Feature integrated in the SoC / present on the board.
    </dd>
    <dt>
      <span class="count okay-count">2</span> /
      <span class="count disabled-count">2</span>
    </dt>
    <dd>
      Number of instances that are enabled / disabled. <br/>
      Click on the label to see the first instance of this feature in the board/SoC DTS files.
    </dd>
    <dt>
      <code class="docutils literal notranslate"><span class="pre">vnd,foo</span></code>
    </dt>
    <dd>
      Compatible string for the Devicetree binding matching the feature. <br/>
      Click on the link to view the binding documentation.
    </dd>
  </dl>
</div>"""
        result_nodes.append(nodes.raw("", html_contents, format="html"))

        tables_container = nodes.container(ids=[f"{board_node['id']}-hw-features"])
        result_nodes.append(tables_container)

        board_json = json.dumps(
            {
                "board_name": board_node["id"],
                "revision_default": board_node["revision_default"],
                "targets": list(supported_features.keys()),
            }
        )
        result_nodes.append(
            nodes.raw(
                "",
                f"""<script>board_data = {board_json}</script>""",
                format="html",
            )
        )

        for target, features in sorted(supported_features.items()):
            if not features:
                continue

            target_heading = nodes.section(ids=[f"{board_node['id']}-{target}-hw-features-section"])
            heading = nodes.title()
            heading += nodes.literal(text=target)
            heading += nodes.Text(" target")
            target_heading += heading
            tables_container += target_heading

            table = nodes.table(
                classes=["colwidths-given", "hardware-features"],
                ids=[f"{board_node['id']}-{target}-hw-features-table"],
            )
            tgroup = nodes.tgroup(cols=4)

            tgroup += nodes.colspec(colwidth=15, classes=["type"])
            tgroup += nodes.colspec(colwidth=12, classes=["location"])
            tgroup += nodes.colspec(colwidth=53, classes=["description"])
            tgroup += nodes.colspec(colwidth=20, classes=["compatible"])

            thead = nodes.thead()
            row = nodes.row()
            headers = ["Type", "Location", "Description", "Compatible"]
            for header in headers:
                entry = nodes.entry(classes=[header.lower()])
                entry += nodes.paragraph(text=header)
                row += entry
            thead += row
            tgroup += thead

            tbody = nodes.tbody()

            def feature_sort_key(feature):
                # Put "CPU" first. Later updates might also give priority to features
                # like "sensor"s, for example.
                if feature == "cpu":
                    return (0, feature)
                return (1, feature)

            sorted_features = sorted(features.keys(), key=feature_sort_key)

            for feature in sorted_features:
                items = list(features[feature].items())
                num_items = len(items)

                for i, (key, value) in enumerate(items):
                    row = nodes.row()

                    # TYPE column
                    if i == 0:
                        type_entry = nodes.entry(morerows=num_items - 1, classes=["type"])
                        type_entry += nodes.paragraph(
                            "",
                            "",
                            BINDING_TYPE_TO_DOCUTILS_NODE.get(
                                feature, nodes.Text(feature)
                            ).deepcopy(),
                        )
                        row += type_entry

                    # LOCATION column
                    location_entry = nodes.entry(classes=["location"])
                    location_para = nodes.paragraph()

                    if "board" in value["locations"]:
                        location_chip = nodes.inline(
                            classes=["location-chip", "onboard"],
                            text="on-board",
                        )
                        location_para += location_chip
                    elif "soc" in value["locations"]:
                        location_chip = nodes.inline(
                            classes=["location-chip", "onchip"],
                            text="on-chip",
                        )
                        location_para += location_chip

                    location_entry += location_para
                    row += location_entry

                    # DESCRIPTION column
                    desc_entry = nodes.entry(classes=["description"])
                    desc_para = nodes.paragraph(classes=["status"])
                    desc_para += nodes.Text(value["description"])

                    # Add count indicators for okay and not-okay instances
                    okay_nodes = value.get("okay_nodes", [])
                    disabled_nodes = value.get("disabled_nodes", [])

                    role_fn, _ = roles.role(
                        "zephyr_file", self.state_machine.language, self.lineno, self.state.reporter
                    )

                    def create_count_indicator(nodes_list, class_type, role_function=role_fn):
                        if not nodes_list:
                            return None

                        count = len(nodes_list)

                        if role_function is None:
                            return nodes.inline(
                                classes=["count", f"{class_type}-count"], text=str(count)
                            )

                        # Create a reference to the first node in the list
                        first_node = nodes_list[0]
                        file_ref = f"{count} <{first_node['filename']}#L{first_node['lineno']}>"

                        role_nodes, _ = role_function(
                            "zephyr_file", file_ref, file_ref, self.lineno, self.state.inliner
                        )

                        count_node = role_nodes[0]
                        count_node["classes"] = ["count", f"{class_type}-count"]

                        return count_node

                    desc_para += create_count_indicator(okay_nodes, "okay")
                    desc_para += create_count_indicator(disabled_nodes, "disabled")

                    desc_entry += desc_para
                    row += desc_entry

                    # COMPATIBLE column
                    compatible_entry = nodes.entry(classes=["compatible"])
                    xref = addnodes.pending_xref(
                        "",
                        refdomain="std",
                        reftype="dtcompatible",
                        reftarget=key,
                        refexplicit=False,
                        refwarn=(not value.get("custom_binding", False)),
                    )
                    xref += nodes.literal(text=key)
                    compatible_entry += nodes.paragraph("", "", xref)
                    row += compatible_entry

                    tbody += row

            tgroup += tbody
            table += tgroup
            tables_container += table

        return result_nodes


class BoardSupportedRunnersDirective(SphinxDirective):
    """A directive for showing the supported runners of a board."""

    has_content = False
    required_arguments = 0
    optional_arguments = 0

    def run(self):
        env = self.env
        docname = env.docname

        matcher = NodeMatcher(BoardNode)
        board_nodes = list(self.state.document.traverse(matcher))
        if not board_nodes:
            logger.warning(
                "board-supported-runners directive must be used in a board documentation page.",
                location=(docname, self.lineno),
            )
            return []

        if not env.app.config.zephyr_generate_hw_features:
            note = nodes.admonition()
            note += nodes.title(text="Note")
            note["classes"].append("warning")
            note += nodes.paragraph(
                text="The list of supported runners was not generated. Run a full documentation "
                "build for the required metadata to be available."
            )
            return [note]

        board_node = board_nodes[0]
        runners = board_node["supported_runners"]
        flash_runner = board_node["flash_runner"]
        debug_runner = board_node["debug_runner"]

        result_nodes = []

        paragraph = nodes.paragraph()
        paragraph += nodes.Text("The ")
        paragraph += nodes.literal(text=board_node["id"])
        paragraph += nodes.Text(
            " board supports the runners and associated west commands listed below."
        )
        result_nodes.append(paragraph)

        env_runners = env.domaindata["zephyr"]["runners"]
        commands = ["flash", "debug"]
        for runner in env_runners:
            if runner in board_node["supported_runners"]:
                for cmd in env_runners[runner].get("commands", []):
                    if cmd not in commands:
                        commands.append(cmd)

        # create the table
        table = nodes.table(classes=["colwidths-given", "runners-table"])
        tgroup = nodes.tgroup(cols=len(commands) + 1)  # +1 for the Runner column

        # Add colspec for Runner column
        tgroup += nodes.colspec(colwidth=15, classes=["type"])
        # Add colspecs for command columns
        for _ in commands:
            tgroup += nodes.colspec(colwidth=15, classes=["type"])

        thead = nodes.thead()
        row = nodes.row()
        entry = nodes.entry()
        row += entry
        headers = [*commands]
        for header in headers:
            entry = nodes.entry(classes=[header.lower()])
            entry += addnodes.literal_strong(text=header, classes=["command"])
            row += entry
        thead += row
        tgroup += thead

        tbody = nodes.tbody()

        # add a row for each runner
        for runner in sorted(runners):
            row = nodes.row()
            # First column - Runner name
            entry = nodes.entry()

            xref = addnodes.pending_xref(
                "",
                refdomain="std",
                reftype="ref",
                reftarget=f"runner_{runner}",
                refexplicit=True,
                refwarn=False,
            )
            xref += nodes.Text(runner)
            entry += addnodes.literal_strong("", "", xref)
            row += entry

            # Add columns for each command
            for command in commands:
                entry = nodes.entry()
                if command in env_runners[runner].get("commands", []):
                    entry += nodes.Text("✅")
                    if (command == "flash" and runner == flash_runner) or (
                        command == "debug" and runner == debug_runner
                    ):
                        entry += nodes.Text(" (default)")
                row += entry
            tbody += row

        tgroup += tbody
        table += tgroup

        result_nodes.append(table)

        return result_nodes


class ZephyrDomain(Domain):
    """Zephyr domain"""

    name = "zephyr"
    label = "Zephyr"

    roles = {
        "code-sample": XRefRole(innernodeclass=nodes.inline, warn_dangling=True),
        "code-sample-category": XRefRole(innernodeclass=nodes.inline, warn_dangling=True),
        "board": XRefRole(innernodeclass=nodes.inline, warn_dangling=True),
    }

    directives = {
        "code-sample": CodeSampleDirective,
        "code-sample-listing": CodeSampleListingDirective,
        "code-sample-category": CodeSampleCategoryDirective,
        "board-catalog": BoardCatalogDirective,
        "board": BoardDirective,
        "board-supported-hw": BoardSupportedHardwareDirective,
        "board-supported-runners": BoardSupportedRunnersDirective,
    }

    object_types: dict[str, ObjType] = {
        "code-sample": ObjType("code sample", "code-sample"),
        "code-sample-category": ObjType("code sample category", "code-sample-category"),
        "board": ObjType("board", "board"),
    }

    initial_data: dict[str, Any] = {
        "code-samples": {},  # id -> code sample data
        "code-samples-categories": {},  # id -> code sample category data
        "code-samples-categories-tree": Node("samples"),
        # keep track of documents containing special directives
        "has_code_sample_listing": {},  # docname -> bool
        "has_board_catalog": {},  # docname -> bool
        "has_board": {},  # docname -> bool
    }

    def clear_doc(self, docname: str) -> None:
        self.data["code-samples"] = {
            sample_id: sample_data
            for sample_id, sample_data in self.data["code-samples"].items()
            if sample_data["docname"] != docname
        }

        self.data["code-samples-categories"] = {
            category_id: category_data
            for category_id, category_data in self.data["code-samples-categories"].items()
            if category_data["docname"] != docname
        }

        # TODO clean up the anytree as well

        self.data["has_code_sample_listing"].pop(docname, None)
        self.data["has_board_catalog"].pop(docname, None)
        self.data["has_board"].pop(docname, None)

    def merge_domaindata(self, docnames: list[str], otherdata: dict) -> None:
        self.data["code-samples"].update(otherdata["code-samples"])
        self.data["code-samples-categories"].update(otherdata["code-samples-categories"])

        # self.data["boards"] contains all the boards right from builder-inited time, but it still
        # potentially needs merging since a board's docname property is set by BoardDirective to
        # indicate the board is documented in a specific document.
        for board_name, board in otherdata["boards"].items():
            if "docname" in board:
                self.data["boards"][board_name]["docname"] = board["docname"]

        # merge category trees by adding all the categories found in the "other" tree that to
        # self tree
        other_tree = otherdata["code-samples-categories-tree"]
        categories = [n for n in PreOrderIter(other_tree) if hasattr(n, "category")]
        for category in categories:
            category_path = f"/{'/'.join(n.name for n in category.path)}"
            self.add_category_to_tree(
                category_path,
                category.category["id"],
                category.category["name"],
                category.category["docname"],
            )

        for docname in docnames:
            self.data["has_code_sample_listing"][docname] = otherdata[
                "has_code_sample_listing"
            ].get(docname, False)
            self.data["has_board_catalog"][docname] = otherdata["has_board_catalog"].get(
                docname, False
            )
            self.data["has_board"][docname] = otherdata["has_board"].get(docname, False)

    def get_objects(self):
        for _, code_sample in self.data["code-samples"].items():
            yield (
                code_sample["id"],
                code_sample["name"],
                "code-sample",
                code_sample["docname"],
                code_sample["id"],
                1,
            )

        for _, code_sample_category in self.data["code-samples-categories"].items():
            yield (
                code_sample_category["id"],
                code_sample_category["name"],
                "code-sample-category",
                code_sample_category["docname"],
                code_sample_category["id"],
                1,
            )

        for _, board in self.data["boards"].items():
            # only boards that do have a documentation page are to be considered as valid objects
            if "docname" in board:
                yield (
                    board["name"],
                    board["full_name"],
                    "board",
                    board["docname"],
                    board["name"],
                    1,
                )

    # used by Sphinx Immaterial theme
    def get_object_synopses(self) -> Iterator[tuple[tuple[str, str], str]]:
        for _, code_sample in self.data["code-samples"].items():
            yield (
                (code_sample["docname"], code_sample["id"]),
                code_sample["description"].astext(),
            )

    def resolve_xref(self, env, fromdocname, builder, type, target, node, contnode):
        if type == "code-sample":
            elem = self.data["code-samples"].get(target)
        elif type == "code-sample-category":
            elem = self.data["code-samples-categories"].get(target)
        elif type == "board":
            elem = self.data["boards"].get(target)
        else:
            return

        if elem and "docname" in elem:
            if not node.get("refexplicit"):
                contnode = [nodes.Text(elem["name"] if type != "board" else elem["full_name"])]

            return make_refnode(
                builder,
                fromdocname,
                elem["docname"],
                elem["id"] if type != "board" else elem["name"],
                contnode,
                elem["description"].astext() if type == "code-sample" else None,
            )

    def add_code_sample(self, code_sample):
        self.data["code-samples"][code_sample["id"]] = code_sample

    def add_code_sample_category(self, code_sample_category):
        self.data["code-samples-categories"][code_sample_category["id"]] = code_sample_category
        self.add_category_to_tree(
            path.dirname(code_sample_category["docname"]),
            code_sample_category["id"],
            code_sample_category["name"],
            code_sample_category["docname"],
        )

    def add_category_to_tree(
        self, category_path: str, category_id: str, category_name: str, docname: str
    ) -> Node:
        resolver = Resolver("name")
        tree = self.data["code-samples-categories-tree"]

        if not category_path.startswith("/"):
            category_path = "/" + category_path

        # node either already exists (and we update it to make it a category node), or we need to
        # create it
        try:
            node = resolver.get(tree, category_path)
            if hasattr(node, "category") and node.category["id"] != category_id:
                raise ValueError(
                    f"Can't add code sample category {category_id} as category "
                    f"{node.category['id']} is already defined in {node.category['docname']}. "
                    "You may only have one category per path."
                )
        except ChildResolverError as e:
            path_of_last_existing_node = f"/{'/'.join(n.name for n in e.node.path)}"
            common_path = path.commonpath([path_of_last_existing_node, category_path])
            remaining_path = path.relpath(category_path, common_path)

            # Add missing nodes under the last existing node
            for node_name in remaining_path.split("/"):
                e.node = Node(node_name, parent=e.node)

            node = e.node

        node.category = {"id": category_id, "name": category_name, "docname": docname}

        return tree


class CustomDoxygenGroupDirective(DoxygenGroupDirective):
    """Monkey patch for Breathe's DoxygenGroupDirective."""

    def run(self) -> list[Node]:
        nodes = super().run()

        if self.config.zephyr_breathe_insert_related_samples:
            return [*nodes, RelatedCodeSamplesNode(id=self.arguments[0])]
        else:
            return nodes


def compute_sample_categories_hierarchy(app: Sphinx, env: BuildEnvironment) -> None:
    domain = env.get_domain("zephyr")
    code_samples = domain.data["code-samples"]

    category_tree = env.domaindata["zephyr"]["code-samples-categories-tree"]
    resolver = Resolver("name")
    for code_sample in code_samples.values():
        try:
            # Try to get the node at the specified path
            node = resolver.get(category_tree, "/" + path.dirname(code_sample["docname"]))
        except ChildResolverError as e:
            # starting with e.node and up, find the first node that has a category
            node = e.node
            while not hasattr(node, "category"):
                node = node.parent
            code_sample["category"] = node.category["id"]


def install_static_assets_as_needed(
    app: Sphinx, pagename: str, templatename: str, context: dict[str, Any], doctree: nodes.Node
) -> None:
    if app.env.domaindata["zephyr"]["has_code_sample_listing"].get(pagename, False):
        app.add_css_file("css/codesample-livesearch.css")
        app.add_js_file("js/codesample-livesearch.js")

    if app.env.domaindata["zephyr"]["has_board_catalog"].get(pagename, False):
        app.add_css_file("css/board-catalog.css")
        app.add_js_file("js/board-catalog.js")

    if app.env.domaindata["zephyr"]["has_board"].get(pagename, False):
        app.add_css_file("css/board.css")
        app.add_js_file("js/board.js")


def load_board_catalog_into_domain(app: Sphinx) -> None:
    board_catalog = get_catalog(
        generate_hw_features=(
            app.builder.format == "html" and app.config.zephyr_generate_hw_features
        )
    )
    app.env.domaindata["zephyr"]["boards"] = board_catalog["boards"]
    app.env.domaindata["zephyr"]["vendors"] = board_catalog["vendors"]
    app.env.domaindata["zephyr"]["socs"] = board_catalog["socs"]
    app.env.domaindata["zephyr"]["runners"] = board_catalog["runners"]


def setup(app):
    app.add_config_value("zephyr_breathe_insert_related_samples", False, "env")
    app.add_config_value("zephyr_generate_hw_features", False, "env")

    app.add_domain(ZephyrDomain)

    app.add_transform(ConvertCodeSampleNode)
    app.add_transform(ConvertCodeSampleCategoryNode)
    app.add_transform(ConvertBoardNode)

    app.add_post_transform(ProcessCodeSampleListingNode)
    app.add_post_transform(CodeSampleCategoriesTocPatching)
    app.add_post_transform(ProcessRelatedCodeSamplesNode)

    app.connect(
        "builder-inited",
        (lambda app: app.config.html_static_path.append(RESOURCES_DIR.as_posix())),
    )
    app.connect("builder-inited", load_board_catalog_into_domain)

    app.connect("html-page-context", install_static_assets_as_needed)
    app.connect("env-updated", compute_sample_categories_hierarchy)

    # monkey-patching of the DoxygenGroupDirective
    app.add_directive("doxygengroup", CustomDoxygenGroupDirective, override=True)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
