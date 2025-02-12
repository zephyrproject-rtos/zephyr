/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

const DB_FILE = 'kconfig.json';
const RESULTS_PER_PAGE_OPTIONS = [10, 25, 50];

/* search state */
let db;
let searchOffset;
let maxResults = RESULTS_PER_PAGE_OPTIONS[0];

/* elements */
let input;
let searchTools;
let summaryText;
let results;
let navigation;
let navigationPagesText;
let navigationPrev;
let navigationNext;

/**
 * Show an error message.
 * @param {String} message Error message.
 */
function showError(message) {
    const admonition = document.createElement('div');
    admonition.className = 'admonition error';
    results.replaceChildren(admonition);

    const admonitionTitle = document.createElement('p');
    admonitionTitle.className = 'admonition-title';
    admonition.appendChild(admonitionTitle);

    const admonitionTitleText = document.createTextNode('Error');
    admonitionTitle.appendChild(admonitionTitleText);

    const admonitionContent = document.createElement('p');
    admonition.appendChild(admonitionContent);

    const admonitionContentText = document.createTextNode(message);
    admonitionContent.appendChild(admonitionContentText);
}

/**
 * Show a progress message.
 * @param {String} message Progress message.
 */
function showProgress(message) {
    const p = document.createElement('p');
    p.className = 'centered';
    results.replaceChildren(p);

    const pText = document.createTextNode(message);
    p.appendChild(pText);
}

/**
 * Render a Kconfig literal property.
 * @param {Element} parent Parent element.
 * @param {String} title Title.
 * @param {String} content Content.
 */
function renderKconfigPropLiteral(parent, title, content) {
    const term = document.createElement('dt');
    parent.appendChild(term);

    const termText = document.createTextNode(title);
    term.appendChild(termText);

    const details = document.createElement('dd');
    parent.appendChild(details);

    const code = document.createElement('code');
    code.className = 'docutils literal';
    details.appendChild(code);

    const literal = document.createElement('span');
    literal.className = 'pre';
    code.appendChild(literal);

    const literalText = document.createTextNode(content);
    literal.appendChild(literalText);
}

/**
 * Render a Kconfig list property.
 * @param {Element} parent Parent element.
 * @param {String} title Title.
 * @param {list} elements List of elements.
 * @param {boolean} linkElements Whether to link elements (treat each element
 *                  as an unformatted option)
 */
function renderKconfigPropList(parent, title, elements, linkElements) {
    if (elements.length === 0) {
        return;
    }

    const term = document.createElement('dt');
    parent.appendChild(term);

    const termText = document.createTextNode(title);
    term.appendChild(termText);

    const details = document.createElement('dd');
    parent.appendChild(details);

    const list = document.createElement('ul');
    list.className = 'simple';
    details.appendChild(list);

    elements.forEach(element => {
        const listItem = document.createElement('li');
        list.appendChild(listItem);

        if (linkElements) {
            const link = document.createElement('a');
            link.href = '#' + element;
            listItem.appendChild(link);

            const linkText = document.createTextNode(element);
            link.appendChild(linkText);
        } else {
            /* using HTML since element content is pre-formatted */
            listItem.innerHTML = element;
        }
    });
}

/**
 * Render a Kconfig list property.
 * @param {Element} parent Parent element.
 * @param {list} elements List of elements.
 * @returns
 */
function renderKconfigDefaults(parent, defaults, alt_defaults) {
    if (defaults.length === 0 && alt_defaults.length === 0) {
        return;
    }

    const term = document.createElement('dt');
    parent.appendChild(term);

    const termText = document.createTextNode('Defaults');
    term.appendChild(termText);

    const details = document.createElement('dd');
    parent.appendChild(details);

    if (defaults.length > 0) {
        const list = document.createElement('ul');
        list.className = 'simple';
        details.appendChild(list);

        defaults.forEach(entry => {
            const listItem = document.createElement('li');
            list.appendChild(listItem);

            /* using HTML since default content may be pre-formatted */
            listItem.innerHTML = entry;
        });
    }

    if (alt_defaults.length > 0) {
        const list = document.createElement('ul');
        list.className = 'simple';
        list.style.display = 'none';
        details.appendChild(list);

        alt_defaults.forEach(entry => {
            const listItem = document.createElement('li');
            list.appendChild(listItem);

            /* using HTML since default content may be pre-formatted */
            listItem.innerHTML = `
                ${entry[0]}
                <em>at</em>
                <code class="docutils literal">
                    <span class"pre">${entry[1]}</span>
                </code>`;
        });

        const show = document.createElement('a');
        show.onclick = () => {
            if (list.style.display === 'none') {
                list.style.display = 'block';
            } else {
                list.style.display = 'none';
            }
        };
        details.appendChild(show);

        const showText = document.createTextNode('Show/Hide other defaults');
        show.appendChild(showText);
    }
}

