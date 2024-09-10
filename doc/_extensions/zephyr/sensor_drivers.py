from typing import Any, Dict, List, Final, Sequence

import re
import os
import subprocess
from pathlib import Path
from docutils import nodes
from docutils.statemachine import ViewList
from docutils.parsers.rst import directives
from docutils.parsers.rst import roles
from sphinx.application import Sphinx
from sphinx.util.docutils import SphinxDirective
from sphinx.util import logging
import pw_sensor.constants_generator as pw_sensor
import yaml

__version__ = "0.0.43"

ZEPHYR_BASE: Final[Path] = Path(__file__).parents[3]
PIGWEED_BASE: Final[Path] = Path(os.environ["PW_ROOT"])

logger = logging.getLogger(__name__)


def _sensor_channel_filter_name(channel_id: str) -> str:
    return f"sensor-channel-{channel_id}"


def _sensor_attribute_filter_name(attribute_id: str) -> str:
    return f"sensor-attribute-{attribute_id}"


def _sensor_trigger_filter_name(filter_id: str) -> str:
    return f"sensor-trigger-{filter_id}"


def _sensor_bus_filter_name(bus_id: str) -> str:
    return f"sensor-bus-{bus_id}"


def _get_filter_container_string(spec: pw_sensor.InputSpec) -> str:
    """ """
    channels: Set[str] = set()
    attributes: Set[str] = set()
    triggers: Set[str] = set()
    buses: Set[str] = set()

    for sensor_spec in spec.sensors.values():
        channels.update(sensor_spec.channels.keys())
        attributes.update(
            [attribute_spec.attribute for attribute_spec in sensor_spec.attributes]
        )
        buses.update(sensor_spec.supported_buses)
        triggers.update(sensor_spec.triggers)
    html = """
<div id="sensor-filter-container">
  <h2>Filters</h2>
"""
    if channels:
        html += "<h3>Supported Channels:</h3>\n"
        html += '  <div id="sensor-filter-channel-container">\n'
        # TODO sort these and others
        for channel_id in channels:
            html += (
                '  <button class="sensor-filter-button"'
                + f' filter="{_sensor_channel_filter_name(channel_id)}">'
                + f"{channel_id.upper()}</button>\n"
            )
        html += "  </div>\n"

    if attributes:
        html += "  <h3>Supported Attributes:</h3>\n"
        html += '  <div id="sensor-filter-attribute-container">\n'
        for attribute_id in attributes:
            html += (
                '  <button class="sensor-filter-button"'
                + f' filter="{_sensor_attribute_filter_name(attribute_id)}">'
                + f"{attribute_id.upper()}</button>\n"
            )
        html += "  </div>\n"

    if triggers:
        html += "  <h3>Supported Triggers:</h3>\n"
        html += '  <div id="sensor-filter-trigger-container">\n'
        for trigger_id in triggers:
            html += (
                '  <button class="sensor-filter-button"'
                + f' filter="{_sensor_trigger_filter_name(trigger_id)}">'
                + f"{trigger_id.upper()}</button>\n"
            )
        html += "  </div>\n"

    if buses:
        html += "  <h3>Supported Buses:</h3>\n"
        html += '  <div id="sensor-filter-bus-container">\n'
        for bus_name in buses:
            html += (
                '  <button class="sensor-filter-button"'
                + f' filter="{_sensor_bus_filter_name(bus_name)}">'
                + f"{bus_name.upper()}</button>\n"
            )
        html += "  </div>\n"

    html += "</div>\n"
    html += """
<script>
const filterButtons = document.querySelectorAll('button.sensor-filter-button');

filterButtons.forEach(button => {
  button.addEventListener('click', () => {
    button.classList.toggle('active');
    filterSensorDrivers();
  });
});

function filterSensorDrivers() {
  // Get all active filter buttons
  const activeFilters = Array.from(document.querySelectorAll('button.sensor-filter-button.active'))
    .map(button => button.getAttribute('filter'));

  // Get all sensor-driver sections
  const sensorDriverSections = document.querySelectorAll('section.sensor-driver');

  // Filter and show/hide sections
  sensorDriverSections.forEach(section => {
    const filterSpan = section.querySelector('.sensor-filter-data');
    const sectionFilters = filterSpan.textContent.trim().split(' ');
    const isVisible = activeFilters.every(filter => sectionFilters.includes(filter));

    section.style.display = isVisible ? 'block' : 'none';
  });

  // Find all sensor-org sections
  const sensorOrgSections = document.querySelectorAll('section.sensor-org');

  // Iterate over each sensor-org section
  sensorOrgSections.forEach(orgSection => {
    const sensorDriverSections = orgSection.querySelectorAll('section.sensor-driver');

    // Check if all sensor-driver sections are hidden
    const allHidden = Array.from(sensorDriverSections).every(section => section.style.display === 'none');

    // Hide the sensor-org section if all its children are hidden
    orgSection.style.display = allHidden ? 'none' : 'block';
  });
}
</script>
"""
    return html


