/**
 * TreeTable — lightweight hierarchical table library
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Usage:
 *   const tt = TreeTable('tableId', options);
 *   const tt = TreeTable(tableElement, options);
 *
 * Options:
 *   data        {Array}    Required. Array of root node objects.
 *   columns     {Array}    Required. Array of column definition objects.
 *   indentSize  {number}   Optional. Pixels per depth level. Default: 20.
 *   icons       {Object}   Optional. Icon configuration object:
 *     icons.expand   {string}   CSS class(es) for the collapsed-parent toggle icon.
 *     icons.collapse {string}   CSS class(es) for the expanded-parent toggle icon.
 *     icons.node     {Function} (node, state) => cssClassString | null
 *   onExpand    {Function} Optional. (node) => void
 *   onCollapse  {Function} Optional. (node) => void
 *   onSelect    {Function} Optional. (node) => void — fired when a row is selected.
 *   onRowCreated {Function} Optional. (node, tr) => void — fired after each row is appended to the table.
 *
 * Node format:
 *   { id, data, children, expanded }
 *
 * Column definition format:
 *   { render: (node) => string|Element, className: 'optional-class' }
 *
 * Public API (returned object):
 *   tt.select(id)        — programmatically select the row with the given id.
 *   tt.deselect()        — deselect the currently selected row (if any).
 *   tt.getSelected()     — return the currently selected node object, or null.
 *   tt.expand(id)        — expand the node with the given id.
 *   tt.collapse(id)      — collapse the node with the given id.
 *   tt.expandAll()       — expand all nodes that have children.
 *   tt.collapseAll()     — collapse all nodes that have children.
 *   tt.filter(str)       — filter rows by string; the table gets tt-filtered and
 *                          matching rows get tt-filter-match. Ancestors of matching
 *                          rows are auto-expanded and re-collapsed when no longer needed.
 *                          Pass '' or null to clear the filter.
 */
