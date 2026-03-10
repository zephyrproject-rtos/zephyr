/**
 * Copyright (c) 2024-2025, The Linux Foundation.
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

  // Restore visibility toggles from URL
  ["show-boards", "show-shields"].forEach(toggle => {
    if (hashParams.has(toggle)) {
      document.getElementById(toggle).checked = hashParams.get(toggle) === "true";
    }
  });

  // Restore supported features from URL
  if (hashParams.has("features")) {
    const features = hashParams.get("features").split(",");
    setTimeout(() => {
      features.forEach(feature => {
        const tagContainer = document.getElementById('hwcaps-tags');
        const tagInput = document.getElementById('hwcaps-input');

        const tagElement = document.createElement('span');
        tagElement.classList.add('tag');
        tagElement.textContent = feature;
        tagElement.onclick = () => {
          tagElement.remove();
          filterBoards();
        };
        tagContainer.insertBefore(tagElement, tagInput);
      });
      filterBoards();
    }, 0);
  }

  // Restore compatibles from URL
  if (hashParams.has("compatibles")) {
    const compatibles = hashParams.get("compatibles").split("|");
    setTimeout(() => {
      compatibles.forEach(compatible => {
        const tagContainer = document.getElementById('compatibles-tags');
        const tagInput = document.getElementById('compatibles-input');

        const tagElement = document.createElement('span');
        tagElement.classList.add('tag');
        tagElement.textContent = compatible;
        tagElement.onclick = () => {
          tagElement.remove();
          filterBoards();
        };
        tagContainer.insertBefore(tagElement, tagInput);
      });
      filterBoards();
    }, 0);
  }

  filterBoards();
}

function updateURL() {
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

  ["show-boards", "show-shields"].forEach(toggle => {
    const isChecked = document.getElementById(toggle).checked;
    isChecked ? hashParams.delete(toggle) : hashParams.set(toggle, "false");
  });

  // Add supported features to URL
  const selectedHWTags = [...document.querySelectorAll('#hwcaps-tags .tag')].map(tag => tag.textContent);
  selectedHWTags.length ? hashParams.set("features", selectedHWTags.join(",")) : hashParams.delete("features");

  // Add compatibles to URL
  const selectedCompatibles = [...document.querySelectorAll('#compatibles-tags .tag')].map(tag => tag.textContent);
  selectedCompatibles.length ? hashParams.set("compatibles", selectedCompatibles.join("|")) : hashParams.delete("compatibles");

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

  const tagContainer = document.getElementById('hwcaps-tags');
  const tagInput = document.getElementById('hwcaps-input');
  const datalist = document.getElementById('tag-list');

  const tagCounts = Array.from(document.querySelectorAll('.board-card')).reduce((acc, board) => {
    (board.getAttribute('data-supported-features') || '').split(' ').forEach(tag => {
      acc[tag] = (acc[tag] || 0) + 1;
    });
    return acc;
  }, {});

  const allTags = Object.keys(tagCounts).sort();

  function addTag(tag) {
    if (selectedTags.includes(tag) || tag === "" || !allTags.includes(tag)) return;
    selectedTags.push(tag);

    const tagElement = document.createElement('span');
    tagElement.classList.add('tag');
    tagElement.textContent = tag;
    tagElement.onclick = () => removeTag(tag);
    tagContainer.insertBefore(tagElement, tagInput);

    tagInput.value = '';
    updateDatalist();
  }

  function removeTag(tag) {
    selectedTags = selectedTags.filter(t => t !== tag);
    document.querySelectorAll('.tag').forEach(el => {
      if (el.textContent.includes(tag)) el.remove();
    });
    updateDatalist();
  }

  function updateDatalist() {
    datalist.innerHTML = '';
    const filteredTags = allTags.filter(tag => !selectedTags.includes(tag));

    filteredTags.forEach(tag => {
      const option = document.createElement('option');
      option.value = tag;
      datalist.appendChild(option);
    });

    filterBoards();
  }

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

  updateDatalist();
}

function setupCompatiblesField() {
  let selectedCompatibles = [];

  const tagContainer = document.getElementById('compatibles-tags');
  const tagInput = document.getElementById('compatibles-input');
  const datalist = document.getElementById('compatibles-list');

  // Collect all unique compatibles from boards
  const allCompatibles = Array.from(document.querySelectorAll('.board-card')).reduce((acc, board) => {
    (board.getAttribute('data-compatibles') || '').split(' ').forEach(compat => {
      if (compat && !acc.includes(compat)) {
        acc.push(compat);
      }
    });
    return acc;
  }, []);

  allCompatibles.sort();

  function addCompatible(compatible) {
    if (selectedCompatibles.includes(compatible) || compatible === "") return;
    selectedCompatibles.push(compatible);

    const tagElement = document.createElement('span');
    tagElement.classList.add('tag');
    tagElement.textContent = compatible;
    tagElement.onclick = () => removeCompatible(compatible);
    tagContainer.insertBefore(tagElement, tagInput);

    tagInput.value = '';
    updateDatalist();
  }

  function removeCompatible(compatible) {
    selectedCompatibles = selectedCompatibles.filter(c => c !== compatible);
    document.querySelectorAll('.tag').forEach(el => {
      if (el.textContent === compatible && el.parentElement === tagContainer) {
        el.remove();
      }
    });
    updateDatalist();
  }

  function updateDatalist() {
    datalist.innerHTML = '';
    const filteredCompatibles = allCompatibles.filter(c => !selectedCompatibles.includes(c));

    filteredCompatibles.forEach(compatible => {
      const option = document.createElement('option');
      option.value = compatible;
      datalist.appendChild(option);
    });

    filterBoards();
  }

  tagInput.addEventListener('input', () => {
    if (allCompatibles.includes(tagInput.value)) {
      addCompatible(tagInput.value);
    }
  });

  tagInput.addEventListener('keydown', (e) => {
    if (e.key === 'Enter' && tagInput.value) {
      addCompatible(tagInput.value);
      e.preventDefault();
    } else if (e.key === 'Backspace' && tagInput.value === '' && selectedCompatibles.length > 0) {
      removeCompatible(selectedCompatibles[selectedCompatibles.length - 1]);
    }
  });

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
  setupCompatiblesField();

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

  boardsToggle = document.getElementById("show-boards");
  boardsToggle.addEventListener("change", () => {
    filterBoards();
  });

  shieldsToggle = document.getElementById("show-shields");
  shieldsToggle.addEventListener("change", () => {
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

  document.getElementById("show-boards").checked = true;
  document.getElementById("show-shields").checked = true;

  // Clear supported features
  document.querySelectorAll('#hwcaps-tags .tag').forEach(tag => tag.remove());
  document.getElementById('hwcaps-input').value = '';

  // Clear compatibles
  document.querySelectorAll('#compatibles-tags .tag').forEach(tag => tag.remove());
  document.getElementById('compatibles-input').value = '';

  filterBoards();
}

function updateBoardCount() {
  const boards = Array.from(document.getElementsByClassName("board-card"));
  const visible = boards.filter(board => !board.classList.contains("hidden"));
  const shields = boards.filter(board => board.classList.contains("shield"));
  const visibleShields = visible.filter(board => board.classList.contains("shield"));

  document.getElementById("nb-matches").textContent =
    `Showing ${visible.length - visibleShields.length} of ${boards.length - shields.length} boards,`
    + ` ${visibleShields.length} of ${shields.length} shields`;
}

function wildcardMatch(pattern, str) {
  // Convert wildcard pattern to regex
  // Escape special regex characters except *
  const regexPattern = pattern
    .replace(/[.+?^${}()|[\]\\]/g, '\\$&')
    .replace(/\*/g, '.*');
  const regex = new RegExp(`^${regexPattern}$`, "i");
  return regex.test(str);
}

