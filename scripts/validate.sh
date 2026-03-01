#!/bin/bash
set -euo pipefail
echo "=== Validation firmware Essensys ==="

# Verifier la structure du repo
echo "--- Structure du depot ---"
for f in Dockerfile ct-ng.config build.sh Makefile bp/Makefile bp/intflash.ld bp/compat/codewarrior.h bp/compat/gcc_comp.h bp/compat/newlib_stubs.c bp/bsp/m52259evb ba/Makefile ba/source .github/workflows/firmware.yml; do
  if [ -e "$f" ]; then echo "  ✅ $f"; else echo "  ❌ $f MISSING"; fi
done

# Verifier les submodules
echo "--- Submodules ---"
git submodule status

# Verifier les tests
echo "--- Tests unitaires BA ---"
make -C tests/ba run_tests 2>&1 || echo "❌ Tests BA echoues"

echo "--- Tests integration ---"
make -C tests/integration run_tests 2>&1 || echo "❌ Tests integration echoues"

echo "=== Validation terminee ==="
