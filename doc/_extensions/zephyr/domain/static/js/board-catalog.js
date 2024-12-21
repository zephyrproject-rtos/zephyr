/**
 * Copyright (c) 2024, The Linux Foundation.
 * SPDX-License-Identifier: Apache-2.0
 */

function toggleDisplayMode(btn) {
  const catalog = document.getElementById("catalog");
  catalog.classList.toggle("compact");
  btn.classList.toggle("fa-bars");
  btn.classList.toggle("fa-th");
  btn.textContent = catalog.classList.contains("compact")
    ? " Switch to Card View"
    : " Switch to Compact View";
}

function populateFormFromURL() {
  // TODO restore supported_features
  const params = ["name", "arch", "vendor", "soc"];
  const hashParams = new URLSearchParams(window.location.hash.slice(1));
  params.forEach((param) => {
    const element = document.getElementById(param);
    if (hashParams.has(param)) {
      const value = hashParams.get(param);
      if (param === "soc") {
        value.split(",").forEach(soc =>
          element.querySelector(`option[value="${soc}"]`).selected = true);
      } else {
        element.value = value;
      }
    }
  });

  filterBoards();
}

function updateURL() {
  // TODO add backup of supported_features
  const params = ["name", "arch", "vendor", "soc"];
  const hashParams = new URLSearchParams(window.location.hash.slice(1));

  params.forEach((param) => {
    const element = document.getElementById(param);
    if (param === "soc") {
      const selectedSocs = [...element.selectedOptions].map(({ value }) => value);
      selectedSocs.length ? hashParams.set(param, selectedSocs.join(",")) : hashParams.delete(param);
    }
    else {
      element.value ? hashParams.set(param, element.value) : hashParams.delete(param);
    }
  });

  window.history.replaceState({}, "", `#${hashParams.toString()}`);
}

function fillSocFamilySelect() {
  const socFamilySelect = document.getElementById("family");

  Object.keys(socs_data).sort().forEach(f => {
    socFamilySelect.add(new Option(f));
  });
}

function fillSocSeriesSelect(families, selectOnFill = false) {
  const socSeriesSelect = document.getElementById("series");

  families = families?.length ? families : Object.keys(socs_data);
  let allSeries = [...new Set(families.flatMap(f => Object.keys(socs_data[f])))];

  socSeriesSelect.innerHTML = "";
  allSeries.sort().map(s => {
    const option = new Option(s, s, selectOnFill, selectOnFill);
    socSeriesSelect.add(option);
  });
}

function fillSocSocSelect(families, series = undefined, selectOnFill = false) {
  const socSocSelect = document.getElementById("soc");

  families = families?.length ? families : Object.keys(socs_data);
  series = series?.length ? series : families.flatMap(f => Object.keys(socs_data[f]));
  matchingSocs = [...new Set(families.flatMap(f => series.flatMap(s => socs_data[f][s] || [])))];

  socSocSelect.innerHTML = "";
  matchingSocs.sort().forEach((soc) => {
    socSocSelect.add(new Option(soc, soc, selectOnFill, selectOnFill));
  });
}

function setupHWCapabilitiesField() {
  let selectedTags = [];

  const tagContainer = document.getElementById('tag-container');
  const tagInput = document.getElementById('tag-input');
  const datalist = document.getElementById('tag-list');

  const tagCounts = Array.from(document.querySelectorAll('.board-card')).reduce((acc, board) => {
    board.getAttribute('data-supported-features').split(' ').forEach(tag => {
      acc[tag] = (acc[tag] || 0) + 1;
    });
    return acc;
  }, {});

  const allTags = Object.entries(tagCounts)
    .sort((a, b) => b[1] - a[1])
    .map(entry => entry[0]);

  // Add tag
  function addTag(tag) {
    if (selectedTags.includes(tag) || tag === "" || !allTags.includes(tag)) return;
    selectedTags.push(tag);

    const tagElement = document.createElement('span');
    tagElement.classList.add('tag');
    tagElement.textContent = tag;
    tagElement.onclick = () => removeTag(tag);
    tagContainer.insertBefore(tagElement, tagInput);

    tagInput.value = ''; // Clear input
    updateDatalist(); // Update the available suggestions
  }

  // Remove tag
  function removeTag(tag) {
    selectedTags = selectedTags.filter(t => t !== tag);
    document.querySelectorAll('.tag').forEach(el => {
      if (el.textContent.includes(tag)) el.remove();
    });
    updateDatalist(); // Update the suggestions when a tag is removed
  }

  // Update the datalist options based on selected tags
  function updateDatalist() {
    datalist.innerHTML = ''; // Clear existing options
    const filteredTags = allTags.filter(tag => !selectedTags.includes(tag)); // Filter out selected tags

    filteredTags.forEach(tag => {
      const option = document.createElement('option');
      option.value = tag;
      datalist.appendChild(option);
    });

    filterBoards();
  }

  // Handle selection from datalist or Enter key to add tag
  tagInput.addEventListener('input', () => {
    if (allTags.includes(tagInput.value)) {
      addTag(tagInput.value);
    }
  });

  // Add tag when pressing the Enter key
  tagInput.addEventListener('keydown', (e) => {
    if (e.key === 'Enter' && allTags.includes(tagInput.value)) {
      addTag(tagInput.value);
      e.preventDefault();
    }
  });

  // Delete tag when pressing the Backspace key
  tagInput.addEventListener('keydown', (e) => {
    if (e.key === 'Backspace' && tagInput.value === '' && selectedTags.length > 0) {
      removeTag(selectedTags[selectedTags.length - 1]);
    }
  });

  // Initialize the datalist
  updateDatalist();
}