function filterBoards() {
  const nameInput = document.getElementById("name").value.toLowerCase();
  const archSelect = document.getElementById("arch").value;
  const vendorSelect = document.getElementById("vendor").value;
  const socSocSelect = document.getElementById("soc");
  const showBoards = document.getElementById("show-boards").checked;
  const showShields = document.getElementById("show-shields").checked;

  // Get selected hardware capability tags
  const selectedHWTags = [...document.querySelectorAll('#hwcaps-tags .tag')].map(tag => tag.textContent);

  // Get selected compatible tags
  const selectedCompatibles = [...document.querySelectorAll('#compatibles-tags .tag')].map(tag => tag.textContent);

  const resetFiltersBtn = document.getElementById("reset-filters");
  if (nameInput || archSelect || vendorSelect || socSocSelect.selectedOptions.length || selectedHWTags.length || selectedCompatibles.length || !showBoards || !showShields) {
    resetFiltersBtn.classList.remove("btn-disabled");
  } else {
    resetFiltersBtn.classList.add("btn-disabled");
  }

  const boards = document.getElementsByClassName("board-card");

  Array.from(boards).forEach(function (board) {
    const boardName = board.getAttribute("data-name").toLowerCase();
    const boardArchs = (board.getAttribute("data-arch") || "").split(" ").filter(Boolean);
    const boardVendor = board.getAttribute("data-vendor") || "";
    const boardSocs = (board.getAttribute("data-socs") || "").split(" ").filter(Boolean);
    const boardSupportedFeatures = (board.getAttribute("data-supported-features") || "").split(" ").filter(Boolean);
    const boardCompatibles = (board.getAttribute("data-compatibles") || "").split(" ").filter(Boolean);
    const isShield = board.classList.contains("shield");

    let matches = true;

    const selectedSocs = [...socSocSelect.selectedOptions].map(({ value }) => value);

    if ((isShield && !showShields) || (!isShield && !showBoards)) {
      matches = false;
    } else {
      // Check if board matches all selected compatibles (with wildcard support)
      const compatiblesMatch = selectedCompatibles.length === 0 ||
        selectedCompatibles.every((pattern) =>
          boardCompatibles.some((compatible) => wildcardMatch(pattern, compatible))
        );

      matches =
        !(nameInput && !boardName.includes(nameInput)) &&
        !(archSelect && !boardArchs.includes(archSelect)) &&
        !(vendorSelect && boardVendor !== vendorSelect) &&
        (selectedSocs.length === 0 || selectedSocs.some((soc) => boardSocs.includes(soc))) &&
        (selectedHWTags.length === 0 || selectedHWTags.every((tag) => boardSupportedFeatures.includes(tag))) &&
        compatiblesMatch;
    }

    board.classList.toggle("hidden", !matches);
  });

  updateURL();
  updateBoardCount();
}
