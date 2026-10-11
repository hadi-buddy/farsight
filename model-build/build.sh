#!/bin/zsh
# Rebuilds zh_asr_gemma4e2b_v3.litertlm from Foresight's own Gemma 4 E2B download (models/15).
# Needs the litert venv: ~/local-llm/venvs/litert (pip: litert-lm-api litert-lm).
set -e
VENV=~/local-llm/venvs/litert/bin
SRC="$HOME/Library/Application Support/com.google.AIEdgeForesight/models/15/1.0/g4786b852942f4511d485.litertlm"
WORK=$(mktemp -d); HERE=${0:A:h}
$VENV/litert-lm-peek --litertlm_file "$SRC" --dump_files_dir "$WORK" >/dev/null
cp "$HERE/model.toml" "$HERE/LlmMetadataProto.pbtext" "$WORK/"
cd "$WORK" && $VENV/litert-lm-builder toml --path model.toml output --path ~/local-llm/models/special/foresight-zh/zh_asr_gemma4e2b_v3.litertlm
rm -rf "$WORK"
