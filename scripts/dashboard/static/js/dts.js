/*
 * Support functions for the dts view.
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

(function () {

  var table = null;
  var tree = null;

  function safeHTML(text) {
    let div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
  }

  function renderName(node) {
    let edtNode = node.edtNode;
    let html = '';
    if (edtNode.labels?.length) {
      html = `<span class="text-secondary">${edtNode.labels[0]}:</span> `;
    }
    if (!edtNode.isProperty) {
      html += `<span id="${node.id}" class="bold-lt">${edtNode.name}</span>`;
    }
    else {
      html += edtNode.name;
    }

    return html;
  }

  function renderValue(node) {
    let edtNode = node.edtNode;
    let html = '';

    // Property Value
    if (edtNode.isProperty) {
      // Convert any references into links to the relevant node.
      let propHtml = safeHTML(edtNode.value).replace(/&amp;(\w+)/g, (match, label) => {
        let path = edtTree.label2path[label];
        return `<a href="#${path}" class="node-ref" title="${path}">&amp;${label}</a>`;
      });
      html += `<div class="dt-prop-value">${propHtml}</div>`;
    }

    html += `<div class="dt-prop-info">`;

    // Description
    if (edtNode.description) {
      html += `<div>${safeHTML(edtNode.description)}</div>`;
    }

    // Type from the specification
    if (edtNode.typeSpec) {
      html += `<i>${safeHTML(edtNode.typeSpec)}</i>`;
    }

    // Location
    if (edtNode.filename) {
      if (edtNode.typeSpec) {
        html += ' at ';
      }
      html += `<span class="text-muted">${edtNode.filename}:${edtNode.lineno}</span>`;
    }

    html += `</div>`;

    return html;
  }

  function initTree(treeId, treeData) {
    var tt = TreeTable(treeId, {
      data: treeData,
      columns: [
        {
          render: renderName,
          htmlContent: true,
          className: "text-nowrap"
        },
        {
          render: renderValue,
          htmlContent: true
        },
      ],
      icons: {
        expand: 'bi-caret-right-fill text-secondary',
        collapse: 'bi-caret-down-fill text-secondary',
        node: function (node, state) {
          return node.children ? 'bi-folder' : 'bi-file-code';
        }
      }
    });
    return tt;
  }

  function jumpToNode (event) {
    let nodeId = event.target.attributes.href.value.substring(1);
    tree.select(nodeId);

    // Stop propagation so the click does not trigger activating the node
    // with the phandle link - we want to select the target node instead.
    event.stopPropagation();
  }

  function toggleAllInfo (event) {
    if (this.checked) {
      table.classList.add('show-info');
    }
    else {
      table.classList.remove('show-info');
    }
  }

  function search (event) {
    let value = event.target.value;

    // Skip filtering for just a single character.
    if (value.length === 1) {
      return;
    }
    tree.filter(value);
  }

  document.addEventListener("DOMContentLoaded", function(event) {
    table = document.getElementById("edtTree");
    tree = initTree(table, edtTree.tree);
    table.addEventListener('click', function (event) {
      if (event.target.classList.contains('node-ref')) {
        jumpToNode(event);
      }
    });
    document.getElementById('showAllInfo').addEventListener('click', toggleAllInfo);
    document.getElementById('search').addEventListener('input', search);
  });
})();
