#!/bin/bash
set -e

show_help() {
  echo "Usage: $0 [perf|valgrind]"
  echo "  perf:     Builds, records with perf, and displays report"
  echo "  valgrind: Builds, runs callgrind, and outputs viewing command"
  exit 0
}

if [ "$1" != "perf" ] && [ "$1" != "valgrind" ]; then
  show_help
fi

echo "=== Configuring build for profiling (RelWithDebInfo) ==="
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
cmake --build .
cd ..

if [ "$1" == "perf" ]; then
  echo "=== Running perf record ==="
  perf record -g ./build/tests/test_perf
  echo "=== Running perf report ==="
  perf report
elif [ "$1" == "valgrind" ]; then
  echo "=== Running valgrind --tool=callgrind ==="
  valgrind --tool=callgrind ./build/tests/test_perf
  echo "=== Finished ==="
  echo "To view the results, run:"
  echo "  kcachegrind callgrind.out.<pid>"
fi