document.addEventListener("DOMContentLoaded", function () {
  const form = document.querySelector(".filter-form");

  // sort vendors alphabetically
  vendorSelect = document.getElementById("vendor");
  vendorOptions = Array.from(vendorSelect.options).slice(1);
  vendorOptions.sort((a, b) => a.text.localeCompare(b.text));
  while (vendorSelect.options.length > 1) {
    vendorSelect.remove(1);
  }
  vendorOptions.forEach((option) => {
    vendorSelect.appendChild(option);
  });

  fillSocFamilySelect();
  fillSocSeriesSelect();
  fillSocSocSelect();
  populateFormFromURL();

  setupHWCapabilitiesField();

  socFamilySelect = document.getElementById("family");
  socFamilySelect.addEventListener("change", () => {
    const selectedFamilies = [...socFamilySelect.selectedOptions].map(({ value }) => value);
    fillSocSeriesSelect(selectedFamilies, true);
    fillSocSocSelect(selectedFamilies, undefined, true);
    filterBoards();
  });

  socSeriesSelect = document.getElementById("series");
  socSeriesSelect.addEventListener("change", () => {
    const selectedFamilies = [...socFamilySelect.selectedOptions].map(({ value }) => value);
    const selectedSeries = [...socSeriesSelect.selectedOptions].map(({ value }) => value);
    fillSocSocSelect(selectedFamilies, selectedSeries, true);
    filterBoards();
  });

  socSocSelect = document.getElementById("soc");
  socSocSelect.addEventListener("change", () => {
    filterBoards();
  });

  form.addEventListener("input", function () {
    filterBoards();
  });

  form.addEventListener("submit", function (event) {
    event.preventDefault();
  });

  filterBoards();
});

function resetForm() {
  const form = document.querySelector(".filter-form");
  form.reset();
  fillSocFamilySelect();
  fillSocSeriesSelect();
  fillSocSocSelect();
  // todo clear supported_features

  filterBoards();
}

function updateBoardCount() {
  const boards = document.getElementsByClassName("board-card");
  const visibleBoards = Array.from(boards).filter(
    (board) => !board.classList.contains("hidden")
  ).length;
  const totalBoards = boards.length;
  document.getElementById("nb-matches").textContent = `Showing ${visibleBoards} of ${totalBoards}`;
}

function filterBoards() {
  const nameInput = document.getElementById("name").value.toLowerCase();
  const archSelect = document.getElementById("arch").value;
  const vendorSelect = document.getElementById("vendor").value;
  const socSocSelect = document.getElementById("soc");

  const selectedTags = [...document.querySelectorAll('.tag')].map(tag => tag.textContent);

  const resetFiltersBtn = document.getElementById("reset-filters");
  if (nameInput || archSelect || vendorSelect || socSocSelect.selectedOptions.length) {
    resetFiltersBtn.classList.remove("btn-disabled");
  } else {
    resetFiltersBtn.classList.add("btn-disabled");
  }

  const boards = document.getElementsByClassName("board-card");

  Array.from(boards).forEach(function (board) {
    const boardName = board.getAttribute("data-name").toLowerCase();
    const boardArchs = board.getAttribute("data-arch").split(" ");
    const boardVendor = board.getAttribute("data-vendor");
    const boardSocs = board.getAttribute("data-socs").split(" ");
    const boardSupportedFeatures = board.getAttribute("data-supported-features").split(" ");

    let matches = true;

    const selectedSocs = [...socSocSelect.selectedOptions].map(({ value }) => value);

    matches =
      !(nameInput && !boardName.includes(nameInput)) &&
      !(archSelect && !boardArchs.includes(archSelect)) &&
      !(vendorSelect && boardVendor !== vendorSelect) &&
      (selectedSocs.length === 0 || selectedSocs.some((soc) => boardSocs.includes(soc))) &&
      (selectedTags.length === 0 || selectedTags.every((tag) => boardSupportedFeatures.includes(tag)));

    board.classList.toggle("hidden", !matches);
  });

  updateURL();
  updateBoardCount();
}
