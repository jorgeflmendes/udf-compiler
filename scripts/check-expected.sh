#!/bin/bash

set -euo pipefail

TEST_BINARY="test"
TEST_ASSEMBLY="test.asm"
TEST_OBJECT="test.o"
TEST_OUTPUT="test.out"
RUNTIME_LIB_DIR="${HOME}/compiladores/root/usr/lib"

ulimit -t 30
ulimit -v 1048576
ulimit -f 1000
ulimit -c 0

passed_count=0
total_count=0

cleanup_artifacts() {
  rm -f "$TEST_ASSEMBLY" "$TEST_OBJECT" "$TEST_OUTPUT" "$TEST_BINARY"
}

for source_file in tests/*.udf; do
  ((++total_count))
  printf "%s:\t" "$source_file"

  cleanup_artifacts

  if ! ./udf -o "$TEST_ASSEMBLY" "$source_file" &>/dev/null; then
    echo "FAILED AT CODE GENERATION"
    exit 1
  fi

  if ! yasm -felf32 "$TEST_ASSEMBLY" &>/dev/null; then
    echo "FAILED AT ASSEMBLY"
    exit 1
  fi

  if ! ld -m elf_i386 -o "$TEST_BINARY" "$TEST_OBJECT" -lrts -L"$RUNTIME_LIB_DIR" &>/dev/null; then
    echo "FAILED AT LINKING"
    exit 1
  fi

  if ! ./"$TEST_BINARY" &>"$TEST_OUTPUT"; then
    echo "FAILED AT EXECUTION"
    echo "Re-running with Valgrind for diagnostics..."
    echo "====================== VALGRIND REPORT ======================"
    valgrind --leak-check=full --track-origins=yes ./"$TEST_BINARY"
    echo "============================================================="
    exit 1
  fi

  expected_output_file=$(basename -s .udf "$source_file")
  if ! diff -q "$TEST_OUTPUT" "tests/expected/${expected_output_file}.out" &>/dev/null; then
    echo "FAILED ON OUTPUT"
    echo "--- EXPECTED OUTPUT ---"
    cat "tests/expected/${expected_output_file}.out"
    echo "--- ACTUAL OUTPUT ---"
    cat "$TEST_OUTPUT"
    echo "----------------------"
    exit 1
  fi

  if ! valgrind --leak-check=full --error-exitcode=1 ./"$TEST_BINARY" &>/dev/null; then
    echo "FAILED VALGRIND CHECK"
    echo "Re-running with Valgrind for details..."
    echo "====================== VALGRIND REPORT ======================"
    valgrind --leak-check=full --track-origins=yes ./"$TEST_BINARY"
    echo "============================================================="
    exit 1
  fi

  echo "OK"
  ((++passed_count))
done

echo
echo "SUCCESS: All ${passed_count}/${total_count} tests passed."

cleanup_artifacts
