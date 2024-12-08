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

    let matches = true;

    const selectedSocs = [...socSocSelect.selectedOptions].map(({ value }) => value);

    matches =
      !(nameInput && !boardName.includes(nameInput)) &&
      !(archSelect && !boardArchs.includes(archSelect)) &&
      !(vendorSelect && boardVendor !== vendorSelect) &&
      (selectedSocs.length === 0 || selectedSocs.some((soc) => boardSocs.includes(soc)));

    board.classList.toggle("hidden", !matches);
  });

  updateURL();
  updateBoardCount();
}
