/*
 * Copyright (c) 2024 Google Inc
 * SPDX-License-Identifier: Apache-2.0
 */

function getFilterAttributeList() {
    return document.querySelector(".dropdown-filter-content #attribute-channel-lists #sensor-filter-attribute-list");
}

function getFilterChannelList() {
    return document.querySelector(".dropdown-filter-content #attribute-channel-lists #sensor-filter-channel-list");
}

function adjustDropdownWidth() {
    const dropdownContent = document.querySelector(".dropdown-filter-attributes .dropdown-filter-content");

    const attributeListWidth = getFilterAttributeList().offsetWidth;
    const channelListWidth = getFilterChannelList().offsetWidth;

    console.log(`attributeList.width=${attributeListWidth}, channelList.width=${channelListWidth}`);
    dropdownContent.style.width = Math.max(attributeListWidth, channelListWidth) * 2 + 20 + 'px';
    console.log(`Calculated dropdownContent.style.width=${dropdownContent.style.width}`);
}

class SensorFilters {
    constructor(compat_string, channels, attributes, triggers, buses) {
        this.compat_string = compat_string;
        this.channels = channels;
        this.attributes = attributes;
        this.triggers = triggers;
        this.buses = buses;
    }

    isSubsetOf(otherFilters) {
        // Check if this.channels is a subset of otherFilters.channels
        if (!this.channels.every(channel => otherFilters.channels.includes(channel))) {
            console.log("Some channel filters don't match");
            return false;
        }

        // Check if this.triggers is a subset of otherFilters.triggers
        if (!this.triggers.every(trigger => otherFilters.triggers.includes(trigger))) {
            console.log("Some trigger filters don't match");
            return false;
        }

        // Check if this.buses is a subset of otherFilters.buses
        if (!this.buses.every(bus => otherFilters.buses.includes(bus))) {
            console.log("Some bus filters don't match");
            return false;
        }

        // Check if this.attributes is a subset of otherFilters.attributes
        if (!this.attributes.every(attr => {
            return otherFilters.attributes.some(otherAttr => {
                return attr.attribute === otherAttr.attribute && attr.channel === otherAttr.channel;
            });
        })) {
            console.log("Some attribute filters don't match");
            return false;
        }

        // Check if this.compat_string is a subset of otherFilters.compat_string
        if (this.compat_string && !otherFilters.compat_string.includes(this.compat_string)) {
            console.log("Compat string doesn't match");
            return false;
        }

        // If all checks pass, this is a subset
        return true;
    }
}

function appendAttributeFilter(attributeName, channelName) {
    const chipsContainer = document.querySelector("#attribute-channel-active-filters");
    const chip = document.createElement('button');
    chip.classList.add('chip', 'btn', 'btn-info');
    chip.innerHTML = `${attributeName}/${channelName}`;

    chipsContainer.appendChild(chip);
    chip.addEventListener('click', () => {
        chipsContainer.removeChild(chip);
        filterSensorCards();
    });
}

function setFiltersFromUrl() {
    const queryParams = new URLSearchParams(window.location.search.slice(1));
    const filters = new SensorFilters("", [], [], [], []);

    for (let [key, value] of queryParams) {
        const match = key.match(/^(.+)\[(\d*)\](?:\[(.+)\])?$/);
        if (match) {
            const [_, name, index, subKey] = match;
            if (index) {
                if (!filters[name][index]) {
                    filters[name][index] = subKey ? {} : [];
                }
                if (subKey) {
                    filters[name][index][subKey] = value;
                } else {
                    filters[name][index] = value;
                }
            } else {
                filters[name].push(value);
            }
        } else if (key === "compat_string") {
            filters.compat_string = value;
        }
    }

    console.log(filters);

    document.querySelector('input#compat').value = filters.compat_string;

    document.querySelectorAll('input[type="checkbox"][name="channels[]"]').forEach(node => {
        node.checked = filters.channels.includes(node.value);
    });

    document.querySelectorAll('input[type="checkbox"][name="triggers[]"]').forEach(node => {
        node.checked = filters.triggers.includes(node.value);
    });

    document.querySelectorAll('input[type="checkbox"][name="buses[]"]').forEach(node => {
        node.checked = filters.triggers.includes(node.value);
    });

    filters.attributes.forEach(attr => {
        appendAttributeFilter(attr.attribute, attr.channel);
    });

    filterSensorCards();
}

function getCurrentFilters() {
    const compatibleFilter = document.querySelector('input#compat').value;

    const selectedChannelNodes = document.querySelectorAll('input[type="checkbox"][name="channels[]"]:checked');
    const selectedChannels = Array.from(selectedChannelNodes).map(node => node.value);

    const selectedAttributeNodes = document.querySelectorAll('#attribute-channel-active-filters button');
    const selectedAttributes = Array.from(selectedAttributeNodes).map(node => {
        const parts = node.innerText.split('/');
        return {
            attribute: parts[0],
            channel: parts[1],
        };
    });

    const selectedTriggerNodes = document.querySelectorAll('input[type="checkbox"][name="triggers[]"]:checked');
    const selectedTriggers = Array.from(selectedTriggerNodes).map(node => node.value);

    const selectedBusNodes = document.querySelectorAll('input[type="checkbox"][name="buses[]"]:checked');
    const selectedBuses = Array.from(selectedBusNodes).map(node => node.value);

    return new SensorFilters(compatibleFilter, selectedChannels, selectedAttributes, selectedTriggers, selectedBuses);
}

