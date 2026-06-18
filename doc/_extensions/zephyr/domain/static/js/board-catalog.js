/**
 * Copyright (c) 2024-2025, The Linux Foundation.
 * SPDX-License-Identifier: Apache-2.0
 */

var memorySliderConfigs = {};

const MEMORY_KB = 1024;
const MEMORY_MB = MEMORY_KB ** 2;
const MEMORY_GB = MEMORY_KB ** 3;
const MEMORY_DEFAULT_MAX_KIB = 64;

function parseMemoryHashParam(hashParams, key, defaultBytes) {
  if (!hashParams.has(key)) return defaultBytes;

  const kib = Number(hashParams.get(key));
  if (!Number.isFinite(kib) || kib < 0) return defaultBytes;

  return kib * MEMORY_KB;
}

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
        value
          .split(",")
          .forEach((soc) => (element.querySelector(`option[value="${soc}"]`).selected = true));
      } else {
        element.value = value;
      }
    }
  });

  // Restore memory slider positions (run after initMemorySliders builds noUiSlider)
  ["ram", "flash"].forEach((type) => {
    const root = document.getElementById(`${type}-slider-root`);
    const cfg = memorySliderConfigs[type];
    if (!root || !root.noUiSlider || !cfg) return;
    let minBytes = parseMemoryHashParam(hashParams, `${type}-min`, 0);
    let maxBytes = parseMemoryHashParam(hashParams, `${type}-max`, Infinity);
    if (minBytes > maxBytes) {
      minBytes = 0;
      maxBytes = Infinity;
    }
    root.noUiSlider.set(
      [memoryBytesToSlider(minBytes, cfg), memoryBytesToSlider(maxBytes, cfg)],
      false,
    );
    updateMemorySlider(type);
  });

  // Restore visibility toggles from URL
  ["show-boards", "show-shields"].forEach((toggle) => {
    if (hashParams.has(toggle)) {
      document.getElementById(toggle).checked = hashParams.get(toggle) === "true";
    }
  });

  // Restore supported features from URL
  if (hashParams.has("features")) {
    const features = hashParams.get("features").split(",");
    setTimeout(() => {
      features.forEach((feature) => {
        const tagContainer = document.getElementById("hwcaps-tags");
        const tagInput = document.getElementById("hwcaps-input");

        const tagElement = document.createElement("span");
        tagElement.classList.add("tag");
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
      compatibles.forEach((compatible) => {
        const tagContainer = document.getElementById("compatibles-tags");
        const tagInput = document.getElementById("compatibles-input");

        const tagElement = document.createElement("span");
        tagElement.classList.add("tag");
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

/**
 * True when the URL hash encodes catalog filters or the #catalog fragment, so the page
 * should start at the board grid rather than the introductory content.
 */
function hashIndicatesFilteredCatalogView() {
  const fields = new Set([
    "name",
    "arch",
    "vendor",
    "soc",
    "features",
    "compatibles",
    "ram-min",
    "ram-max",
    "flash-min",
    "flash-max",
  ]);
  for (const [key, value] of new URLSearchParams(window.location.hash.slice(1))) {
    if (key === "catalog") return true;
    if (fields.has(key) && value !== "") return true;
    if ((key === "show-boards" || key === "show-shields") && value === "false") return true;
  }
  return false;
}

function updateURL() {
  const params = ["name", "arch", "vendor", "soc"];
  const hashParams = new URLSearchParams(window.location.hash.slice(1));

  params.forEach((param) => {
    const element = document.getElementById(param);
    if (param === "soc") {
      const selectedSocs = [...element.selectedOptions].map(({ value }) => value);
      selectedSocs.length
        ? hashParams.set(param, selectedSocs.join(","))
        : hashParams.delete(param);
    } else {
      element.value ? hashParams.set(param, element.value) : hashParams.delete(param);
    }
  });

  // Persist memory slider positions only when non-default
  ["ram", "flash"].forEach((type) => {
    const { minBytes, maxBytes } = memorySliderRange(type);
    minBytes > 0
      ? hashParams.set(`${type}-min`, String(Math.floor(minBytes / MEMORY_KB)))
      : hashParams.delete(`${type}-min`);
    maxBytes < Infinity
      ? hashParams.set(`${type}-max`, String(Math.ceil(maxBytes / MEMORY_KB)))
      : hashParams.delete(`${type}-max`);
  });

  ["show-boards", "show-shields"].forEach((toggle) => {
    const isChecked = document.getElementById(toggle).checked;
    isChecked ? hashParams.delete(toggle) : hashParams.set(toggle, "false");
  });

  // Add supported features to URL
  const selectedHWTags = [...document.querySelectorAll("#hwcaps-tags .tag")].map(
    (tag) => tag.textContent,
  );
  selectedHWTags.length
    ? hashParams.set("features", selectedHWTags.join(","))
    : hashParams.delete("features");

  // Add compatibles to URL
  const selectedCompatibles = [...document.querySelectorAll("#compatibles-tags .tag")].map(
    (tag) => tag.textContent,
  );
  selectedCompatibles.length
    ? hashParams.set("compatibles", selectedCompatibles.join("|"))
    : hashParams.delete("compatibles");

  window.history.replaceState({}, "", `#${hashParams.toString()}`);
}

function fillSocFamilySelect() {
  const socFamilySelect = document.getElementById("family");

  Object.keys(socs_data)
    .sort()
    .forEach((f) => {
      socFamilySelect.add(new Option(f));
    });
}

function fillSocSeriesSelect(families, selectOnFill = false) {
  const socSeriesSelect = document.getElementById("series");

  families = families?.length ? families : Object.keys(socs_data);
  let allSeries = [...new Set(families.flatMap((f) => Object.keys(socs_data[f])))];

  socSeriesSelect.innerHTML = "";
  allSeries.sort().map((s) => {
    const option = new Option(s, s, selectOnFill, selectOnFill);
    socSeriesSelect.add(option);
  });
}

function fillSocSocSelect(families, series = undefined, selectOnFill = false) {
  const socSocSelect = document.getElementById("soc");

  families = families?.length ? families : Object.keys(socs_data);
  series = series?.length ? series : families.flatMap((f) => Object.keys(socs_data[f]));
  const matchingSocs = [
    ...new Set(families.flatMap((f) => series.flatMap((s) => socs_data[f][s] || []))),
  ];

  socSocSelect.innerHTML = "";
  matchingSocs.sort().forEach((soc) => {
    socSocSelect.add(new Option(soc, soc, selectOnFill, selectOnFill));
  });
}

function setupHWCapabilitiesField() {
  let selectedTags = [];

  const tagContainer = document.getElementById("hwcaps-tags");
  const tagInput = document.getElementById("hwcaps-input");
  const datalist = document.getElementById("tag-list");

  const tagCounts = Array.from(document.querySelectorAll(".board-card")).reduce((acc, board) => {
    (board.getAttribute("data-supported-features") || "").split(" ").forEach((tag) => {
      acc[tag] = (acc[tag] || 0) + 1;
    });
    return acc;
  }, {});

  const allTags = Object.keys(tagCounts).sort();

  function addTag(tag) {
    if (selectedTags.includes(tag) || tag === "" || !allTags.includes(tag)) return;
    selectedTags.push(tag);

    const tagElement = document.createElement("span");
    tagElement.classList.add("tag");
    tagElement.textContent = tag;
    tagElement.onclick = () => removeTag(tag);
    tagContainer.insertBefore(tagElement, tagInput);

    tagInput.value = "";
    updateDatalist();
  }

  function removeTag(tag) {
    selectedTags = selectedTags.filter((t) => t !== tag);
    document.querySelectorAll(".tag").forEach((el) => {
      if (el.textContent.includes(tag)) el.remove();
    });
    updateDatalist();
  }

  function updateDatalist() {
    datalist.innerHTML = "";
    const filteredTags = allTags.filter((tag) => !selectedTags.includes(tag));

    filteredTags.forEach((tag) => {
      const option = document.createElement("option");
      option.value = tag;
      datalist.appendChild(option);
    });

    filterBoards();
  }

  tagInput.addEventListener("input", () => {
    if (allTags.includes(tagInput.value)) {
      addTag(tagInput.value);
    }
  });

  // Add tag when pressing the Enter key
  tagInput.addEventListener("keydown", (e) => {
    if (e.key === "Enter" && allTags.includes(tagInput.value)) {
      addTag(tagInput.value);
      e.preventDefault();
    }
  });

  // Delete tag when pressing the Backspace key
  tagInput.addEventListener("keydown", (e) => {
    if (e.key === "Backspace" && tagInput.value === "" && selectedTags.length > 0) {
      removeTag(selectedTags[selectedTags.length - 1]);
    }
  });

  updateDatalist();
}

function setupCompatiblesField() {
  let selectedCompatibles = [];

  const tagContainer = document.getElementById("compatibles-tags");
  const tagInput = document.getElementById("compatibles-input");
  const datalist = document.getElementById("compatibles-list");

  // Collect all unique compatibles from boards
  const allCompatibles = Array.from(document.querySelectorAll(".board-card")).reduce(
    (acc, board) => {
      (board.getAttribute("data-compatibles") || "").split(" ").forEach((compat) => {
        if (compat && !acc.includes(compat)) {
          acc.push(compat);
        }
      });
      return acc;
    },
    [],
  );

  allCompatibles.sort();

  function addCompatible(compatible) {
    if (selectedCompatibles.includes(compatible) || compatible === "") return;
    selectedCompatibles.push(compatible);

    const tagElement = document.createElement("span");
    tagElement.classList.add("tag");
    tagElement.textContent = compatible;
    tagElement.onclick = () => removeCompatible(compatible);
    tagContainer.insertBefore(tagElement, tagInput);

    tagInput.value = "";
    updateDatalist();
  }

  function removeCompatible(compatible) {
    selectedCompatibles = selectedCompatibles.filter((c) => c !== compatible);
    document.querySelectorAll(".tag").forEach((el) => {
      if (el.textContent === compatible && el.parentElement === tagContainer) {
        el.remove();
      }
    });
    updateDatalist();
  }

  function updateDatalist() {
    datalist.innerHTML = "";
    const filteredCompatibles = allCompatibles.filter((c) => !selectedCompatibles.includes(c));

    filteredCompatibles.forEach((compatible) => {
      const option = document.createElement("option");
      option.value = compatible;
      datalist.appendChild(option);
    });

    filterBoards();
  }

  tagInput.addEventListener("input", () => {
    if (allCompatibles.includes(tagInput.value)) {
      addCompatible(tagInput.value);
    }
  });

  tagInput.addEventListener("keydown", (e) => {
    if (e.key === "Enter" && tagInput.value) {
      addCompatible(tagInput.value);
      e.preventDefault();
    } else if (e.key === "Backspace" && tagInput.value === "" && selectedCompatibles.length > 0) {
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
  initMemorySliders();
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

  setTimeout(() => {
    if (!hashIndicatesFilteredCatalogView()) return;
    document.getElementById("nb-matches")?.scrollIntoView({ behavior: "smooth", block: "start" });
  }, 0);
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
  document.querySelectorAll("#hwcaps-tags .tag").forEach((tag) => tag.remove());
  document.getElementById("hwcaps-input").value = "";

  // Clear compatibles
  document.querySelectorAll("#compatibles-tags .tag").forEach((tag) => tag.remove());
  document.getElementById("compatibles-input").value = "";

  // Reset memory sliders to full range
  ["ram", "flash"].forEach((type) => {
    const root = document.getElementById(`${type}-slider-root`);
    const cfg = memorySliderConfigs[type];
    if (!root || !root.noUiSlider || !cfg) return;
    root.noUiSlider.set([memoryBytesToSlider(0, cfg), memoryBytesToSlider(Infinity, cfg)], false);
    updateMemorySlider(type);
  });

  filterBoards();
}

function updateBoardCount() {
  const boards = Array.from(document.getElementsByClassName("board-card"));
  const visible = boards.filter((board) => !board.classList.contains("hidden"));
  const shields = boards.filter((board) => board.classList.contains("shield"));
  const visibleShields = visible.filter((board) => board.classList.contains("shield"));

  document.getElementById("nb-matches").textContent =
    `Showing ${visible.length - visibleShields.length} of ${boards.length - shields.length} boards,` +
    ` ${visibleShields.length} of ${shields.length} shields`;
}

function wildcardMatch(pattern, str) {
  // Convert wildcard pattern to regex
  // Escape special regex characters except *
  const regexPattern = pattern.replace(/[.+?^${}()|[\]\\]/g, "\\$&").replace(/\*/g, ".*");
  const regex = new RegExp(`^${regexPattern}$`, "i");
  return regex.test(str);
}

function makeMemorySliderConfig(maxBoardBytes) {
  const allowZero = true;
  const allowInfinite = true;
  const stepMode = "pow2";
  const boundsMin = 2 * MEMORY_KB;
  const maxBoardKiB = Math.ceil(maxBoardBytes / MEMORY_KB);
  /* Slider uses log₂ steps; upper KiB bound is the next power of two for clean scale ends. */
  const maxKiB = Math.max(64, 2 ** Math.ceil(Math.log2(maxBoardKiB)));
  const boundsMax = maxKiB * MEMORY_KB;
  const minExp = Math.log2(boundsMin);
  const maxExp = Math.log2(boundsMax);
  const sliderMin = allowZero ? minExp - 1 : minExp;
  const sliderMax = allowInfinite ? maxExp + 1 : maxExp;
  return {
    allowZero,
    allowInfinite,
    stepMode,
    boundsMin,
    boundsMax,
    minExp,
    maxExp,
    sliderMin,
    sliderMax,
  };
}

function clamp(value, lo, hi) {
  return Math.max(lo, Math.min(hi, value));
}

function memoryBytesToSlider(bytes, cfg) {
  if (cfg.allowZero && bytes === 0) return cfg.sliderMin;
  if (cfg.allowInfinite && bytes === Infinity) return cfg.sliderMax;
  if (!Number.isFinite(bytes)) return cfg.sliderMax;
  if (bytes <= 0) return cfg.sliderMin;
  return clamp(Math.log2(bytes), cfg.minExp, cfg.maxExp);
}

function memorySliderToBytes(value, cfg) {
  value = Number(value);
  if (cfg.allowZero && value <= cfg.minExp - 0.5) return 0;
  if (cfg.allowInfinite && value >= cfg.maxExp + 0.5) return Infinity;
  const exp = cfg.stepMode === "pow2" ? Math.round(value) : value;
  return 2 ** clamp(exp, cfg.minExp, cfg.maxExp);
}

function formatMemoryBytes(bytes) {
  if (bytes === 0) return "0 B";
  if (bytes === Infinity) return "∞";

  let unit = 1;
  let label = "B";

  if (bytes >= MEMORY_GB) {
    unit = MEMORY_GB;
    label = "GiB";
  } else if (bytes >= MEMORY_MB) {
    unit = MEMORY_MB;
    label = "MiB";
  } else if (bytes >= MEMORY_KB) {
    unit = MEMORY_KB;
    label = "KiB";
  }

  const num = bytes / unit;
  const text = Number.isInteger(num) ? String(num) : num.toFixed(2).replace(/\.?0+$/, "");

  return `${text} ${label}`;
}

function parseMemoryValue(value) {
  if (value === null || value === undefined || String(value).trim() === "") return null;

  const bytes = Number(value);
  return Number.isFinite(bytes) && bytes >= 0 ? bytes : null;
}

function parseBoardTargetMemory(board) {
  const encoded = board.getAttribute("data-target-memory");
  if (encoded) {
    try {
      const targets = JSON.parse(encoded);
      if (Array.isArray(targets)) {
        const parsedTargets = targets
          .map((target) => ({
            target: target.target,
            ram: parseMemoryValue(target.ram),
            flash: parseMemoryValue(target.flash),
          }))
          .filter((target) => target.ram !== null || target.flash !== null);
        if (parsedTargets.length) return parsedTargets;
      }
    } catch {
      return [];
    }
  }

  return [];
}

function initMemorySliders() {
  const boards = Array.from(document.querySelectorAll(".board-card"));

  ["ram", "flash"].forEach((type) => {
    const root = document.getElementById(`${type}-slider-root`);
    if (!root) return;

    const values = boards
      .flatMap((b) => parseBoardTargetMemory(b).map((target) => target[type]))
      .filter((v) => v !== null && v > 0);

    const maxBoardBytes = values.length ? Math.max(...values) : MEMORY_DEFAULT_MAX_KIB * MEMORY_KB;

    const cfg = makeMemorySliderConfig(maxBoardBytes);
    memorySliderConfigs[type] = cfg;
    const typeLabel = type === "ram" ? "RAM" : "Flash";

    if (typeof noUiSlider === "undefined") {
      updateMemorySlider(type);
      return;
    }

    noUiSlider.create(root, {
      start: [memoryBytesToSlider(0, cfg), memoryBytesToSlider(Infinity, cfg)],
      range: {
        min: cfg.sliderMin,
        max: cfg.sliderMax,
      },
      step: cfg.stepMode === "pow2" ? 1 : undefined,
      connect: true,
      behaviour: "tap-drag",
      keyboardSupport: true,
      handleAttributes: [
        { "aria-label": `Minimum on-target ${typeLabel}` },
        { "aria-label": `Maximum on-target ${typeLabel}` },
      ],
      ariaFormat: {
        to(value) {
          return formatMemoryBytes(memorySliderToBytes(value, cfg));
        },
        from(value) {
          return Number(value);
        },
      },
      pips: {
        mode: "steps",
        density: 4,
        // Show labeled ticks at min, max, and every 25% of the range. 0 and infinity have no tick
        filter(value) {
          const x = Number(value);
          if (!Number.isFinite(x) || Math.abs(x - Math.round(x)) > 1e-6) return -1;
          const v = Math.round(x);
          if (v < cfg.minExp || v > cfg.maxExp) return -1;
          const lo = cfg.minExp;
          const hi = cfg.maxExp;
          const s = hi - lo;
          const labeled = new Set([
            lo,
            hi,
            Math.round(lo + s * 0.25),
            Math.round(lo + s * 0.5),
            Math.round(lo + s * 0.75),
          ]);
          return labeled.has(v) ? 2 : 0;
        },
        format: {
          to(value) {
            return formatMemoryBytes(memorySliderToBytes(value, cfg));
          },
          from(value) {
            return Number(value);
          },
        },
      },
    });

    const onMemorySliderChange = () => {
      updateMemorySlider(type);
      filterBoards();
    };

    root.noUiSlider.on("slide", onMemorySliderChange);
    root.noUiSlider.on("set", onMemorySliderChange);

    /* Sync disabled state + strip handle tabindex when catalog lacks HW feature metadata */
    if (root.hasAttribute("disabled")) {
      root.noUiSlider.disable();
    }
  });
}

function updateMemorySlider(type) {
  const label = document.getElementById(`${type}-range-label`);
  const root = document.getElementById(`${type}-slider-root`);
  const cfg = memorySliderConfigs[type];

  if (!label) return;

  if (!root || !root.noUiSlider || !cfg) {
    label.textContent = "Any";
    return;
  }

  const unencoded = root.noUiSlider.get(true);
  const minBytes = memorySliderToBytes(unencoded[0], cfg);
  const maxBytes = memorySliderToBytes(unencoded[1], cfg);

  const atMin = minBytes <= 0;
  const atMax = maxBytes >= Infinity;

  if (atMin && atMax) {
    label.textContent = "Any";
  } else if (atMin) {
    label.textContent = `≤ ${formatMemoryBytes(maxBytes)}`;
  } else if (atMax) {
    label.textContent = `≥ ${formatMemoryBytes(minBytes)}`;
  } else {
    label.textContent = `${formatMemoryBytes(minBytes)} – ${formatMemoryBytes(maxBytes)}`;
  }
}

function memorySliderRange(type) {
  /* Returns {minBytes, maxBytes} from the noUiSlider for 'type' ("ram" or "flash").
   * maxBytes is Infinity when the upper handle is at the unlimited end of the scale. */
  const root = document.getElementById(`${type}-slider-root`);
  const cfg = memorySliderConfigs[type];
  if (!root || !root.noUiSlider || !cfg) {
    return { minBytes: 0, maxBytes: Infinity };
  }
  const unencoded = root.noUiSlider.get(true);
  return {
    minBytes: memorySliderToBytes(unencoded[0], cfg),
    maxBytes: memorySliderToBytes(unencoded[1], cfg),
  };
}

function memoryInRange(boardBytes, minBytes, maxBytes) {
  /* Returns false when the board should be excluded by a memory range filter. */
  if (boardBytes === null || Number.isNaN(boardBytes)) {
    return minBytes <= 0 && maxBytes >= Infinity;
  }
  return boardBytes >= minBytes && (maxBytes >= Infinity || boardBytes <= maxBytes);
}

function targetMemoryMatches(targets, ramMinBytes, ramMaxBytes, flashMinBytes, flashMaxBytes) {
  const ramFiltered = ramMinBytes > 0 || ramMaxBytes < Infinity;
  const flashFiltered = flashMinBytes > 0 || flashMaxBytes < Infinity;

  if (!ramFiltered && !flashFiltered) return true;

  return targets.some(
    (target) =>
      memoryInRange(target.ram, ramMinBytes, ramMaxBytes) &&
      memoryInRange(target.flash, flashMinBytes, flashMaxBytes),
  );
}

function filterBoards() {
  const nameInput = document.getElementById("name").value.toLowerCase();
  const archSelect = document.getElementById("arch").value;

  const { minBytes: ramMinBytes, maxBytes: ramMaxBytes } = memorySliderRange("ram");
  const { minBytes: flashMinBytes, maxBytes: flashMaxBytes } = memorySliderRange("flash");

  const vendorSelect = document.getElementById("vendor").value;
  const socSocSelect = document.getElementById("soc");
  const showBoards = document.getElementById("show-boards").checked;
  const showShields = document.getElementById("show-shields").checked;

  // Get selected hardware capability tags
  const selectedHWTags = [...document.querySelectorAll("#hwcaps-tags .tag")].map(
    (tag) => tag.textContent,
  );

  // Get selected compatible tags
  const selectedCompatibles = [...document.querySelectorAll("#compatibles-tags .tag")].map(
    (tag) => tag.textContent,
  );

  const resetFiltersBtn = document.getElementById("reset-filters");
  const ramFiltered = ramMinBytes > 0 || ramMaxBytes < Infinity;
  const flashFiltered = flashMinBytes > 0 || flashMaxBytes < Infinity;
  if (
    nameInput ||
    archSelect ||
    ramFiltered ||
    flashFiltered ||
    vendorSelect ||
    socSocSelect.selectedOptions.length ||
    selectedHWTags.length ||
    selectedCompatibles.length ||
    !showBoards ||
    !showShields
  ) {
    resetFiltersBtn.classList.remove("btn-disabled");
  } else {
    resetFiltersBtn.classList.add("btn-disabled");
  }

  const boards = document.getElementsByClassName("board-card");

  Array.from(boards).forEach(function (board) {
    const boardName = board.getAttribute("data-name").toLowerCase();
    const boardArchs = (board.getAttribute("data-arch") || "").split(" ").filter(Boolean);
    const boardTargetMemory = parseBoardTargetMemory(board);
    const boardVendor = board.getAttribute("data-vendor") || "";
    const boardSocs = (board.getAttribute("data-socs") || "").split(" ").filter(Boolean);
    const boardSupportedFeatures = (board.getAttribute("data-supported-features") || "")
      .split(" ")
      .filter(Boolean);
    const boardCompatibles = (board.getAttribute("data-compatibles") || "")
      .split(" ")
      .filter(Boolean);
    const isShield = board.classList.contains("shield");

    let matches = true;

    const selectedSocs = [...socSocSelect.selectedOptions].map(({ value }) => value);

    if ((isShield && !showShields) || (!isShield && !showBoards)) {
      matches = false;
    } else {
      // Check if board matches all selected compatibles (with wildcard support)
      const compatiblesMatch =
        selectedCompatibles.length === 0 ||
        selectedCompatibles.every((pattern) =>
          boardCompatibles.some((compatible) => wildcardMatch(pattern, compatible)),
        );

      matches =
        !(isShield && (ramFiltered || flashFiltered)) &&
        !(nameInput && !boardName.includes(nameInput)) &&
        !(archSelect && !boardArchs.includes(archSelect)) &&
        targetMemoryMatches(
          boardTargetMemory,
          ramMinBytes,
          ramMaxBytes,
          flashMinBytes,
          flashMaxBytes,
        ) &&
        !(vendorSelect && boardVendor !== vendorSelect) &&
        (selectedSocs.length === 0 || selectedSocs.some((soc) => boardSocs.includes(soc))) &&
        (selectedHWTags.length === 0 ||
          selectedHWTags.every((tag) => boardSupportedFeatures.includes(tag))) &&
        compatiblesMatch;
    }

    board.classList.toggle("hidden", !matches);
  });

  updateURL();
  updateBoardCount();
}
