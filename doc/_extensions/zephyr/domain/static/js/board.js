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

  // Target parsing helper
  const parseTargetString = (targetString) => {
    const [boardWithRev, ...qualifiers] = targetString.split('/');
    const [board, revision] = boardWithRev.split('@');
    return {
      board: board || '',
      revision: revision || '',
      qualifier: qualifiers.join('/') || ''
    };
  };

  // Select element creation helper
  const createSelect = (options, selectedValue) => {
    const select = createElement('select');
    options.sort().forEach(value => {
      const option = createElement('option', null, value);
      option.value = value;
      option.selected = (value === selectedValue);
      select.appendChild(option);
    });
    return select;
  };

  // Main initialization function
  const initializeBoardSelector = () => {
    const container = document.querySelector('.container[id$="-hw-features"]');
    if (!container) return console.error('Container element not found');

    const components = {
      boards: new Set([board_data.board_name]),
      revisions: new Set(),
      qualifiers: new Set()
    };

    board_data.targets.forEach(target => {
      const { revision, qualifier } = parseTargetString(target);
      if (revision) components.revisions.add(revision);
      if (qualifier) components.qualifiers.add(qualifier);
    });

    const initialValues = parseTargetString(board_data.targets[0]);
    if (board_data.revision_default) {
      initialValues.revision = board_data.revision_default;
    }

    const selector = createElement('div', 'board-target-selector');
    selector.appendChild(createElement('div', 'static-value', initialValues.board));

    // Add revision selector if needed
    let revisionSelect;
    if (components.revisions.size > 0) {
      selector.appendChild(createElement('div', 'separator', '@'));
      revisionSelect = createSelect(
        Array.from(components.revisions),
        initialValues.revision
      );
      selector.appendChild(revisionSelect);
    }

    // Add qualifier selector or static value
    let qualifierSelect;
    if (components.qualifiers.size > 0) {
      selector.appendChild(createElement('div', 'separator', '/'));
      if (components.qualifiers.size === 1) {
        selector.appendChild(createElement('div', 'static-value', initialValues.qualifier));
      } else {
        qualifierSelect = createSelect(
          Array.from(components.qualifiers),
          null
        );
        selector.appendChild(qualifierSelect);
      }
    }

    // Add copy button
    const copyButton = createElement('button', 'copy-button');
    copyButton.innerHTML = COPY_ICON;
    copyButton.addEventListener('click', handleCopyButtonClick(
      initialValues,
      revisionSelect,
      qualifierSelect,
      copyButton
    ));
    selector.appendChild(copyButton);

    // Insert selector into DOM and set up event listeners
    container.parentNode.insertBefore(selector, container);
    selector.querySelectorAll('select').forEach(select => {
      select.addEventListener('change', () => {
        updateSelectOptions(initialValues, revisionSelect, qualifierSelect);
        updateDisplayedTable(revisionSelect, qualifierSelect);

      });
    });

    // Initial display setup
    initializeDisplay();
    // updateDisplayedTable(revisionSelect, qualifierSelect);
  };

  // Copy button click handler
  const handleCopyButtonClick = (initialValues, revisionSelect, qualifierSelect, button) => async () => {
    try {
      const target = buildTargetString(initialValues, revisionSelect, qualifierSelect);
      await navigator.clipboard.writeText(target);

      button.classList.add('copied');
      button.innerHTML = CHECK_ICON;

      setTimeout(() => {
        button.classList.remove('copied');
        button.innerHTML = COPY_ICON;
      }, 1000);
    } catch (error) {
      console.error('Failed to copy text:', error);
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

    return parts.join('');
  };

  // Update displayed table based on selections
  const updateDisplayedTable = (revisionSelect, qualifierSelect) => {
    const currentTarget = buildTargetString(
      parseTargetString(board_data.targets[0]),
      revisionSelect,
      qualifierSelect
    );

    console.log(currentTarget);

    document.querySelectorAll('section[id$="-hw-features-section"]').forEach(section => {
      // Find the matching target in board_data.targets
      const isMatch = section.id.replace(/${board_data.name}/).replace(/-hw-features-section$/, '').endsWith(currentTarget);

      const table = document.querySelector(`table[id="${section.id.replace('-section', '-table')}"]`);
      const wrapper = table?.closest('.wy-table-responsive');


      if (table) table.style.display = isMatch ? 'table' : 'none';
      if (wrapper) wrapper.style.display = isMatch ? 'block' : 'none';
    });
  };

  // Initial display setup
  const initializeDisplay = () => {
    document.querySelectorAll('section[id$="-hw-features-section"]').forEach(section => section.style.display = 'none');
    document.querySelectorAll('table.hardware-features').forEach((table, index) => {
      const wrapper = table.closest('.wy-table-responsive');
      table.style.display = index === 0 ? 'table' : 'none';
      if (wrapper) wrapper.style.display = index === 0 ? 'block' : 'none';
    });
    updateDisplayedTable(revisionSelect, qualifierSelect);
  };

  // Update select options based on valid combinations
  const updateSelectOptions = (initialValues, revisionSelect, qualifierSelect) => {
    if (revisionSelect) {
      const currentBoard = initialValues.board;
      const validRevisions = new Set();

      // Find valid revisions for current board
      board_data.targets.forEach(target => {
        const { board, revision } = parseTargetString(target);
        if (board === currentBoard && revision) {
          validRevisions.add(revision);
        }
      });

      // Update revision select options
      Array.from(revisionSelect.options).forEach(option => {
        option.disabled = !validRevisions.has(option.value);
        if (option.disabled && option.selected) {
          // If current selection is invalid, select first valid option
          const firstValid = Array.from(revisionSelect.options).find(opt => !opt.disabled);
          if (firstValid) firstValid.selected = true;
        }
      });
    }

    if (qualifierSelect) {
      const currentBoard = initialValues.board;
      const currentRevision = revisionSelect ? revisionSelect.value : initialValues.revision;
      const validQualifiers = new Set();

      // Find valid qualifiers for current board and revision
      board_data.targets.forEach(target => {
        const { board, revision, qualifier } = parseTargetString(target);
        if (board === currentBoard &&
          (!revision || revision === currentRevision) &&
          qualifier) {
          validQualifiers.add(qualifier);
        }
      });

      // Update qualifier select options
      Array.from(qualifierSelect.options).forEach(option => {
        option.disabled = !validQualifiers.has(option.value);
        if (option.disabled && option.selected) {
          // If current selection is invalid, select first valid option
          const firstValid = Array.from(qualifierSelect.options).find(opt => !opt.disabled);
          if (firstValid) firstValid.selected = true;
        }
      });
    }
  };

  // Start initialization when DOM is ready
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initializeBoardSelector);
  } else {
    initializeBoardSelector();
  }
})();
