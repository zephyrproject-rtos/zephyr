/**
 * Copyright (c) 2025, The Linux Foundation.
 * SPDX-License-Identifier: Apache-2.0
 */

(() => {
  // SVG icons for copy button
  const COPY_ICON = `<svg viewBox="0 0 24 24"><path d="M16 1H4c-1.1 0-2 .9-2 2v14h2V3h12V1zm3 4H8c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h11c1.1 0 2-.9 2-2V7c0-1.1-.9-2-2-2zm0 16H8V7h11v14z"/></svg>`;
  const CHECK_ICON = `<svg viewBox="0 0 24 24"><path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41L9 16.17z"/></svg>`;

  // DOM element creation helper
  const createElement = (tag, className, text) => {
    const element = document.createElement(tag);
    if (className) element.className = className;
    if (text) element.textContent = text;
    return element;
  };

  const boardTargetsList = () => {
    if (!Array.isArray(board_data?.targets)) return [];
    return board_data.targets.filter((t) => t != null && String(t).trim() !== "");
  };

  /** Parse a canonical Zephyr board target string; safe for missing or empty input. */
  const parseTargetString = (targetString, defaultBoard = "") => {
    if (targetString == null || String(targetString).trim() === "") {
      return { board: defaultBoard || "", revision: "", qualifier: "" };
    }
    const s = String(targetString);
    const [boardWithRev, ...qualifiers] = s.split("/");
    const [board, revision] = (boardWithRev ?? "").split("@");
    return {
      board: board || defaultBoard || "",
      revision: revision || "",
      qualifier: qualifiers.join("/") || "",
    };
  };

  // Select element creation helper
  const createSelect = (options, selectedValue) => {
    const select = createElement("select");
    options.sort().forEach((value) => {
      const option = createElement("option", null, value);
      option.value = value;
      option.selected = value === selectedValue;
      select.appendChild(option);
    });
    return select;
  };

  // Main initialization function
  const initializeBoardSelector = () => {
    const container = document.querySelector('.container[id$="-hw-features"]');
    if (!container) return console.error("Container element not found");

    const targets = boardTargetsList();
    if (targets.length === 0) {
      initializeDisplay(undefined, undefined);
      return;
    }

    const components = {
      boards: new Set([board_data.board_name]),
      revisions: new Set(),
      qualifiers: new Set(),
    };

    targets.forEach((target) => {
      const { revision, qualifier } = parseTargetString(target, board_data.board_name || "");
      if (revision) components.revisions.add(revision);
      if (qualifier) components.qualifiers.add(qualifier);
    });

    const initialValues = parseTargetString(targets[0], board_data.board_name || "");
    if (board_data.revision_default) {
      initialValues.revision = board_data.revision_default;
    }

    const selector = createElement("div", "board-target-selector");
    selector.appendChild(createElement("div", "static-value", initialValues.board));

    // Add revision selector if needed
    let revisionSelect;
    if (components.revisions.size > 0) {
      selector.appendChild(createElement("div", "separator", "@"));
      revisionSelect = createSelect(Array.from(components.revisions), initialValues.revision);
      selector.appendChild(revisionSelect);
    }

    // Add qualifier selector or static value
    let qualifierSelect;
    if (components.qualifiers.size > 0) {
      selector.appendChild(createElement("div", "separator", "/"));
      if (components.qualifiers.size === 1) {
        selector.appendChild(createElement("div", "static-value", initialValues.qualifier));
      } else {
        qualifierSelect = createSelect(Array.from(components.qualifiers), null);
        selector.appendChild(qualifierSelect);
      }
    }

    // Add copy button
    const copyButton = createElement("button", "copy-button");
    copyButton.innerHTML = COPY_ICON;
    copyButton.addEventListener(
      "click",
      handleCopyButtonClick(initialValues, revisionSelect, qualifierSelect, copyButton),
    );
    selector.appendChild(copyButton);

    // Insert selector into DOM and set up event listeners
    container.parentNode.insertBefore(selector, container);
    selector.querySelectorAll("select").forEach((select) => {
      select.addEventListener("change", () => {
        updateSelectOptions(initialValues, revisionSelect, qualifierSelect);
        updateDisplayedTarget(revisionSelect, qualifierSelect);
      });
    });

    // Initial display setup
    initializeDisplay(revisionSelect, qualifierSelect);
  };

  // Copy button click handler
  const handleCopyButtonClick =
    (initialValues, revisionSelect, qualifierSelect, button) => async () => {
      try {
        const target = buildTargetString(initialValues, revisionSelect, qualifierSelect);
        await navigator.clipboard.writeText(target);

        button.classList.add("copied");
        button.innerHTML = CHECK_ICON;

        setTimeout(() => {
          button.classList.remove("copied");
          button.innerHTML = COPY_ICON;
        }, 1000);
      } catch (error) {
        console.error("Failed to copy text:", error);
      }
    };

  // Build target string from current selections
  const buildTargetString = (initialValues, revisionSelect, qualifierSelect) => {
    const parts = [initialValues.board];

    if (revisionSelect) {
      parts.push(`@${revisionSelect.value}`);
    } else if (initialValues.revision) {
      parts.push(`@${initialValues.revision}`);
    }

    if (qualifierSelect) {
      parts.push(`/${qualifierSelect.value}`);
    } else if (initialValues.qualifier) {
      parts.push(`/${initialValues.qualifier}`);
    }

    return parts.join("");
  };

  /** Panel id: ``{board_id}-{target}-hw-features-panel`` (same target string as section key). */
  const targetKeyFromHwFeaturesPanelId = (panelId) => {
    const suffix = "-hw-features-panel";
    if (!panelId.endsWith(suffix)) {
      return "";
    }
    const core = panelId.slice(0, -suffix.length);
    const prefix = `${board_data.board_name}-`;
    return core.startsWith(prefix) ? core.slice(prefix.length) : "";
  };

  const buildCurrentTargetString = (revisionSelect, qualifierSelect) => {
    const list = boardTargetsList();
    const initialValues = parseTargetString(list[0], board_data.board_name || "");
    if (board_data.revision_default) {
      initialValues.revision = board_data.revision_default;
    }
    return buildTargetString(initialValues, revisionSelect, qualifierSelect);
  };

  // Update displayed build target (memory + hardware-features panel) based on selections
  const updateDisplayedTarget = (revisionSelect, qualifierSelect) => {
    const currentTarget = buildCurrentTargetString(revisionSelect, qualifierSelect);
    const panels = document.querySelectorAll('[id$="-hw-features-panel"]');
    const singlePanel = panels.length === 1 ? panels[0] : null;

    panels.forEach((panel) => {
      const targetKey = targetKeyFromHwFeaturesPanelId(panel.id);
      let isMatch = targetKey === currentTarget;
      if (!isMatch && singlePanel === panel) {
        isMatch = true;
      }
      panel.style.display = isMatch ? "" : "none";
    });
  };

  // Initial display setup
  const initializeDisplay = (revisionSelect, qualifierSelect) => {
    document.querySelectorAll('section[id$="-hw-features-section"]').forEach((section) => {
      section.style.display = "none";
    });
    document.querySelectorAll('[id$="-hw-features-panel"]').forEach((panel) => {
      panel.style.display = "none";
    });
    updateDisplayedTarget(revisionSelect, qualifierSelect);
  };

  // Update select options based on valid combinations
  const updateSelectOptions = (initialValues, revisionSelect, qualifierSelect) => {
    if (revisionSelect) {
      const currentBoard = initialValues.board;
      const validRevisions = new Set();

      // Find valid revisions for current board
      boardTargetsList().forEach((target) => {
        const { board, revision } = parseTargetString(target, board_data.board_name || "");
        if (board === currentBoard && revision) {
          validRevisions.add(revision);
        }
      });

      // Update revision select options
      Array.from(revisionSelect.options).forEach((option) => {
        option.disabled = !validRevisions.has(option.value);
        if (option.disabled && option.selected) {
          // If current selection is invalid, select first valid option
          const firstValid = Array.from(revisionSelect.options).find((opt) => !opt.disabled);
          if (firstValid) firstValid.selected = true;
        }
      });
    }

    if (qualifierSelect) {
      const currentBoard = initialValues.board;
      const currentRevision = revisionSelect ? revisionSelect.value : initialValues.revision;
      const validQualifiers = new Set();

      // Find valid qualifiers for current board and revision
      boardTargetsList().forEach((target) => {
        const { board, revision, qualifier } = parseTargetString(
          target,
          board_data.board_name || "",
        );
        if (board === currentBoard && (!revision || revision === currentRevision) && qualifier) {
          validQualifiers.add(qualifier);
        }
      });

      // Update qualifier select options
      Array.from(qualifierSelect.options).forEach((option) => {
        option.disabled = !validQualifiers.has(option.value);
        if (option.disabled && option.selected) {
          // If current selection is invalid, select first valid option
          const firstValid = Array.from(qualifierSelect.options).find((opt) => !opt.disabled);
          if (firstValid) firstValid.selected = true;
        }
      });
    }
  };

  // Start initialization when DOM is ready
  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", initializeBoardSelector);
  } else {
    initializeBoardSelector();
  }
})();