(function (global) {
  'use strict';

  /**
   * Factory function — creates and initialises a TreeTable instance.
   *
   * @param {string|Element} tableId  - The id of the <table> element, or the element itself.
   * @param {Object} options          - Configuration object (see above).
   * @returns {Object}                - Public API object.
   */
  function TreeTable(tableId, options) {
    // ------------------------------------------------------------------ //
    // Validate inputs
    // ------------------------------------------------------------------ //
    var table = typeof tableId === 'string' ? document.getElementById(tableId) : tableId;
    if (!table || !(table instanceof Element)) {
      throw new Error('TreeTable: first argument must be a table element id string or a DOM Element');
    }

    var opts = options || {};
    var data = opts.data || [];
    var columns = opts.columns || [];
    var indentSize = typeof opts.indentSize === 'number' ? opts.indentSize : 20;
    var icons = opts.icons && typeof opts.icons === 'object' ? opts.icons : {};
    var iconExpandCls = typeof icons.expand === 'string' && icons.expand.trim() !== '' ? icons.expand.trim() : null;
    var iconCollapseCls = typeof icons.collapse === 'string' && icons.collapse.trim() !== '' ? icons.collapse.trim() : null;
    var iconClassFn = typeof icons.node === 'function' ? icons.node : null;
    var onExpand = typeof opts.onExpand === 'function' ? opts.onExpand : null;
    var onCollapse = typeof opts.onCollapse === 'function' ? opts.onCollapse : null;
    var onSelect = typeof opts.onSelect === 'function' ? opts.onSelect : null;
    var onRowCreated = typeof opts.onRowCreated === 'function' ? opts.onRowCreated : null;

    if (columns.length === 0) {
      throw new Error('TreeTable: "columns" must be a non-empty array');
    }

    // ------------------------------------------------------------------ //
    // Internal state
    // nodeMap: { [id]: { node, expanded, tr, depth, parentId } }
    // selectedId: id of the currently selected row (or null)
    // autoExpandedIds: plain-object set of ids expanded silently by filter()
    // ------------------------------------------------------------------ //
    var nodeMap = {};
    var selectedId = null;
    var autoExpandedIds = {};

    // ------------------------------------------------------------------ //
    // Locate or create <tbody>
    // ------------------------------------------------------------------ //
    var tbody = table.querySelector('tbody');
    if (!tbody) {
      tbody = document.createElement('tbody');
      table.appendChild(tbody);
    }
    // Clear any existing rows
    tbody.innerHTML = '';

    // ------------------------------------------------------------------ //
    // Build the toggle icon element for a given state.
    // state: 'expanded' | 'collapsed' | 'leaf'
    // Uses icons.expand / icons.collapse CSS classes when provided,
    // otherwise falls back to +/- text.
    // ------------------------------------------------------------------ //
    function buildToggleIcon(state) {
      var icon = document.createElement('span');
      icon.className = 'tt-icon';
      if (state === 'collapsed') {
        if (iconExpandCls) {
          iconExpandCls.split(/\s+/).forEach(function (c) { icon.classList.add(c); });
        } else {
          icon.textContent = '+';
        }
      } else if (state === 'expanded') {
        if (iconCollapseCls) {
          iconCollapseCls.split(/\s+/).forEach(function (c) { icon.classList.add(c); });
        } else {
          icon.textContent = '-';
        }
      } else {
        // leaf — blank, no pointer cursor
        icon.classList.add('tt-icon-leaf');
      }
      return icon;
    }

    // ------------------------------------------------------------------ //
    // Build the user-supplied icon element (tt-user-icon) for a given node
    // and state. Returns null if no iconClassFn is set or it returns nothing.
    // ------------------------------------------------------------------ //
    function buildUserIcon(node, state) {
      if (!iconClassFn) return null;
      var cls = iconClassFn(node, state);
      if (!cls || typeof cls !== 'string' || cls.trim() === '') return null;
      var icon = document.createElement('span');
      icon.className = 'tt-user-icon';
      cls.trim().split(/\s+/).forEach(function (c) {
        icon.classList.add(c);
      });
      return icon;
    }

    // ------------------------------------------------------------------ //
    // Update the toggle icon and user icon inside a row's first cell.
    // DOM order: [indent] [tt-icon] [tt-user-icon?] [tt-cell-content]
    // ------------------------------------------------------------------ //
    function updateIcon(entry) {
      var td = entry.tr.cells[0];
      var hasChildren = entry.node.children && entry.node.children.length > 0;
      var state = hasChildren ? (entry.expanded ? 'expanded' : 'collapsed') : 'leaf';

      // --- Replace toggle icon ---
      var oldToggle = td.querySelector('.tt-icon');
      var newToggle = buildToggleIcon(state);
      if (hasChildren) {
        newToggle.addEventListener('click', function () {
          toggleNode(entry.node.id);
        });
      }
      td.replaceChild(newToggle, oldToggle);

      // --- Replace or insert user icon ---
      var oldUserIcon = td.querySelector('.tt-user-icon');
      var newUserIcon = buildUserIcon(entry.node, state);
      if (oldUserIcon) {
        if (newUserIcon) {
          td.replaceChild(newUserIcon, oldUserIcon);
        } else {
          td.removeChild(oldUserIcon);
        }
      } else if (newUserIcon) {
        // Insert after the toggle icon, before tt-cell-content
        var content = td.querySelector('.tt-cell-content');
        td.insertBefore(newUserIcon, content);
      }
    }

    // ------------------------------------------------------------------ //
    // Determine whether a row should be visible:
    // visible if all ancestors are expanded
    // ------------------------------------------------------------------ //
    function isVisible(entry) {
      var parentId = entry.parentId;
      while (parentId !== null && parentId !== undefined) {
        var parentEntry = nodeMap[parentId];
        if (!parentEntry) break;
        if (!parentEntry.expanded) return false;
        parentId = parentEntry.parentId;
      }
      return true;
    }

    // ------------------------------------------------------------------ //
    // Show / hide direct children of a node, recursing into expanded ones
    // ------------------------------------------------------------------ //
    function setChildrenVisibility(parentId, visible) {
      var parentEntry = nodeMap[parentId];
      if (!parentEntry || !parentEntry.node.children) return;

      parentEntry.node.children.forEach(function (child) {
        var childEntry = nodeMap[child.id];
        if (!childEntry) return;

        if (visible) {
          childEntry.tr.style.display = '';
          // If this child is itself expanded, show its children too
          if (childEntry.expanded && childEntry.node.children && childEntry.node.children.length > 0) {
            setChildrenVisibility(child.id, true);
          }
        } else {
          childEntry.tr.style.display = 'none';
          // Recursively hide all descendants regardless of their expanded state
          if (childEntry.node.children && childEntry.node.children.length > 0) {
            setChildrenVisibility(child.id, false);
          }
        }
      });
    }

    // ------------------------------------------------------------------ //
    // Toggle expand/collapse for a node
    // ------------------------------------------------------------------ //
    function toggleNode(id) {
      var entry = nodeMap[id];
      if (!entry) return;
      if (!entry.node.children || entry.node.children.length === 0) return;

      if (entry.expanded) {
        collapseNode(id);
      } else {
        expandNode(id);
      }
    }

    // ------------------------------------------------------------------ //
    // Expand / collapse a node silently (no onExpand / onCollapse callback).
    // Used internally by filter() so user callbacks are not spammed.
    // ------------------------------------------------------------------ //
    function expandNodeSilent(id) {
      var entry = nodeMap[id];
      if (!entry) return;
      if (!entry.node.children || entry.node.children.length === 0) return;
      if (entry.expanded) return;
      entry.expanded = true;
      updateIcon(entry);
      setChildrenVisibility(id, true);
    }

    function collapseNodeSilent(id) {
      var entry = nodeMap[id];
      if (!entry) return;
      if (!entry.node.children || entry.node.children.length === 0) return;
      if (!entry.expanded) return;
      entry.expanded = false;
      updateIcon(entry);
      setChildrenVisibility(id, false);
    }

    // ------------------------------------------------------------------ //
    // Return a plain-object set of all ancestor ids of the currently
    // selected row. Used by filter() to avoid hiding the selected row
    // when collapsing auto-expanded nodes.
    // ------------------------------------------------------------------ //
    function getSelectedAncestorIds() {
      var result = {};
      if (selectedId === null) return result;
      var selEntry = nodeMap[selectedId];
      if (!selEntry) return result;
      var pid = selEntry.parentId;
      while (pid !== null && pid !== undefined) {
        result[pid] = true;
        var pEntry = nodeMap[pid];
        if (!pEntry) break;
        pid = pEntry.parentId;
      }
      return result;
    }

    // ------------------------------------------------------------------ //
    // Expand / collapse a single node (internal) — fires user callbacks.
    // ------------------------------------------------------------------ //
    function expandNode(id) {
      expandNodeSilent(id);
      var entry = nodeMap[id];
      if (entry && entry.expanded && onExpand) onExpand(entry.node);
    }

    function collapseNode(id) {
      collapseNodeSilent(id);
      var entry = nodeMap[id];
      if (entry && !entry.expanded && onCollapse) onCollapse(entry.node);
    }

    // ------------------------------------------------------------------ //
    // Build a <td> for the first column (includes indent + icon + content)
    // ------------------------------------------------------------------ //
    function buildFirstCell(node, depth, colDef) {
      var td = document.createElement('td');
      if (colDef.className) td.className = colDef.className;

      // Indent spacer
      var indent = document.createElement('span');
      indent.className = 'tt-indent';
      indent.style.display = 'inline-block';
      indent.style.width = (depth * indentSize) + 'px';
      td.appendChild(indent);

      // Icon placeholder — will be replaced by updateIcon after nodeMap entry exists
      var iconPlaceholder = document.createElement('span');
      iconPlaceholder.className = 'tt-icon';
      td.appendChild(iconPlaceholder);

      // Rendered content
      var content = document.createElement('span');
      content.className = 'tt-cell-content';
      var rendered = colDef.render(node);
      if (rendered instanceof Element || rendered instanceof DocumentFragment) {
        content.appendChild(rendered);
      } else if (colDef.htmlContent === true) {
        content.innerHTML = rendered == null ? '' : String(rendered);
      } else {
        content.textContent = rendered == null ? '' : String(rendered);
      }
      td.appendChild(content);

      return td;
    }

    // ------------------------------------------------------------------ //
    // Build a <td> for columns 1..n
    // ------------------------------------------------------------------ //
    function buildCell(node, colDef) {
      var td = document.createElement('td');
      if (colDef.className) td.className = colDef.className;

      var content = document.createElement('span');
      content.className = 'tt-cell-content';
      var rendered = colDef.render(node);
      if (rendered instanceof Element || rendered instanceof DocumentFragment) {
        content.appendChild(rendered);
      } else if (colDef.htmlContent === true) {
        content.innerHTML = rendered == null ? '' : String(rendered);
      } else {
        content.textContent = rendered == null ? '' : String(rendered);
      }
      td.appendChild(content);

      return td;
    }

    // ------------------------------------------------------------------ //
    // Ensure all ancestors of a node are expanded so the row is visible
    // ------------------------------------------------------------------ //
    function ensureVisible(id) {
      var entry = nodeMap[id];
      if (!entry) return;
      var parentId = entry.parentId;
      while (parentId !== null && parentId !== undefined) {
        var parentEntry = nodeMap[parentId];
        if (!parentEntry) break;
        if (!parentEntry.expanded) {
          expandNode(parentId);
        }
        parentId = parentEntry.parentId;
      }
    }

    // ------------------------------------------------------------------ //
    // Select a row by id (deselects the previously selected row)
    // ------------------------------------------------------------------ //
    function selectNode(id) {
      // Make sure the row is visible before selecting it
      ensureVisible(id);

      // Deselect previous
      if (selectedId !== null && selectedId !== id) {
        var prevEntry = nodeMap[selectedId];
        if (prevEntry) {
          prevEntry.tr.classList.remove('tt-selected');
        }
      }

      var entry = nodeMap[id];
      if (!entry) return;

      if (selectedId === id) {
        // Clicking the already-selected row deselects it
        entry.tr.classList.remove('tt-selected');
        selectedId = null;
      } else {
        entry.tr.classList.add('tt-selected');
        selectedId = id;
        if (onSelect) onSelect(entry.node);
      }
    }

    // ------------------------------------------------------------------ //
    // Recursively walk the node tree and append <tr> elements to tbody
    // autoIdCounter provides fallback ids for nodes that omit the id field.
    // ------------------------------------------------------------------ //
    var autoIdCounter = 0;
    function walkNodes(nodes, depth, parentId) {
      nodes.forEach(function (node) {
        // Assign an automatic id if the node does not supply one
        if (node.id == null || node.id === '') {
          node.id = 'tt-auto-' + autoIdCounter;
        }
        autoIdCounter += 1;

        var tr = document.createElement('tr');
        tr.className = 'tt-row';
        tr.setAttribute('data-tt-id', node.id);
        tr.setAttribute('data-tt-depth', depth);
        if (parentId !== null && parentId !== undefined) {
          tr.setAttribute('data-tt-parent', parentId);
        }

        // Build cells
        columns.forEach(function (colDef, colIndex) {
          var td = colIndex === 0
            ? buildFirstCell(node, depth, colDef)
            : buildCell(node, colDef);
          tr.appendChild(td);
        });

        // Row click — select unless the click was on the toggle icon
        tr.addEventListener('click', function (e) {
          if (e.target.closest('.tt-icon') && !e.target.closest('.tt-icon.tt-icon-leaf')) {
            return; // let the icon's own click handler handle expand/collapse
          }
          selectNode(node.id);
        });

        tbody.appendChild(tr);
        if (onRowCreated) onRowCreated(node, tr);

        // Register in nodeMap — warn if this id has already been seen
        if (nodeMap[node.id] !== undefined) {
          console.warn('TreeTable: duplicate node id "' + node.id + '" — only the first occurrence will be rendered correctly.');
        }
        var isExpanded = node.expanded === true;
        nodeMap[node.id] = {
          node: node,
          expanded: isExpanded,
          tr: tr,
          depth: depth,
          parentId: parentId !== undefined ? parentId : null
        };

        // Recurse into children
        if (node.children && node.children.length > 0) {
          walkNodes(node.children, depth + 1, node.id);
        }
      });
    }

    // ------------------------------------------------------------------ //
    // Initialise: build all rows, then apply initial visibility
    // ------------------------------------------------------------------ //
    walkNodes(data, 0, null);

    // Now that nodeMap is fully populated, set icons and initial visibility
    Object.keys(nodeMap).forEach(function (id) {
      var entry = nodeMap[id];
      updateIcon(entry);

      // Hide rows that should not be visible given initial expanded states
      if (!isVisible(entry)) {
        entry.tr.style.display = 'none';
      }
    });

    // ------------------------------------------------------------------ //
    // Public API
    // ------------------------------------------------------------------ //
    return {
      /**
       * Programmatically select the node with the given id.
       * @param {string} id
       */
      select: function (id) {
        selectNode(id);
      },

      /**
       * Deselect the currently selected row (if any).
       */
      deselect: function () {
        if (selectedId !== null) {
          var entry = nodeMap[selectedId];
          if (entry) entry.tr.classList.remove('tt-selected');
          selectedId = null;
        }
      },

      /**
       * Return the currently selected node object, or null.
       */
      getSelected: function () {
        if (selectedId === null) return null;
        var entry = nodeMap[selectedId];
        return entry ? entry.node : null;
      },

      /**
       * Expand the node with the given id.
       * @param {string} id
       */
      expand: function (id) {
        expandNode(id);
      },

      /**
       * Collapse the node with the given id.
       * @param {string} id
       */
      collapse: function (id) {
        collapseNode(id);
      },

      /**
       * Expand all nodes that have children.
       */
      expandAll: function () {
        Object.keys(nodeMap).forEach(function (id) {
          var entry = nodeMap[id];
          if (entry.node.children && entry.node.children.length > 0 && !entry.expanded) {
            // Expand without firing callbacks for each — fire once at the end
            entry.expanded = true;
            updateIcon(entry);
          }
        });
        // Show all rows
        Object.keys(nodeMap).forEach(function (id) {
          nodeMap[id].tr.style.display = '';
        });
      },

      /**
       * Filter rows by a string.
       * The table receives the class tt-filtered while a filter is active.
       * Rows whose cell text contains the string (case-insensitive) receive
       * tt-filter-match; unmatched rows are styled via the CSS rule
       * table.tt-filtered tr:not(.tt-filter-match).
       * Ancestors of matching rows are expanded automatically; those expansions
       * are tracked and reversed when the filter changes or is cleared.
       * Pass an empty string or null to clear the filter entirely.
       * @param {string|null} str
       */
      filter: function (str) {
        var term = str ? str.trim().toLowerCase() : '';

        // Always clear previous filter state first
        Object.keys(nodeMap).forEach(function (id) {
          nodeMap[id].tr.classList.remove('tt-filter-match');
        });

        if (term === '') {
          // Remove the filtered marker from the table
          table.classList.remove('tt-filtered');
          // Collapse auto-expanded nodes, but keep ancestors of the selected
          // row expanded so the selected row remains visible.
          var selAncestors = getSelectedAncestorIds();
          Object.keys(autoExpandedIds).forEach(function (id) {
            if (!selAncestors[id]) {
              collapseNodeSilent(id);
              delete autoExpandedIds[id];
            }
          });
          return;
        }

        // Determine which nodes match
        var matchingIds = {};
        Object.keys(nodeMap).forEach(function (id) {
          var entry = nodeMap[id];
          var text = '';
          var cells = entry.tr.querySelectorAll('.tt-cell-content');
          cells.forEach(function (cell) { text += cell.textContent + ' '; });
          if (text.toLowerCase().indexOf(term) !== -1) {
            matchingIds[id] = true;
          }
        });

        // Collect all ancestor ids that must be expanded to reveal matching rows
        var neededAncestorIds = {};
        Object.keys(matchingIds).forEach(function (id) {
          var parentId = nodeMap[id].parentId;
          while (parentId !== null && parentId !== undefined) {
            neededAncestorIds[parentId] = true;
            var parentEntry = nodeMap[parentId];
            if (!parentEntry) break;
            parentId = parentEntry.parentId;
          }
        });

        // Collapse previously auto-expanded nodes that are no longer needed,
        // but keep ancestors of the selected row expanded so it stays visible.
        var selAncestors = getSelectedAncestorIds();
        Object.keys(autoExpandedIds).forEach(function (id) {
          if (!neededAncestorIds[id] && !selAncestors[id]) {
            collapseNodeSilent(id);
            delete autoExpandedIds[id];
          }
        });

        // Expand needed ancestors that are not yet expanded
        Object.keys(neededAncestorIds).forEach(function (id) {
          var entry = nodeMap[id];
          if (entry && !entry.expanded) {
            expandNodeSilent(id);
            autoExpandedIds[id] = true;
          }
        });

        // Mark matching rows and activate the table-level filter marker
        Object.keys(matchingIds).forEach(function (id) {
          nodeMap[id].tr.classList.add('tt-filter-match');
        });
        table.classList.add('tt-filtered');
      },

      /**
       * Collapse all nodes that have children.
       */
      collapseAll: function () {
        Object.keys(nodeMap).forEach(function (id) {
          var entry = nodeMap[id];
          if (entry.node.children && entry.node.children.length > 0 && entry.expanded) {
            entry.expanded = false;
            updateIcon(entry);
          }
        });
        // Hide all non-root rows
        Object.keys(nodeMap).forEach(function (id) {
          var entry = nodeMap[id];
          if (entry.depth > 0) {
            entry.tr.style.display = 'none';
          }
        });
      }
    };
  }

  // Expose globally
  global.TreeTable = TreeTable;

}(typeof window !== 'undefined' ? window : this));
