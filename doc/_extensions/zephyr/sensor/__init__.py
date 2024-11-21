# Copyright (c) 2024 Google Inc
# SPDX-License-Identifier: Apache-2.0

import re
import subprocess
from enum import Enum
from pathlib import Path
from typing import Any, Final

import pw_sensor.constants_generator as pw_sensor
import yaml
from bs4 import BeautifulSoup
from docutils import nodes, transforms
from sphinx.application import Sphinx
from sphinx.util import logging
from sphinx.util.docutils import SphinxDirective
from sphinx.util.template import SphinxRenderer

__version__ = "0.0.1"

ZEPHYR_BASE: Final[Path] = Path(__file__).parents[4]
TEMPLATES_DIR = Path(__file__).parent / "templates"
RESOURCES_DIR = Path(__file__).parent / "static"
DOMAIN_RESOURCES_DIR = Path(__file__).parents[1] / "domain" / "static"

logger = logging.getLogger(__name__)


class SensorDirective(SphinxDirective):
    """
    A generic directive for sensors
    """

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._sensor_spec = None

    @staticmethod
    def sort_channel_keys(s: str) -> tuple[str, int]:
        """
        Sort channel keys alphabetically except:
            Items ending in _XYZ or _DXYZ should come after [_X, _Y, _Z] and
            [_DX, _DY, _DZ] respectively.

        Args:
            s: String to sort

        Returns:
            A tuple with the substring to use for further sorting and a sorting
            priority.
        """
        text = s.upper()
        if text.endswith("_XYZ"):
            return (s[:-3], 1)  # Sort XYZ after X, Y, Z
        elif text.endswith("_DXYZ"):
            return (s[:-4], 1)  # Sort DXYZ after DX, DY, DZ
        elif text.endswith(("_X", "_Y", "_Z")):
            return (s[:-1], 0)  # Sort single/double letter suffixes before XYZ
        elif text.endswith(("_DX", "_DY", "_DZ")):
            return (s[:-2], 0)  # Sort single/double letter suffixes before DXYZ
        else:
            return (s, 2)  # Keep other strings unchanged

    @property
    def sensor_spec(self) -> pw_sensor.InputSpec:
        """
        Returns:
            The sensor spec for the current documentation build
        """
        if self._sensor_spec is None:
            sensor_yaml = self.env.domaindata["zephyr"]["sensors"]
            self._sensor_spec = pw_sensor.create_dataclass_from_dict(
                pw_sensor.InputSpec, sensor_yaml
            )
        return self._sensor_spec

    class RenderPage(Enum):
        """Supported page templates"""

        SENSOR_DETAIL_CARD = 1
        SENSOR_CATALOG = 2

    def render(self, page: RenderPage, context: dict[str, Any]) -> nodes.Node:
        """
        Render a template with some context. This function is a shorthand for
        creating a SphinxRenderer with the right template_path and selecting
        the right HTML template to render.

        Args:
            page: The page to render
            context: The variables to pass to the jinja renderer

        Returns:
            The rendered HTML node
        """
        if page == self.RenderPage.SENSOR_DETAIL_CARD:
            template_name = "sensor-summary-card.html"
        elif page == self.RenderPage.SENSOR_CATALOG:
            template_name = "sensor-catalog.html"
        else:
            raise ValueError(f"Invalid page {page}")

        renderer = SphinxRenderer(template_path=[TEMPLATES_DIR])
        html = renderer.render(
            template_name=template_name,
            context=context,
        )
        return nodes.raw(rawsource=html, text=html, format="html")

    def create_section(
        self,
        title: str,
        content: str | None = None,
        id_string: str | None = None,
    ) -> nodes.section:
        """
        Create a section with the appropriate target and content paragraph.

        Args:
            title: The title of the section
            content: Optional first paragraph to add. It will handle http and
                https references automatically.
            id_string: Optional ID for the section if we might need to make
                references to it later.

        Returns:
            A section node
        """
        section = nodes.section()
        title_node = nodes.title(text=title)
        section += title_node
        if id_string:
            id_string = nodes.make_id(id_string)
            section.attributes["ids"] = [id_string]

            target_node = nodes.target("", "", ids=[id_string], names=[id_string])
            self.state.document.note_explicit_target(target_node)
            title_node += target_node

        # Add the content paragraph to the section
        if content:
            paragraph_node = nodes.paragraph(text=content, classes=["linkable-transform"])
            section += paragraph_node

        return section


def install_static_assets(
    app: Sphinx,
    pagename: str,
    templatename: str,
    context: dict[str, Any],
    doctree: nodes.Node,
) -> None:
    app.add_css_file("css/board-catalog.css")
    app.add_css_file("css/sensor-catalog.css")
    app.add_js_file("js/sensor-catalog.js")


