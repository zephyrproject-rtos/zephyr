/**
 * Simple search result scoring code.
 *
 * Copyright 2007-2018 by the Sphinx team
 * Copyright (c) 2019, Intel
 * SPDX-License-Identifier: Apache-2.0
 */

var Scorer = {
  // Implement the following function to further tweak the score for
  // each result The function takes a result array [filename, title,
  // anchor, descr, score] and returns the new score.

  // For Zephyr search results, push display down for kconfig, boards,
  // and samples so "regular" docs will show up before them

  score: function(result) {
    if (result[0].search("reference/kconfig/")>=0) {
       return -5;
    }
    else if (result[0].search("boards/")>=0) {
       return -5;
    }
    else if (result[0].search("samples/")>=0) {
       return -5;
    }
    else {
       return result[4];
    }
  },


  // query matches the full name of an object
  objNameMatch: 11,
  // or matches in the last dotted part of the object name
  objPartialMatch: 6,
  // Additive scores depending on the priority of the object
  objPrio: {0:  15,   // used to be importantResults
            1:  5,   // used to be objectResults
            2: -5},  // used to be unimportantResults
  //  Used when the priority is not in the mapping.
  objPrioDefault: 0,

  // query found in title
  title: 15,
  // query found in terms
  term: 5
};
