/*
 * Support functions for the memory report view.
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

(function () {

  function initTree(treeId, treeData) {
    var tt = TreeTable(treeId, {
      data: treeData?.tree,
      columns: [
        // Column 1: Symbol name.
        {
          render: function (node) {
            let html = node.data.name;
            if (node.data.memoryType) {
              html += `<span class="memory-type">${node.data.memoryType}</span>`;
            }
            return html;
          },
          htmlContent: true,
        },
        // Column 2: Size in bytes.
        {
          render: function (node) {
            return node.data.displaySize;
          },
          className: "text-end"
        },
        // Column 3: Size in percent.
        {
          render: function (node) {
            return `${(node.data.size * 100.0 / treeData.size).toFixed(2)}%`;
          },
          className: "text-end"
        },
      ],
      onRowCreated: function (node, tr) {
        // Do not color the (hidden) and (no path) nodes.
        if (node.data.name.startsWith('(') && node.data.name.endsWith(')')) {
          return;
        }
        if (!node.children) {
          tr.classList.add('mem-rpt-symbol');
        }
        else if (node.data.name.endsWith('.c')) {
          tr.classList.add('mem-rpt-file');
        }

      },
      icons: {
        expand: 'bi-caret-right-fill text-secondary',
        collapse: 'bi-caret-down-fill text-secondary',
        node: function (node, state) {
          if (!node.children) {
            return 'bi-file-binary';
          }
          else if (node.data.name.endsWith('.c')) {
            return 'bi-file-earmark';
          }
          return 'bi-folder';
        }
      }
    });
  }

  document.addEventListener("DOMContentLoaded", function(event) {
    initTree("ramTree", ramReport);
    initTree("romTree", romReport);
    initTree("allTree", allReport);
  });
})();
