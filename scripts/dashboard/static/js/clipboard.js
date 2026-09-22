/*
 * Generic copy-to-clipboard handler.
 *
 * Any element with a data-copy-target="<id>" attribute becomes a copy button.
 * On click it copies the textContent of the element with that id, then briefly
 * swaps .bi-copy for .bi-check2 to give visual feedback.
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

(function () {
  document.addEventListener('DOMContentLoaded', function () {
    document.querySelectorAll('[data-copy-target]').forEach(function (btn) {
      btn.addEventListener('click', function () {
        const text = document.getElementById(btn.dataset.copyTarget).textContent;
        navigator.clipboard.writeText(text).then(function () {
          const copyIcon  = btn.querySelector('.bi-copy');
          const checkIcon = btn.querySelector('.bi-check2');
          copyIcon.classList.add('d-none');
          checkIcon.classList.remove('d-none');
          setTimeout(function () {
            checkIcon.classList.add('d-none');
            copyIcon.classList.remove('d-none');
          }, 1500);
        });
      });
    });
  });
})();
