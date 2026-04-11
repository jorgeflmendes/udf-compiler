# Development Notes

## Environment

This project depends on the CDK toolchain distributed for the IST Compilers course. By default, the `Makefile` expects it under:

```text
${HOME}/compiladores/root
```

If your local installation differs, update the `ROOT` variable in the `Makefile`.

The course support packages are also available as binary packages for openSUSE through:

- [home:d4vid:co25/openSUSE_Tumbleweed](https://download.opensuse.org/repositories/home:/d4vid:/co25/openSUSE_Tumbleweed/)

Repository setup:

```bash
zypper ar https://download.opensuse.org/repositories/home:/d4vid:/co25/openSUSE_Tumbleweed/ CO25
zypper refresh
```

Package installation:

```bash
zypper install libcdk20-devel librts6-devel
```

These packages provide the course support libraries without requiring a manual build of the toolchain.

## Build

```bash
make
```

## Formatting

The repository includes a [.clang-format](../.clang-format) file that defines the intended formatting style for C++, Flex, and Bison edits.

## Rebuild

```bash
make rebuild
```

## Test Script

The repository includes a regression helper script:

```bash
./scripts/check-expected.sh
```

It expects a local `tests/` tree with `.udf` fixtures and matching expected outputs. If those fixtures are not present in your current copy of the repository, the script will not run successfully.

## Generated Artifacts

The following files are generated during normal development and should not be committed:

- parser tables (`*.tab.c`, `*.tab.h`)
- scanner output (`udf_scanner.cpp`)
- compiler binary (`udf`)
- object files and backend outputs (`*.o`, `*.asm`, `*.xml`)
- auto-generated declarations inside `.auto/`
