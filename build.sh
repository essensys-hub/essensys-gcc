#!/usr/bin/env bash
# ============================================================
# build.sh — Essensys firmware build orchestrator
#
# Usage:
#   build.sh bp       Build BP firmware (MCF52259 ColdFire V2)
#   build.sh ba       Build BA firmware (PIC16F946) for all board variants
#   build.sh test     Run unit and integration tests (host GCC)
#   build.sh all      Run tests, then build BP and BA
#
# Expected workspace layout (mounted at /workspace):
#   /workspace/bp/           — BP firmware sources + Makefile
#   /workspace/ba/           — BA firmware sources + Makefile
#   /workspace/tests/bp/     — BP unit tests
#   /workspace/tests/ba/     — BA unit tests
#   /workspace/tests/integration/ — Integration tests
# ============================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE="${WORKSPACE:-/workspace}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info()  { echo -e "${GREEN}[INFO]${NC}  $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC}  $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*" >&2; }

check_tool() {
    if ! command -v "$1" &>/dev/null; then
        log_error "$1 not found in PATH"
        return 1
    fi
}

build_bp() {
    log_info "Building BP firmware (MCF52259 ColdFire V2)..."
    check_tool m68k-elf-gcc

    if [ ! -d "${WORKSPACE}/bp" ]; then
        log_error "BP source directory not found: ${WORKSPACE}/bp"
        exit 1
    fi

    make -C "${WORKSPACE}/bp"
    log_info "BP firmware build complete."
}

build_ba() {
    log_info "Building BA firmware (PIC16F946) for all board variants..."
    check_tool xc8-cc

    if [ ! -d "${WORKSPACE}/ba" ]; then
        log_error "BA source directory not found: ${WORKSPACE}/ba"
        exit 1
    fi

    make -C "${WORKSPACE}/ba" all-boards
    log_info "BA firmware build complete."
}

run_tests() {
    log_info "Running tests..."
    check_tool gcc

    local has_tests=false

    if [ -d "${WORKSPACE}/tests/bp" ]; then
        log_info "Running BP unit tests..."
        make -C "${WORKSPACE}/tests/bp" run_tests
        has_tests=true
    fi

    if [ -d "${WORKSPACE}/tests/ba" ]; then
        log_info "Running BA unit tests..."
        make -C "${WORKSPACE}/tests/ba" run_tests
        has_tests=true
    fi

    if [ -d "${WORKSPACE}/tests/integration" ]; then
        log_info "Running integration tests..."
        make -C "${WORKSPACE}/tests/integration" run_tests
        has_tests=true
    fi

    if [ "$has_tests" = false ]; then
        log_warn "No test directories found under ${WORKSPACE}/tests/"
    fi

    log_info "All tests complete."
}

usage() {
    echo "Usage: $(basename "$0") {bp|ba|test|all}"
    echo ""
    echo "Commands:"
    echo "  bp     Build BP firmware (MCF52259 ColdFire V2, m68k-elf-gcc)"
    echo "  ba     Build BA firmware (PIC16F946, xc8-cc) for all board variants"
    echo "  test   Run unit and integration tests (host GCC)"
    echo "  all    Run tests, then build BP and BA"
}

case "${1:-}" in
    bp)
        build_bp
        ;;
    ba)
        build_ba
        ;;
    test)
        run_tests
        ;;
    all)
        run_tests
        build_bp
        build_ba
        ;;
    *)
        usage
        exit 1
        ;;
esac
