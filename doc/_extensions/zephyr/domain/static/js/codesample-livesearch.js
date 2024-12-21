/**
 * Copyright (c) 2024, The Linux Foundation.
 * SPDX-License-Identifier: Apache-2.0
 */

function filterSamples(input) {
  const searchQuery = input.value.toLowerCase();
  const container = input.closest(".code-sample-listing");

  function removeHighlights(element) {
    const marks = element.querySelectorAll("mark");
    marks.forEach((mark) => {
      const parent = mark.parentNode;
      while (mark.firstChild) {
        parent.insertBefore(mark.firstChild, mark);
      }
      parent.removeChild(mark);
      parent.normalize(); // Merge adjacent text nodes
    });
  }

  function highlightMatches(node, query) {
    if (node.nodeType === Node.TEXT_NODE) {
      const text = node.textContent;
      const index = text.toLowerCase().indexOf(query);
      if (index !== -1 && query.length > 0) {
        const highlightedFragment = document.createDocumentFragment();

        const before = document.createTextNode(text.substring(0, index));
        const highlight = document.createElement("mark");
        highlight.textContent = text.substring(index, index + query.length);
        const after = document.createTextNode(text.substring(index + query.length));

        highlightedFragment.appendChild(before);
        highlightedFragment.appendChild(highlight);
        highlightedFragment.appendChild(after);

        node.parentNode.replaceChild(highlightedFragment, node);
      }
    } else if (node.nodeType === Node.ELEMENT_NODE) {
      node.childNodes.forEach((child) => highlightMatches(child, query));
    }
  }

  function processSection(section) {
    let sectionVisible = false;
    const lists = section.querySelectorAll(":scope > ul.code-sample-list");
    const childSections = section.querySelectorAll(":scope > section");

    // Process lists directly under this section
    lists.forEach((list) => {
      let listVisible = false;
      const items = list.querySelectorAll("li");

      items.forEach((item) => {
        const nameElement = item.querySelector(".code-sample-name");
        const descElement = item.querySelector(".code-sample-description");

        removeHighlights(nameElement);
        removeHighlights(descElement);

        const sampleName = nameElement.textContent.toLowerCase();
        const sampleDescription = descElement.textContent.toLowerCase();

        if (
          sampleName.includes(searchQuery) ||
          sampleDescription.includes(searchQuery)
        ) {
          if (searchQuery) {
            highlightMatches(nameElement, searchQuery);
            highlightMatches(descElement, searchQuery);
          }

          item.style.display = "";
          listVisible = true;
          sectionVisible = true;
        } else {
          item.style.display = "none";
        }
      });

      // Show or hide the list based on whether any items are visible
      list.style.display = listVisible ? "" : "none";
    });

    // Recursively process child sections
    childSections.forEach((childSection) => {
      const childVisible = processSection(childSection);
      if (childVisible) {
        sectionVisible = true;
      }
    });

    // Show or hide the section heading based on visibility
    const heading = section.querySelector(
      ":scope > h2, :scope > h3, :scope > h4, :scope > h5, :scope > h6"
    );
    if (sectionVisible) {
      if (heading) heading.style.display = "";
      section.style.display = "";
    } else {
      if (heading) heading.style.display = "none";
      section.style.display = "none";
    }

    return sectionVisible;
  }

  // Start processing from the container
  processSection(container);

  // Ensure the input and its container are always visible
  input.style.display = "";
  container.style.display = "";
}
