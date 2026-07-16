# Contributing to m-alloc

Thanks for taking the time to contribute. This document explains the workflow and the expectations for changes.

## Getting started

1. Fork the repository and clone your fork
2. Create a branch from `main`: `git checkout -b feat/my-change`
3. Build and test:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Requirements: CMake >= 3.25 and a C++23 compiler (GCC 13+, Clang 17+, MSVC 19.38+).

## Development workflow

- Run the sanitizer preset before opening a PR: `scripts/run-tests.sh asan`
- Format your changes: `scripts/format.sh --fix`
- New behavior needs tests; bug fixes need a regression test
- Keep zero warnings - CI builds with `-Werror` / `/WX` on GCC, Clang and MSVC

## Commit messages

The project uses [Conventional Commits](https://www.conventionalcommits.org):

```
feat: add decommit support for idle segments
fix: correct prevSize update after medium block split
docs: clarify aligned allocation limits
test: cover cross-thread reallocation
ci: pin clang-tidy version
```

Types in use: `feat`, `fix`, `docs`, `test`, `build`, `ci`, `perf`, `refactor`, `chore`.

## Code style

- C++23, formatted by the repository `.clang-format` and checked by `.clang-tidy`
- RAII for every resource; no raw owning pointers
- `std::expected` for fallible internal operations, `noexcept` where guaranteed
- `enum class` over plain enums, `constexpr` where possible
- No comments in source code - names and structure carry the meaning
- No macros except platform selection, no global mutable state

## Pull requests

- Keep PRs focused; unrelated changes belong in separate PRs
- Fill in the PR template, link related issues
- CI must be green (build matrix, tests, format, clang-tidy, CodeQL)
- A maintainer review is required before merge

## Reporting issues

Use the issue templates. For security vulnerabilities, follow [SECURITY.md](.github/SECURITY.md) instead of opening a public issue.

## Release process (maintainers)

1. Update `CHANGELOG.md` and the version in `CMakeLists.txt` and `include/MAlloc/Version.hpp`
2. Tag: `git tag vX.Y.Z && git push origin vX.Y.Z`
3. The release workflow builds artifacts and publishes the GitHub Release

Versioning follows [Semantic Versioning](https://semver.org).
