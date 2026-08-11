# Security

## Reporting

Report suspected vulnerabilities through the repository's private GitHub vulnerability-reporting feature. If that feature is unavailable, open an issue containing no sensitive or exploit details and ask the maintainer to establish a private channel. Do not publish exploit details before a fix is available.

Include the affected revision, reproduction steps, security impact, and any relevant input artifact that can be shared legally.

## Operational guidance

Lifter parses untrusted binary formats and invokes complex native libraries. Run it with least privilege in an isolated environment, especially when analyzing unknown files. Do not execute generated output outside a controlled test environment until it has been independently reviewed.

Only analyze software you own or are authorized to inspect.