/**
 * Render a Kconfig entry.
 * @param {Object} entry Kconfig entry.
 */
function renderKconfigEntry(entry) {
    const container = document.createElement('dl');
    container.className = 'kconfig';

    /* title (name and permalink) */
    const title = document.createElement('dt');
    title.className = 'sig sig-object';
    container.appendChild(title);

    const name = document.createElement('span');
    name.className = 'pre';
    title.appendChild(name);

    const nameText = document.createTextNode(entry.name);
    name.appendChild(nameText);

    const permalink = document.createElement('a');
    permalink.className = 'headerlink';
    permalink.href = '#' + entry.name;
    title.appendChild(permalink);

    const permalinkText = document.createTextNode('\uf0c1');
    permalink.appendChild(permalinkText);

    /* details */
    const details = document.createElement('dd');
    container.append(details);

    /* prompt and help */
    const prompt = document.createElement('p');
    details.appendChild(prompt);

    const promptTitle = document.createElement('em');
    prompt.appendChild(promptTitle);

    const promptTitleText = document.createTextNode('');
    promptTitle.appendChild(promptTitleText);
    if (entry.prompt) {
        promptTitleText.nodeValue = entry.prompt;
    } else {
        promptTitleText.nodeValue = 'No prompt - not directly user assignable.';
    }

    if (entry.help) {
        const help = document.createElement('p');
        details.appendChild(help);

        const helpText = document.createTextNode(entry.help);
        help.appendChild(helpText);
    }

    /* symbol properties (defaults, selects, etc.) */
    const props = document.createElement('dl');
    props.className = 'field-list simple';
    details.appendChild(props);

    renderKconfigPropLiteral(props, 'Type', entry.type);
    if (entry.dependencies) {
        renderKconfigPropList(props, 'Dependencies', [entry.dependencies]);
    }
    renderKconfigDefaults(props, entry.defaults, entry.alt_defaults);
    renderKconfigPropList(props, 'Selects', entry.selects, false);
    renderKconfigPropList(props, 'Selected by', entry.selected_by, true);
    renderKconfigPropList(props, 'Implies', entry.implies, false);
    renderKconfigPropList(props, 'Implied by', entry.implied_by, true);
    renderKconfigPropList(props, 'Ranges', entry.ranges, false);
    renderKconfigPropList(props, 'Choices', entry.choices, false);
    renderKconfigPropLiteral(props, 'Location', `${entry.filename}:${entry.linenr}`);
    renderKconfigPropLiteral(props, 'Menu path', entry.menupath);

    return container;
}

/** Perform a search and display the results. */
function doSearch() {
    /* replace current state (to handle back button) */
    history.replaceState({
        value: input.value,
        searchOffset: searchOffset
    }, '', window.location);

    /* nothing to search for */
    if (!input.value) {
        results.replaceChildren();
        navigation.style.visibility = 'hidden';
        searchTools.style.visibility = 'hidden';
        return;
    }

    /* perform search */
    const regexes = input.value.trim().split(/\s+/).map(
        element => new RegExp(element.toLowerCase())
    );
    let count = 0;

    const searchResults = db.filter(entry => {
        let matches = 0;
        const name = entry.name.toLowerCase();
        const prompt = entry.prompt ? entry.prompt.toLowerCase() : "";

        regexes.forEach(regex => {
            if (name.search(regex) >= 0 || prompt.search(regex) >= 0) {
                matches++;
            }
        });

        if (matches === regexes.length) {
            count++;
            if (count > searchOffset && count <= (searchOffset + maxResults)) {
                return true;
            }
        }

        return false;
    });

    /* show results count and search tools */
    summaryText.nodeValue = `${count} options match your search.`;
    searchTools.style.visibility = 'visible';

    /* update navigation */
    navigation.style.visibility = 'visible';
    navigationPrev.disabled = searchOffset - maxResults < 0;
    navigationNext.disabled = searchOffset + maxResults > count;

    const currentPage = Math.floor(searchOffset / maxResults) + 1;
    const totalPages = Math.floor(count / maxResults) + 1;
    navigationPagesText.nodeValue = `Page ${currentPage} of ${totalPages}`;

    /* render Kconfig entries */
    results.replaceChildren();
    searchResults.forEach(entry => {
        results.appendChild(renderKconfigEntry(entry));
    });
}

/** Do a search from URL hash */
function doSearchFromURL() {
    const rawOption = window.location.hash.substring(1);
    if (!rawOption) {
        return;
    }

    const option = decodeURIComponent(rawOption);
    if (option.startsWith('!')) {
        input.value = option.substring(1);
    } else {
        input.value = '^' + option + '$';
    }

    searchOffset = 0;
    doSearch();
}

