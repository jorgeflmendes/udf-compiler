# Contributing

## Development Principles

- Keep naming in English and consistent with the existing compiler architecture.
- Prefer small, focused changes.
- Avoid committing generated files.
- Preserve compatibility with the CDK-based build flow used in the course.

## Before Opening a Pull Request

1. Build the compiler with `make`.
2. Run the available regression checks if your local `tests/` fixtures are present.
3. Update documentation when language features or build steps change.

## Style

- C++ code uses 2-space indentation.
- Keep headers and implementations focused.
- Prefer descriptive names over abbreviations when editing semantic analysis or backend logic.
