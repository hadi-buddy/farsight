# foresight-zh

**English** · [简体中文](README_zh.md)

Chinese (and mixed Chinese/English) transcription for Google AI Edge Foresight on macOS,
without modifying the app.

Unofficial and not affiliated with Google. No Google code or model weights are included: the
ASR bundle is built locally from the Gemma model Foresight itself downloads.

## Use

- Launch **`~/Applications/Foresight 中文.app`** (or run `~/tools/foresight-zh/launch.sh`).
  It quits a running Foresight and relaunches it with the shim.
- Launching Foresight any other way (Dock, Spotlight, login item, a notification) starts it
  *without* the shim. It still works, but transcribes English only.
- Log: `~/Library/Logs/foresight-zh/shim.log`. Debug run that also logs every transcript
  result: `FORESIGHT_ZH_TRACE=1 ~/tools/foresight-zh/launch.sh`.

## Setup

1. **Install Foresight and open it once.** Let it finish downloading its models. The build
   uses its Gemma 4 E2B download (`models/15`, about 3.7GB) in
   `~/Library/Application Support/com.google.AIEdgeForesight/models/`.
2. **Install Google's LiteRT-LM tools** (open source, used to unpack and repack `.litertlm` files):

   ```bash
   uv venv ~/local-llm/venvs/litert --python 3.12
   uv pip install -p ~/local-llm/venvs/litert/bin/python litert-lm-api litert-lm
   ```

3. **Build the Chinese ASR model:**

   ```bash
   model-build/build.sh
   ```

   This takes a few seconds and writes about 3.4GB to
   `~/local-llm/models/special/foresight-zh/zh_asr_gemma4e2b_v3.litertlm`. The script:
   - unpacks the Gemma 4 E2B file with `litert-lm-peek`
   - keeps the tokenizer, embedders, audio encoder, audio adapter, end-of-audio and decoder
     sections, and drops the vision sections and the speculative-decoding drafter
     (`model-build/model.toml`)
   - drops the `backend_constraint = "cpu"` tag on the audio sections, because Foresight runs
     its speech encoder on the GPU
   - replaces the chat template with a transcription-only one
     (`model-build/LlmMetadataProto.pbtext`)
   - repacks everything with `litert-lm-builder`

   Override paths with `LITERT_VENV=...` and `OUT=...`. If you write the model somewhere
   else, put its path in `~/tools/foresight-zh/asr_model_path` (one line) so the shim
   finds it. Quit Foresight before rebuilding: it memory-maps the model while running.

4. **Build the shim:**

   ```bash
   cd shim && clang -arch arm64 -dynamiclib -O2 -Wall -o libforesight_zh.dylib foresight_zh.c
   ```

5. **Optional, make a Dock/Spotlight launcher:**

   ```bash
   osacompile -o ~/Applications/"Foresight 中文.app" -e 'do shell script "$HOME/tools/foresight-zh/launch.sh"'
   ```

The shim and launcher expect this repo at `~/tools/foresight-zh`.

## How it works

Foresight's live transcription runs a separate on-device ASR model (`models/3`, English-only,
streaming USM encoder + small decoder) through Google's closed ODML transcription library,
which the Dart UI calls through an exported C API (`OdmlCreateInferenceEngine`,
`OdmlCreateSpeechPipelineFromProto`, `OdmlSpeechProcessorTranscribe`, ...).

`shim/libforesight_zh.dylib` is injected with `DYLD_INSERT_LIBRARIES`. The app's
entitlements allow that (`allow-dyld-environment-variables`, `disable-library-validation`),
so Google's code signature and keychain access are untouched. The shim:

1. **Model redirect.** It interposes `dlsym` (used by Dart FFI) and points the ASR engine at
   `~/local-llm/models/special/foresight-zh/zh_asr_gemma4e2b_v3.litertlm`. That file
   repackages the audio encoder, adapter and decoder of the Gemma 4 E2B model Foresight
   already downloads (`models/15`) into a transcription-only bundle. The audio encoder runs
   on the GPU, as the app requires.
2. **Options rewrite.** The app asks for streaming mode with interim results. Gemma's audio
   encoder is batch-only, which fails with "Streaming audio encoder is not available". The
   shim substitutes `pipeline_override.pb`: streaming off, VAD segmentation on, and a Gemma
   turn-format prompt that asks for Simplified Chinese with English kept as English.
   Regenerate it with `python3 make_override.py pipeline_override.pb`.
3. **Filter patch.** The Dart layer strips everything outside printable ASCII from transcripts
   (`[^\x20-\x7E\n\r]`), which deletes all CJK. The shim patches that regex constant in
   memory (copy-on-write page; the binary on disk is unchanged) to `[\x00-\x08\x7F]+`.

Text appears once per spoken phrase, after each pause, rather than word by word.

## When Foresight updates

The shim checks before acting and logs what it skipped:

- **Filter regex not found:** the patch is skipped and Chinese may get stripped again. Find
  the new filter string and update `patch_ascii_filter` in `shim/foresight_zh.c`.
- **Speech C API changed:** model redirect and options rewrite are tied to the current
  argument layout. Check `shim.log` for errors. Without the shim, the app is unaffected.
- **Gemma model id/path changed:** rebuild the ASR bundle with `model-build/build.sh`. It
  stops with "missing section" if the Gemma file layout changed; update `model.toml` to match.

## Files

- `shim/foresight_zh.c`: shim source; `shim/libforesight_zh.dylib`: built shim
- `launch.sh`: launcher; `~/Applications/Foresight 中文.app` wraps it
- `make_override.py`, `pipeline_override.pb`: speech-processor options substituted by the shim
- `model-build/`: recipe for the Gemma-based ASR bundle
- `~/local-llm/models/special/foresight-backup/`: Foresight's original ASR model (encrypted
  and decrypted copies), kept for reference