function setupKconfigSearch() {
    /* populate kconfig-search container */
    const container = document.getElementById('__kconfig-search');
    if (!container) {
        console.error("Couldn't find Kconfig search container");
        return;
    }

    /* create input field */
    const inputContainer = document.createElement('div');
    inputContainer.className = 'input-container'
    container.appendChild(inputContainer)

    input = document.createElement('input');
    input.placeholder = 'Type a Kconfig option name (RegEx allowed)';
    input.type = 'text';
    inputContainer.appendChild(input);

    const copyLinkButton = document.createElement('button');
    copyLinkButton.title = "Copy link to results";
    copyLinkButton.onclick = () => {
        if (!window.isSecureContext) {
            console.error("Cannot copy outside of a secure context");
            return;
        }

        const copyURL = window.location.protocol + '//' + window.location.host +
        window.location.pathname + '#!' + input.value;

        navigator.clipboard.writeText(encodeURI(copyURL));
    }
    inputContainer.appendChild(copyLinkButton)

    const copyLinkText = document.createTextNode('🔗');
    copyLinkButton.appendChild(copyLinkText);

    /* create search tools container */
    searchTools = document.createElement('div');
    searchTools.className = 'search-tools';
    searchTools.style.visibility = 'hidden';
    container.appendChild(searchTools);

    /* create search summary */
    const searchSummaryContainer = document.createElement('div');
    searchTools.appendChild(searchSummaryContainer);

    const searchSummary = document.createElement('p');
    searchSummaryContainer.appendChild(searchSummary);

    summaryText = document.createTextNode('');
    searchSummary.appendChild(summaryText);

    /* create results per page selector */
    const resultsPerPageContainer = document.createElement('div');
    resultsPerPageContainer.className = 'results-per-page-container';
    searchTools.appendChild(resultsPerPageContainer);

    const resultsPerPageTitle = document.createElement('span');
    resultsPerPageTitle.className = 'results-per-page-title';
    resultsPerPageContainer.appendChild(resultsPerPageTitle);

    const resultsPerPageTitleText = document.createTextNode('Results per page:');
    resultsPerPageTitle.appendChild(resultsPerPageTitleText);

    const resultsPerPageSelect = document.createElement('select');
    resultsPerPageSelect.onchange = (event) => {
        maxResults = parseInt(event.target.value);
        searchOffset = 0;
        doSearch();
    }
    resultsPerPageContainer.appendChild(resultsPerPageSelect);

    RESULTS_PER_PAGE_OPTIONS.forEach((value, index) => {
        const option = document.createElement('option');
        option.value = value;
        option.text = value;
        option.selected = index === 0;
        resultsPerPageSelect.appendChild(option);
    });

    /* create search results container */
    results = document.createElement('div');
    container.appendChild(results);

    /* create search navigation */
    navigation = document.createElement('div');
    navigation.className = 'search-nav';
    navigation.style.visibility = 'hidden';
    container.appendChild(navigation);

    navigationPrev = document.createElement('button');
    navigationPrev.className = 'btn';
    navigationPrev.disabled = true;
    navigationPrev.onclick = () => {
        searchOffset -= maxResults;
        doSearch();
        window.scroll(0, 0);
    }
    navigation.appendChild(navigationPrev);

    const navigationPrevText = document.createTextNode('Previous');
    navigationPrev.appendChild(navigationPrevText);

    const navigationPages = document.createElement('p');
    navigation.appendChild(navigationPages);

    navigationPagesText = document.createTextNode('');
    navigationPages.appendChild(navigationPagesText);

    navigationNext = document.createElement('button');
    navigationNext.className = 'btn';
    navigationNext.disabled = true;
    navigationNext.onclick = () => {
        searchOffset += maxResults;
        doSearch();
        window.scroll(0, 0);
    }
    navigation.appendChild(navigationNext);

    const navigationNextText = document.createTextNode('Next');
    navigationNext.appendChild(navigationNextText);

    /* load database */
    showProgress('Loading database...');

    fetch(DB_FILE)
        .then(response => response.json())
        .then(json => {
            db = json;

            results.replaceChildren();

            /* perform initial search */
            doSearchFromURL();

            /* install event listeners */
            input.addEventListener('input', () => {
                searchOffset = 0;
                doSearch();
            });

            /* install hash change listener (for links) */
            window.addEventListener('hashchange', doSearchFromURL);

            /* handle back/forward navigation */
            window.addEventListener('popstate', (event) => {
                if (!event.state) {
                    return;
                }

                input.value = event.state.value;
                searchOffset = event.state.searchOffset;
                doSearch();
            });
        })
        .catch(error => {
            showError(`Kconfig database could not be loaded (${error})`);
        });
}

setupKconfigSearch();
