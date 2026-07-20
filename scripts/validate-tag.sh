#!/bin/bash
#
# validate-tag.sh — Validate firmware version tag format
#
# Usage:
#   ./scripts/validate-tag.sh fw-v099-38      (validate existing tag)
#   ./scripts/validate-tag.sh --list           (list all fw-v* tags)
#   ./scripts/validate-tag.sh --next           (suggest next version)
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# ============================================================================
# Functions
# ============================================================================

validate_format() {
    local tag="$1"

    # Check format: fw-vNNN-MM where NNN and MM are digits
    if [[ $tag =~ ^fw-v([0-9]{3})-([0-9]{2})$ ]]; then
        local major="${BASH_REMATCH[1]}"
        local minor="${BASH_REMATCH[2]}"
        echo "✓ Tag format valid: fw-v$major-$minor"
        return 0
    else
        echo "✗ Invalid tag format: $tag"
        echo "  Expected format: fw-vNNN-MM (e.g., fw-v099-38)"
        return 1
    fi
}

list_tags() {
    echo "Firmware version tags:"
    git tag -l "fw-v*" | sort -V || echo "  (no tags found)"
}

get_latest_tag() {
    git tag -l "fw-v*" | sort -V | tail -1
}

suggest_next_version() {
    local latest=$(get_latest_tag)

    if [ -z "$latest" ]; then
        echo "No previous tags found."
        echo "Suggested first tag: fw-v001-00"
        return 0
    fi

    # Parse latest tag
    if [[ $latest =~ ^fw-v([0-9]{3})-([0-9]{2})$ ]]; then
        local major="${BASH_REMATCH[1]}"
        local minor="${BASH_REMATCH[2]}"

        echo "Latest version: $latest"
        echo ""
        echo "Option 1 (minor bump): fw-v${major}-$((minor + 1))"
        echo "Option 2 (major bump): fw-v$((major + 1))-00"
    else
        echo "✗ Could not parse latest tag: $latest"
        return 1
    fi
}

# ============================================================================
# Main
# ============================================================================

case "${1:-}" in
    "")
        echo "Usage: $0 <tag | --list | --next>"
        echo ""
        echo "Examples:"
        echo "  $0 fw-v099-38           # Validate tag"
        echo "  $0 --list               # List all firmware tags"
        echo "  $0 --next               # Suggest next version"
        exit 1
        ;;

    "--list")
        list_tags
        ;;

    "--next")
        suggest_next_version
        ;;

    *)
        validate_format "$1"
        ;;
esac