def update_pending_rst_references(
    app: Sphinx,
    doctree: nodes.Node,
    docname: str,
) -> None:
    """
    Search the document for any HTML tags containing the 'data-rst-ref'
    attribute, resolve that reference and update the 'href' attribute with
    the resolved reference value.

    Args:
        app: The current Sphinx running app
        doctree: The document tree to parse
        docname: The name of the current document
    """
    if doctree is None:
        return

    # We only care about raw nodes with HTML format
    for node in doctree.findall(condition=nodes.raw):
        if node.attributes.get("format") != "html":
            continue
        html_content = node.astext()
        soup = BeautifulSoup(html_content, "html.parser")

        ref_tags = soup.find_all(
            attrs={"data-rst-ref": lambda ref: ref and ref.strip()},
        )
        if len(ref_tags) > 0:
            logger.debug(f"Found {len(ref_tags)} matching <a> tags in ({docname})")
        else:
            continue
        for ref_tag in ref_tags:
            data_ref = ref_tag.attrs["data-rst-ref"]
            try:
                refnode = nodes.reference(
                    rawsource="",
                    text="",
                    internal=True,
                    refid=data_ref,
                    refexplicit=True,
                )
                resolve_reference = app.builder.env.domains["std"].resolve_xref
                resolved_ref = resolve_reference(
                    env=app.builder.env,
                    fromdocname=docname,
                    builder=app.builder,
                    typ="ref",
                    target=data_ref,
                    node=refnode,
                    contnode=None,
                )
                if resolved_ref:
                    href = resolved_ref.attributes["refuri"]
                    logger.debug(f"'{data_ref}' resolved to '{href}'")
                    ref_tag.attrs["href"] = href
                else:
                    logger.warning(f"Failed to resolve ref '{data_ref}'")
            except Exception as e:
                logger.error(f"Exception resolving reference '{data_ref}': {e}")

        new_node = nodes.raw("", str(soup), format="html")
        node.replace_self(new_node)


def configure_static_paths(app: Sphinx) -> None:
    """
    Append the required resource directories to the app's html_static_path
    list.

    Args:
        app: The current Sphinx running app
    """
    app.config.html_static_path.append(RESOURCES_DIR.as_posix())
    app.config.html_static_path.append(DOMAIN_RESOURCES_DIR.as_posix())


def parse_sensor_yaml_files(app: Sphinx) -> None:
    """
    Parse the configuration provided sensor yaml files and add the resulting
    dictionary into the domaindata ['zephyr']['sensors']

    Args:
        app: The current Sphinx running app
    """
    command = [
        "python3",
        "-m",
        "pw_sensor.sensor_desc",
        "-I",
        str(ZEPHYR_BASE),
    ] + app.env.config.sensor_yaml_files.split()
    try:
        result = subprocess.run(
            command,
            capture_output=True,
            text=True,
            check=True,
        )
        app.env.domaindata["zephyr"]["sensors"] = yaml.safe_load(result.stdout)
    except subprocess.CalledProcessError as e:
        logger.error(f"Error running generator script: {e}")
        if e.stderr:
            logger.error(e.stderr)
        raise e
    except Exception as e:
        raise e


class AutoLinkTransform(transforms.Transform):
    """
    Automatically convert URLs in paragraphs with the 'linkable-transform'
    class to links.
    """

    default_priority = 500

    url_pattern = re.compile(r"(https?://\S+)")

    def apply(self):
        """
        Traverse the document and convert URLs in specific paragraphs to links.
        """
        node: nodes.Node
        for node in self.document.traverse(nodes.paragraph):
            if "linkable-transform" in getattr(node, "classes", []):
                text = node.astext()
                new_nodes = []
                last_pos = 0
                for match in self.url_pattern.finditer(text):
                    start, end = match.span()
                    url = match.group(0)
                    new_nodes.append(nodes.Text(text[last_pos:start]))
                    new_nodes.append(nodes.reference(refuri=url, text=url))
                    last_pos = end
                new_nodes.append(nodes.Text(text[last_pos:]))

                node.children = new_nodes


def setup(app: Sphinx) -> dict[str, Any]:
    """Introduce the sensor utilities"""

    # Add the sensor_yaml_files configuration value
    app.add_config_value(name="sensor_yaml_files", default="", rebuild="html", types=str)

    # Parse the sensor yaml files into a dictionary in the domain
    app.connect("builder-inited", parse_sensor_yaml_files)

    # Add the static paths needed for sensors
    app.connect("builder-inited", configure_static_paths)

    # Install static assets such as JS and CSS files
    app.connect("html-page-context", install_static_assets)

    # Resolve pending hrefs
    app.connect("doctree-resolved", update_pending_rst_references)
    app.add_transform(AutoLinkTransform)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
