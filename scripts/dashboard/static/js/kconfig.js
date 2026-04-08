/*
 * Support functions for the kconfig view.
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

(function () {

  var input;
  var table;
  var tr;

  function filterKconfigs() {
    // Get the current filter value and normalize to upper case.
    let filter = input.value.toUpperCase();

    // Loop through all table rows, and hide those that do not
    // match the search query.
    for (var i = 0; i < tr.length; i++) {
      let td = tr[i].getElementsByTagName("td")[1];
      if (td) {
        let txtValue = td.textContent || td.innerText;
        if (txtValue.toUpperCase().indexOf(filter) > -1) {
          tr[i].style.display = "";
        } else {
          tr[i].style.display = "none";
        }
      }
    }
  }

  document.addEventListener("DOMContentLoaded", function(event) {
    input = document.getElementById("kconfigFilter");
    input.addEventListener('input', filterKconfigs);
    table = document.getElementById("kconfigTable");
    tr = table.getElementsByTagName("tr");
  });
})();
