#!/bin/bash
# Use the same Python environment as this Mac's existing IDF build cache.
set -euo pipefail
cd "$(dirname "$0")/.."
PET_IDF_PATH="${IDF_PATH:-$HOME/.espressif/v5.3.5/esp-idf}"
source "$PET_IDF_PATH/export.sh" >/dev/null
PET_PYTHON="$HOME/.espressif/tools/python/v5.3.5/venv/bin/python"
if [[ ! -x "$PET_PYTHON" ]]; then PET_PYTHON="$(command -v python)"; fi
exec "$PET_PYTHON" "$PET_IDF_PATH/tools/idf.py" -C firmware "$@"
