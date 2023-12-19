/**
 * Copyright (c) 2020-2023, The Godot community
 * Copyright (c) 2023, Benjamin Cabé <benjamin@zephyrproject.org>
 * SPDX-License-Identifier: CC-BY-3.0
 */


// Handle page scroll and adjust sidebar accordingly.

// Each page has two scrolls: the main scroll, which is moving the content of the page;
// and the sidebar scroll, which is moving the navigation in the sidebar.
// We want the logo to gradually disappear as the main content is scrolled, giving
// more room to the navigation on the left. This means adjusting the height
// available to the navigation on the fly.
const registerOnScrollEvent = (function(){
        // Configuration.

        // The number of pixels the user must scroll by before the logo is completely hidden.
        const scrollTopPixels = 156;
        // The target margin to be applied to the navigation bar when the logo is hidden.
        const menuTopMargin = 54;
        // The max-height offset when the logo is completely visible.
        const menuHeightOffset_default = 210;
        // The max-height offset when the logo is completely hidden.
        const menuHeightOffset_fixed = 63;
        // The distance between the two max-height offset values above; used for intermediate values.
        const menuHeightOffset_diff = (menuHeightOffset_default - menuHeightOffset_fixed);

        // Media query handler.
        return function(mediaQuery) {
          // We only apply this logic to the "desktop" resolution (defined by a media query at the bottom).
          // This handler is executed when the result of the query evaluation changes, which means that
          // the page has moved between "desktop" and "mobile" states.

          // When entering the "desktop" state, we register scroll events and adjust elements on the page.
          // When entering the "mobile" state, we clean up any registered events and restore elements on the page
          // to their initial state.

          const $window = $(window);
          const $sidebar = $('.wy-side-scroll');
          const $search = $sidebar.children('.wy-side-nav-search');
          const $menu = $sidebar.children('.wy-menu-vertical');

          if (mediaQuery.matches) {
            // Entering the "desktop" state.

            // The main scroll event handler.
            // Executed as the page is scrolled and once immediately as the page enters this state.
            const handleMainScroll = (currentScroll) => {
              if (currentScroll >= scrollTopPixels) {
                // After the page is scrolled below the threshold, we fix everything in place.
                $search.css('margin-top', `-${scrollTopPixels}px`);
                $menu.css('margin-top', `${menuTopMargin}px`);
                $menu.css('max-height', `calc(100% - ${menuHeightOffset_fixed}px)`);
              }
              else {
                // Between the top of the page and the threshold we calculate intermediate values
                // to guarantee a smooth transition.
                $search.css('margin-top', `-${currentScroll}px`);
                $menu.css('margin-top', `${menuTopMargin + (scrollTopPixels - currentScroll)}px`);

                if (currentScroll > 0) {
                  const scrolledPercent = (scrollTopPixels - currentScroll) / scrollTopPixels;
                  const offsetValue = menuHeightOffset_fixed + menuHeightOffset_diff * scrolledPercent;
                  $menu.css('max-height', `calc(100% - ${offsetValue}px)`);
                } else {
                  $menu.css('max-height', `calc(100% - ${menuHeightOffset_default}px)`);
                }
              }
            };

            // The sidebar scroll event handler.
            // Executed as the sidebar is scrolled as well as after the main scroll. This is needed
            // because the main scroll can affect the scrollable area of the sidebar.
            const handleSidebarScroll = () => {
              const menuElement = $menu.get(0);
              const menuScrollTop = $menu.scrollTop();
              const menuScrollBottom = menuElement.scrollHeight - (menuScrollTop + menuElement.offsetHeight);

              // As the navigation is scrolled we add a shadow to the top bar hanging over it.
              if (menuScrollTop > 0) {
                $search.addClass('fixed-and-scrolled');
              } else {
                $search.removeClass('fixed-and-scrolled');
              }
            };

            $search.addClass('fixed');

            $window.scroll(function() {
              handleMainScroll(window.scrollY);
              handleSidebarScroll();
            });

            $menu.scroll(function() {
              handleSidebarScroll();
            });

            handleMainScroll(window.scrollY);
            handleSidebarScroll();
          } else {
            // Entering the "mobile" state.

            $window.unbind('scroll');
            $menu.unbind('scroll');

            $search.removeClass('fixed');

            $search.css('margin-top', `0px`);
            $menu.css('margin-top', `0px`);
            $menu.css('max-height', 'initial');
          }
        };
      })();

      $(document).ready(() => {
        // Initialize handlers for page scrolling and our custom sidebar.
        const mediaQuery = window.matchMedia('only screen and (min-width: 769px)');

        registerOnScrollEvent(mediaQuery);
        mediaQuery.addListener(registerOnScrollEvent);
      });
