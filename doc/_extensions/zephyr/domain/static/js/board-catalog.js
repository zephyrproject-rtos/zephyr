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

  populateFormFromURL();

  form.addEventListener("submit", function (event) {
    event.preventDefault();
  });

  form.addEventListener("input", function () {
    filterBoards();
    updateURL();
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

    let matches = true;

    matches =
      !(nameInput && !boardName.includes(nameInput)) &&
      !(archSelect && !boardArchs.includes(archSelect)) &&
      !(vendorSelect && boardVendor !== vendorSelect);

    if (matches) {
      board.classList.remove("hidden");
    } else {
      board.classList.add("hidden");
    }
  });

  updateBoardCount();
}
