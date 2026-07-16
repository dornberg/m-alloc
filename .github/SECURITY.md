# Security Policy

## Supported versions

| Version | Supported |
|---|---|
| 0.1.x | yes |

## Reporting a vulnerability

Memory allocators are security-sensitive. If you find a vulnerability (heap corruption exploitable through the API, metadata disclosure, bypass of debug protections), please report it privately:

1. Use [GitHub private vulnerability reporting](https://github.com/dornberg/m-alloc/security/advisories/new)
2. Do not open a public issue or discuss the problem publicly before a fix is released

Include:

- Affected version or commit
- Platform and compiler
- A minimal reproduction
- Impact assessment as you understand it

## What to expect

- Acknowledgement within 72 hours
- A fix or mitigation plan within 30 days for confirmed reports
- Credit in the release notes unless you prefer to stay anonymous

## Scope

Bugs that require already-corrupted input pointers (arbitrary invalid frees from an attacker who can already call arbitrary code) are generally treated as regular bugs, not vulnerabilities. Hardening improvements are still welcome.
