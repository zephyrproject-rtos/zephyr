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
  const params = ["name", "arch", "vendor"];
  const hashParams = new URLSearchParams(window.location.hash.slice(1));
  params.forEach((param) => {
    const element = document.getElementById(param);
    if (hashParams.has(param)) {
      element.value = hashParams.get(param);
    }
  });

  filterBoards();
}

function updateURL() {
  const params = ["name", "arch", "vendor"];
  const hashParams = new URLSearchParams(window.location.hash.slice(1));

  params.forEach((param) => {
    const value = document.getElementById(param).value;
    value ? hashParams.set(param, value) : hashParams.delete(param);
  });

  window.history.replaceState({}, "", `#${hashParams.toString()}`);
}

// Populate Family listbox
function populateFamilies() {
  const familySelect = document.getElementById('family');
  const seriesSelect = document.getElementById('series');
  const socSelect = document.getElementById('soc');

  familySelect.innerHTML = ''; // Clear any previous options
  for (const family in socs_data) {
    let option = document.createElement('option');
    option.value = family;
    option.text = family !== '<no family>' ? family : '(No Family)';
    familySelect.appendChild(option);
  }
  populateSeries();  // Populate series and socs initially
  populateSocs();
}

// Populate Series listbox
function populateSeries() {
  const familySelect = document.getElementById('family');
  const seriesSelect = document.getElementById('series');
  const socSelect = document.getElementById('soc');


  seriesSelect.innerHTML = ''; // Clear any previous options

  const selectedFamilies = Array.from(familySelect.selectedOptions).map(option => option.value);
  let seriesList = [];

  if (selectedFamilies.length === 0) {
    // Show all series if no family is selected
    for (const family in socs_data) {
      const familySeries = socs_data[family];
      for (const series in familySeries) {
        if (!seriesList.includes(series)) {
          seriesList.push(series);
        }
      }
    }
  } else {
    // Filter series based on selected families
    selectedFamilies.forEach(family => {
      const familySeries = socs_data[family];
      for (const series in familySeries) {
        if (!seriesList.includes(series)) {
          seriesList.push(series);
        }
      }
    });
  }

  seriesList.forEach(series => {
    let option = document.createElement('option');
    option.value = series;
    option.text = series !== '<no series>' ? series : '(No Series)';
    seriesSelect.appendChild(option);
  });
  populateSocs();  // Select SoCs automatically when series changes
}

// Populate SoC listbox
function populateSocs() {
  const familySelect = document.getElementById('family');
  const seriesSelect = document.getElementById('series');
  const socSelect = document.getElementById('soc');

  socSelect.innerHTML = ''; // Clear previous options

  const selectedFamilies = Array.from(familySelect.selectedOptions).map(option => option.value);
  const selectedSeries = Array.from(seriesSelect.selectedOptions).map(option => option.value);

  let socList = [];

  if (selectedFamilies.length === 0 && selectedSeries.length === 0) {
    // Show all SoCs if no family or series is selected
    for (const family in socs_data) {
      for (const series in socs_data[family]) {
        socs_data[family][series].forEach(soc => {
          if (!socList.includes(soc)) {
            socList.push(soc);
          }
        });
      }
    }
  } else if (selectedSeries.length > 0) {
    // Filter SoCs based on selected series
    selectedSeries.forEach(series => {
      for (const family in socs_data) {
        if (socs_data[family][series]) {
          socs_data[family][series].forEach(soc => {
            if (!socList.includes(soc)) {
              socList.push(soc);
            }
          });
        }
      }
    });
  } else {
    // Filter SoCs based on selected families if no series is selected
    selectedFamilies.forEach(family => {
      for (const series in socs_data[family]) {
        socs_data[family][series].forEach(soc => {
          if (!socList.includes(soc)) {
            socList.push(soc);
          }
        });
      }
    });
  }

  socList.sort((a, b) => a.localeCompare(b)).forEach(soc => {
    let option = document.createElement('option');
    option.value = soc;
    option.text = soc;
    socSelect.appendChild(option);
  });

  // Automatically select all SoCs that match the selected family/series
  Array.from(socSelect.options).forEach(option => {
    option.selected = true;
  });
}


document.addEventListener("DOMContentLoaded", function () {
  updateBoardCount();
  populateFormFromURL();
  populateFamilies();

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

  familySelect = document.getElementById('family');
  seriesSelect = document.getElementById('series');
  socSelect = document.getElementById('soc');

  // Handle changes for the listboxes
  familySelect.addEventListener('change', function () {
    populateSeries();
    populateSocs(); // Update SoCs based on family/series changes
    updateURL();    // Sync URL with new selections
    filterBoards(); // Filter boards immediately after listbox change
  });

  seriesSelect.addEventListener('change', function () {
    populateSocs(); // Update SoCs when series changes
    updateURL();    // Sync URL with new selections
    filterBoards(); // Filter boards immediately after listbox change
  });

  socSelect.addEventListener('change', function () {
    updateURL();    // Sync URL with new selections
    filterBoards(); // Filter boards immediately after listbox change
  });



  form.addEventListener("submit", function (event) {
    event.preventDefault();
  });

  form.addEventListener("input", function (event) {
    const target = event.target;

    if (!["family", "series", "soc"].includes(target.id)) {
      filterBoards();
      updateURL();
    }
  });
});

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

  const selectedSocs = Array.from(document.getElementById("soc").selectedOptions).map(option => option.value);

  const resetFiltersBtn = document.getElementById("reset-filters");
  if (nameInput || archSelect || vendorSelect) {
    resetFiltersBtn.classList.remove("btn-disabled");
  } else {
    resetFiltersBtn.classList.add("btn-disabled");
  }

  const boards = document.getElementsByClassName("board-card");

  Array.from(boards).forEach(function (board) {
    const boardName = board.getAttribute("data-name").toLowerCase();
    const boardArchs = board.getAttribute("data-arch").split(" ");
    const boardVendor = board.getAttribute("data-vendor");
    const boardSocs = board.getAttribute("data-socs").split(" "); // Multiple SoCs for the board

    let matches = true;

    matches =
      !(nameInput && !boardName.includes(nameInput)) &&
      !(archSelect && !boardArchs.includes(archSelect)) &&
      !(vendorSelect && boardVendor !== vendorSelect) &&
      !(selectedSocs.length > 0 && !selectedSocs.some(soc => boardSocs.includes(soc)));

    if (matches) {
      board.classList.remove("hidden");
    } else {
      board.classList.add("hidden");
    }
  });

  updateBoardCount();
}
