# Third-party software

Lifter does not vendor its runtime dependencies. The pinned vcpkg manifest resolves the complete dependency graph.

Direct runtime dependencies:

- LLVM and LLD: Apache-2.0 with LLVM exception.
- LIEF: Apache-2.0.
- Zydis: MIT.
- toml++: MIT.

Catch2 is a test-only dependency under BSL-1.0.

The list above is a summary, not a substitute for the license texts. A binary distributor must include the applicable licenses, copyright notices, and required notices for the complete resolved dependency graph, including transitive static dependencies. vcpkg installs authoritative package copyright files under `vcpkg_installed/<triplet>/share/<port>/copyright`; release packaging must collect the relevant files from the exact resolved build.
