#!/bin/bash
# Setup do ambiente virtual para backup do UV-K5
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

if [ ! -d ".venv" ]; then
    python3 -m venv .venv
fi

source .venv/bin/activate
pip install -q pyserial

echo ""
echo "Ambiente pronto. Rodando backup..."
echo ""
python3 "$SCRIPT_DIR/uvk5_full_backup.py" "$@"