function getFiltersUrl() {
    const params = new URLSearchParams();
    const filters = getCurrentFilters();

    // Helper function to add parameters recursively
    function addParams(name, value) {
        if (Array.isArray(value)) {
            value.forEach((item, index) => {
                addParams(`${name}[${index}]`, item);
            });
        } else if (typeof value === 'object') {
            for (let key in value) {
                addParams(`${name}[${key}]`, value[key]);
            }
        } else {
            params.append(name, value);
        }
    }

    // Add parameters for each filter property
    addParams('channels', filters.channels);
    addParams('attributes', filters.attributes);
    addParams('triggers', filters.triggers);
    addParams('buses', filters.buses);
    params.append('compat_string', filters.compat_string);

    return params.toString();
}

function getFiltersForSensorCard(card) {
    const compatible = card.dataset.compatible || "";
    const channels = card.dataset.channels ? card.dataset.channels.split(';') : [];
    const triggers = card.dataset.triggers ? card.dataset.triggers.split(';') : [];
    const buses = card.dataset.buses ? card.dataset.buses.split(';') : [];
    const attributes = card.dataset.attributes
        ? card.dataset.attributes.split(';').map(attr => {
            const [attribute, channel] = attr.split('/');
            return { attribute, channel };
        })
        : [];

    return new SensorFilters(compatible, channels, attributes, triggers, buses);
}

function filterSensorCards() {
    const filters = getCurrentFilters();

    console.log(filters);

    document.querySelectorAll(".sensor-card").forEach(sensorCard => {
        const cardFilters = getFiltersForSensorCard(sensorCard);
        console.log(`${sensorCard.dataset.compatible}: ${JSON.stringify(cardFilters)}`);
        if (filters.isSubsetOf(cardFilters)) {
            // Make card visible
            sensorCard.style.display = "flex";
        } else {
            // Make card hidden
            sensorCard.style.display = "none";
        }
    });

    window.history.replaceState({}, "", `?${getFiltersUrl()}`)
}

function initCatalogFilters() {
    if (typeof attributeChannelFilters === "undefined") {
        return;
    }
    console.log(attributeChannelFilters);
    const filter_attributes_div = getFilterAttributeList();
    const filter_channels_div = getFilterChannelList();
    const add_button = document.querySelector("#add-attribute-filter");
    for (const attribute in attributeChannelFilters) {
        let label = document.createElement('label');
        label.innerHTML = `<input type="radio" name="attribute" value="${attribute}"> ${attribute}`;
        filter_attributes_div.appendChild(label);
    }

    filter_attributes_div.addEventListener('change', (event) => {
        const selectedAttribute = event.target.value;
        const chipsContainer = document.querySelector("#attribute-channel-active-filters");
        filter_channels_div.innerHTML = '';

        for (const channel of attributeChannelFilters[selectedAttribute]) {
            let filterText = `${selectedAttribute}/${channel}`;
            let filterExists = false;
            chipsContainer.querySelectorAll(".chip").forEach(chip => {
                if (chip.textContent.trim() == filterText) {
                    filterExists = true;
                }
            });

            if (filterExists) {
                continue;
            }

            let label = document.createElement('label');
            label.innerHTML = `<input type="radio" name="channel" value="${channel}"> ${channel}`;
            filter_channels_div.appendChild(label);
        }

        adjustDropdownWidth();
    });

    filter_channels_div.addEventListener('change', () => {
        add_button.disabled = false;
    })

    add_button.addEventListener('click', () => {
        const selectedAttribute = document.querySelector('#sensor-filter-attribute-list input[name="attribute"]:checked').value;
        const selectedChannel = document.querySelector('#sensor-filter-channel-list input[name="channel"]:checked').value;

        appendAttributeFilter(selectedAttribute, selectedChannel);

        document.querySelector('#sensor-filter-attribute-list input[name="attribute"]:checked').checked = false;
        document.querySelector('#sensor-filter-channel-list input[name="channel"]:checked').checked = false;
        getFilterChannelList().innerHTML = '';
        add_button.disabled = true;
        filterSensorCards();
    });

    document.querySelector("input#compat")
        .addEventListener("input", filterSensorCards);
    document.querySelectorAll(".dropdown-filter-content input[type='checkbox']")
        .forEach(node => node.addEventListener("change", filterSensorCards));

        adjustDropdownWidth();
        setFiltersFromUrl();
}

document.addEventListener('DOMContentLoaded', () => {
    console.log("Content is loaded...");
    initCatalogFilters();

    document.querySelectorAll('.sensor-summary-card-content button').forEach(button => {
        button.addEventListener('click', () => { window.location.href = button.getAttribute('href'); });
    });
});

window.addEventListener('resize', adjustDropdownWidth);