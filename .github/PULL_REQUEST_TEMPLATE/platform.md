## 🧱 New Platform Contribution: Board / SoC Support

### 🆕 Summary
Briefly describe the new board or SoC being added.
Include vendor name, architecture, and key features.
Explain who would want to use this board and why.

### 📝 Completion of enablement
- [ ] I have fully completed the enablement of this platform by adding the DT nodes and trivial
      code for the ALL devices on this platform which *already* have support in zephyr and will
      not purposefully make multiple trailing PRs adding these things one at a time. (this is
      to avoid an overloading of reviewers' time and sanity).
- [ ] I have enabled testing of all the hardware features of the platform that I know I am
      enabling with this PR, and any others which are most relevant to this board use case.

### 🧪 Testing & Validation
- [ ] Verified workflow with `west build` and `west flash`
- [ ] Common kernel test passes on this platform
- [ ] Running twister targeting this platform has no build failures

### 📚 Documentation (if adding a board)
- [ ] `index.rst` created for board docs
- [ ] External datasheets or references linked
- [ ] Debugging and flashing instructions documented
- [ ] Supported feature list snippet added to doc page
- [ ] Board image added

### 🛠️ Maintenance
Check which of the following applies:
- [ ] This platform is being contributed by and maintained by it's vendor:
  - [ ] A maintenance area already exists for this vendor, and nothing more is needed.
  - [ ] A new maintainter area will need to be defined for this vendor.
- [ ] This platform is being contributed by and maintained by a third party:
  - [ ] The vendor has a maintenance area and agrees to maintain this new platform.
  - [ ] The vendor has a maintenance area and has not yet agreed to maintain this new platform.
  - [ ] A new maintenance area will be made for the specific platform and not maintained by vendor.
- [ ] The platform will not be actively maintained once added.

### 🧵 Notes
Any special considerations (e.g., multi-core SoC, TF-M support, secure/non-secure variants)

