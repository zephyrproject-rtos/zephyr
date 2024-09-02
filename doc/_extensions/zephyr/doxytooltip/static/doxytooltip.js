/**
 * Copyright (c) 2024, Benjamin Cabé <benjamin@zephyrproject.org>
 * SPDX-License-Identifier: Apache-2.0
 *
 * This script adds mouse hover hooks to the current page to display
 * Doxygen documentation as tooltips when hovering over a symbol in the
 * documentation.
 */

registerDoxygenTooltip = function () {
  const parser = new DOMParser();

  const tooltip = document.createElement('div');
  tooltip.id = 'doxytooltip';
  document.body.appendChild(tooltip);

  const tooltipShadow = document.createElement('div');
  tooltipShadow.id = 'doxytooltip-shadow';
  tooltip.attachShadow({ mode: 'open' }).appendChild(tooltipShadow);

  /* tippy's JS code automatically adds a <style> tag to the document's <head> with the
   * styles for the tooltips. We need to copy it to the tooltip's shadow DOM for it to apply to the
   * tooltip's content.
   */
  const tippyStyle = document.querySelector('style[data-tippy-stylesheet]');
  if (tippyStyle) {
    tooltipShadow.appendChild(tippyStyle.cloneNode(true));
    tippyStyle.remove();
  }

  /*
   * similarly, doxytooltip.css is added to the document's <head> by the Sphinx extension, but we
   * need it in the shadow DOM.
   */
  const doxytooltipStyle = document.querySelector('link[href*="doxytooltip.css"]');
  if (doxytooltipStyle) {
    tooltipShadow.appendChild(doxytooltipStyle.cloneNode(true));
    doxytooltipStyle.remove();
  }

  let firstTimeShowingTooltip = true;
  let links = Array.from(document.querySelectorAll('a.reference.internal')).filter((a) =>
    a.querySelector('code.c')
  );
  links.forEach((link) => {
    tippy(link, {
      content: "Loading...",
      allowHTML: true,
      maxWidth: '500px',
      placement: 'auto',
      trigger: 'mouseenter',
      touch: false,
      theme: 'doxytooltip',
      interactive: true,
      appendTo: () =>
        document
          .querySelector('#doxytooltip')
          .shadowRoot.getElementById('doxytooltip-shadow'),

      onShow(instance) {
        const url = link.getAttribute('href');
        const targetId = link.getAttribute('href').split('#')[1];

        fetch(url)
          .then((response) => response.text())
          .then((data) => {
            const parsedDoc = parser.parseFromString(data, 'text/html');

            if (firstTimeShowingTooltip) {
              /* Grab the main stylesheets from the Doxygen page and inject them in the tooltip's
                shadow DOM */
              const doxygenCSSNames = ['doxygen-awesome.css', 'doxygen-awesome-zephyr.css'];
              for (const cssName of doxygenCSSNames) {
                const cssLink = parsedDoc.querySelector(`link[href$="${cssName}"]`);
                if (cssLink) {
                  const link = document.createElement('link');
                  link.rel = 'stylesheet';
                  link.href = new URL(
                    cssLink.getAttribute('href'),
                    new URL(url, window.location.href)
                  );
                  tooltipShadow.appendChild(link);
                }
              }

              firstTimeShowingTooltip = false;
            }

            const codeClasses = link.getElementsByTagName('code')[0].classList;
            let content;
            if (codeClasses.contains('c-enumerator')) {
              const target = parsedDoc.querySelector(`tr td a[id="${targetId}"]`);
              if (target) {
                content = target.parentElement.nextElementSibling;
              }
            } else if (codeClasses.contains('c-struct')) {
              content = parsedDoc.querySelector(`#details ~ div.textblock`);
            } else {
              /* c-function, c-type, etc. */
              const target = parsedDoc.querySelector(
                `h2.memtitle span.permalink a[href="#${targetId}"]`
              );
              if (target) {
                content = target.closest('h2').nextElementSibling;
              }
            }

            if (content) {
              // rewrite all relative links as they are originally relative to the Doxygen page
              content.querySelectorAll('a').forEach((a) => {
                const href = a.getAttribute('href');
                if (href && !href.startsWith('http')) {
                  a.href = new URL(href, new URL(url, window.location.href));
                }
              });

              // set the tooltip content
              instance.setContent(content.cloneNode(true).innerHTML);
            } else {
              // don't show tooltip if no meaningful content was found (ex. no brief description)
              instance.setContent(null);
              return false;
            }
          })
          .catch((err) => {
            console.error('Failed to fetch the documentation:', err);
          });
      },
    });
  });
};

document.addEventListener('DOMContentLoaded', registerDoxygenTooltip);