class SensorDriverListDirective(SphinxDirective):
    """List of all sensor drivers"""

    has_content = True

    def _create_section(
        self,
        title: str,
        content: None | str = None,
        id_string_override: str | None = None,
    ) -> nodes.section:
        # Create a unique ID for each section to avoid potential conflicts
        if id_string_override:
            section_id = nodes.make_id(id_string_override)
        else:
            section_id = nodes.make_id(title)

        # Create the section node with the ID
        section = nodes.section(ids=[section_id])
        section += nodes.title(text=title.title())

        # Add the content paragraph to the section
        if content:
            # Split content into text nodes and reference nodes for URLs
            content_nodes: List[nodes.Node] = []
            for part in re.split(r"(\bhttps?://\S+/?\b)", content):
                if part.startswith("http"):
                    # Create a reference node for the URL
                    ref_node = nodes.reference(
                        "",  # Internal reference text (usually empty)
                        part,  # The URL itself as the text content
                        refuri=part,  # The URL as the target
                    )
                    content_nodes.append(ref_node)
                else:
                    # Regular text node
                    content_nodes.append(nodes.Text(part))

            # Add the content nodes to a paragraph
            paragraph_node = nodes.paragraph(text="")
            paragraph_node.extend(content_nodes)
            section += paragraph_node
        return section

    def _get_all_sensors(self) -> dict:
        command = [
            "python3",
            str(PIGWEED_BASE / "pw_sensor" / "py" / "pw_sensor" / "sensor_desc.py"),
            "-I",
            str(ZEPHYR_BASE),
        ] + self.env.config.sensor_yaml_files
        try:
            result = subprocess.run(
                command,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=True,
            )
            logger.info(result.stdout)
            return yaml.safe_load(result.stdout)
        except subprocess.CalledProcessError as e:
            logger.error(f"Error running generator script: {e}")
            if e.stderr:
                logger.error(e.stderr)
            raise e
        except Exception as e:
            raise e

    def _make_admonition(
        self, title: str = "Note:", clazz: str = "note"
    ) -> nodes.admonition:
        admonition_node = nodes.admonition(text="", classes=[clazz])

        # Create a paragraph with the class admonition-title for the title
        title_paragraph = nodes.paragraph(text=title, classes=["admonition-title"])
        admonition_node += title_paragraph

        return admonition_node

    def _make_paragraph(self, parts: List[nodes.Node]) -> nodes.paragraph:
        paragraph_node = nodes.paragraph(text="")
        paragraph_node.extend(parts)
        return paragraph_node

    def _get_sensors_section(self, sensors: List[pw_sensor.SensorSpec]) -> nodes.Node:
        sensors.sort(key=lambda sensor: sensor.compatible.org)

        section = self._create_section(title="Supported Sensors")

        current_org_section: None | nodes.section = None
        current_org: None | str = None
        for sensor in sensors:
            if sensor.compatible.org != current_org:
                current_org = sensor.compatible.org
                current_org_section = self._create_section(
                    title=sensor.compatible.org.title(),
                )
                current_org_section.attributes["classes"].append("sensor-org")
                section += current_org_section
            assert current_org_section is not None
            sensor_section = self._create_section(
                title=sensor.compatible.part,
                content=sensor.description,
            )
            filters = (
                [
                    _sensor_channel_filter_name(channel_id)
                    for channel_id in sensor.channels.keys()
                ]
                + [
                    _sensor_attribute_filter_name(attribute_spec.attribute)
                    for attribute_spec in sensor.attributes
                ]
                + [
                    _sensor_bus_filter_name(bus_id)
                    for bus_id in sensor.supported_buses
                ]
                + [
                    _sensor_trigger_filter_name(trigger_id)
                    for trigger_id in sensor.triggers
                ]
            )
            print("Filters: " + str(filters))
            filter_span = nodes.inline(
                classes=["sensor-filter-data"], style="display: none;"
            )
            filter_span += nodes.Text(" ".join(filters))
            sensor_section += filter_span
            sensor_section.attributes["classes"].append("sensor-driver")
            
            # Add list of supported channels, triggers, and attributes
            channel_list = nodes.bullet_list()
            sensor_section += channel_list

            for channel_id in sensor.channels.keys():
                list_item = nodes.list_item()
                channel_list += list_item

                anchor = nodes.make_id(f'sensor_chan_{channel_id}')
                link = nodes.raw(
                    '',
                    f'<a href="channels.html#{anchor}">{channel_id.upper()}</a>',
                    format='html',
                )
                list_item += link
                # reference = nodes.reference(
                #     internal=False,
                #     refuri='channels.html#sensor_chan_' + channel_id
                # )
                # reference += nodes.paragraph(text=channel_id.upper())
                # list_item += reference

            # print("sensor_section: " + str(sensor_section))
            current_org_section += sensor_section
        return section

    def _get_attributes_section(
        self, attributes: Dict[str, pw_sensor.AttributeSpec]
    ) -> nodes.Node:
        keys: List[str] = list(attributes.keys())
        keys.sort()

        section = self._create_section(title="Supported Attributes")

        for key in keys:
            attribute: pw_sensor.AttributeSpec = attributes[key]
            attribute_section = self._create_section(
                title=attribute.name,
                content=attribute.description,
                id_string_override=key,
            )
            section += attribute_section

            attribute_admonition_node = self._make_admonition()
            attribute_admonition_node += self._make_paragraph(
                [
                    nodes.Text("To use this attribute in a sensor.yaml file, use: "),
                    nodes.literal(text=key),
                ]
            )
            attribute_admonition_node += self._make_paragraph(
                [
                    nodes.Text("To use this attribute in C/C++, use: "),
                    nodes.literal(text="SENSOR_ATTR_" + key.upper()),
                ]
            )
            attribute_section += attribute_admonition_node
        return section

    def _get_channels_section(
        self, channels: Dict[str, pw_sensor.ChannelSpec]
    ) -> nodes.Node:
        keys: List[str] = list(channels.keys())
        keys.sort()

        section = self._create_section(title="Supported Channels")

        for key in keys:
            channel: pw_sensor.ChannelSpec = channels[key]
            channel_section = self._create_section(
                title=channel.name,
                content=channel.description,
                id_string_override='sensor_chan_' + key,
            )
            section += channel_section

            channel_admonition_node = self._make_admonition()
            channel_admonition_node += self._make_paragraph(
                parts=[
                    nodes.Text("To use this channel in a sensor.yaml file, use: "),
                    nodes.literal(text=key),
                ]
            )
            channel_admonition_node += self._make_paragraph(
                parts=[
                    nodes.Text("To use this channel in C/C++, use: "),
                    nodes.literal(text="SENSOR_CHAN_" + key.upper()),
                ]
            )
            channel_section += channel_admonition_node
        return section

    def _get_triggers_section(
        self, triggers: Dict[str, pw_sensor.TriggerSpec]
    ) -> nodes.Node:
        keys: List[str] = list(triggers.keys())
        keys.sort()

        section = self._create_section(title="Supported Triggers")

        for key in keys:
            trigger: pw_sensor.TriggerSpec = triggers[key]
            trigger_section = self._create_section(
                title=trigger.name,
                content=trigger.description,
                id_string_override=key,
            )
            section += trigger_section

            trigger_admonition_node = self._make_admonition()
            trigger_admonition_node += self._make_paragraph(
                [
                    nodes.Text("To use this trigger in a sensor.yaml file, use: "),
                    nodes.literal(text=key),
                ]
            )
            trigger_admonition_node += self._make_paragraph(
                [
                    nodes.Text("To use this trigger in C/C++, use: "),
                    nodes.literal(text="SENSOR_TRIG_" + key.upper()),
                ]
            )
            trigger_section += trigger_admonition_node

        return section

    def _get_units_section(self, units: Dict[str, pw_sensor.UnitsSpec]) -> nodes.Node:
        keys: List[str] = list(units.keys())
        keys.sort()

        section = self._create_section(title="Supported Units")

        for key in keys:
            unit: pw_sensor.UnitsSpec = units[key]
            unit_section = self._create_section(
                title=unit.name,
                content=unit.description,
                id_string_override=key,
            )
            section += unit_section

            unit_admonition_node = self._make_admonition()
            unit_admonition_node += self._make_paragraph(
                [
                    nodes.Text("This unit is measured in "),
                    nodes.math(text=unit.symbol),
                ]
            )
            unit_admonition_node += self._make_paragraph(
                [
                    nodes.Text("To use this unit in a sensor.yaml file, use: "),
                    nodes.literal(text=key),
                ]
            )
            unit_section += unit_admonition_node

        return section

    option_spec = {
        "show-drivers": directives.flag,
        "show-channels": directives.flag,
        "show-attributes": directives.flag,
        "show-triggers": directives.flag,
        "show-units": directives.flag,
    }

    def run(self) -> Sequence[nodes.Node]:
        sensor_yaml = self._get_all_sensors()
        spec: pw_sensor.InputSpec = pw_sensor.create_dataclass_from_dict(
            pw_sensor.InputSpec, sensor_yaml
        )

        output: List[nodes.Node] = []
        if "show-drivers" in self.options:
            output.append(
                nodes.raw(
                    "",
                    _get_filter_container_string(spec=spec),
                    format="html",
                )
            )
            output.append(
                self._get_sensors_section(sensors=list(spec.sensors.values()))
            )
        if "show-attributes" in self.options:
            output.append(self._get_attributes_section(attributes=spec.attributes))
        if "show-channels" in self.options:
            output.append(self._get_channels_section(channels=spec.channels))
        if "show-units" in self.options:
            output.append(self._get_units_section(units=spec.units))
        if "show-triggers" in self.options:
            output.append(self._get_triggers_section(triggers=spec.triggers))

        return output


def setup(app: Sphinx) -> Dict[str, Any]:
    """Introduce the sensor-driver-list directive"""
    app.add_directive(
        "sensor-driver-list",
        SensorDriverListDirective,
        override=True,
    )

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
