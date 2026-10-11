# foresight-zh

**English** · [简体中文](README_zh.md)

Chinese (and mixed Chinese/English) transcription for Google AI Edge Foresight on macOS,
without modifying the app.

Unofficial and not affiliated with Google. No Google code or model weights are included.

## Use

- Launch **`~/Applications/Foresight 中文.app`** (or run `~/tools/foresight-zh/launch.sh`).
  It quits a running Foresight and relaunches it with the shim.
- Launching Foresight any other way (Dock, Spotlight, login item, a notification) starts it
  *without* the shim. It still works, but transcribes English only.
- Text appears once per spoken phrase, after each pause, rather than word by word.
- Log: `~/Library/Logs/foresight-zh/shim.log`. Debug run that also logs every transcript
  result: `FORESIGHT_ZH_TRACE=1 ~/tools/foresight-zh/launch.sh`.

## Setup

The shim and launcher expect this repo at `~/tools/foresight-zh`.

1. Install Foresight and open it once.
2. Download the transcription engine, about 270MB: the
   [sherpa-onnx](https://github.com/k2-fsa/sherpa-onnx) C library,
   SenseVoice-Small (int8) and Silero VAD. They go to `~/local-llm/models/special/foresight-zh/`.

   ```bash
   ./setup-sensevoice.sh
   ```

3. Build the shim:

   ```bash
   shim/build.sh
   ```

4. Optional, make a Dock/Spotlight launcher:

   ```bash
   osacompile -o ~/Applications/"Foresight 中文.app" -e 'do shell script "$HOME/tools/foresight-zh/launch.sh"'
   ```

## How it works

Foresight's live transcription runs through Google's closed ODML transcription library. The
Dart UI calls it through an exported C API (`OdmlCreateInferenceEngine`,
`OdmlCreateSpeechPipelineFromProto`, `OdmlSpeechProcessorTranscribe`,
`OdmlSpeechProcessorFinishStream`, ...). Its model (`models/3`) is English-only.

`shim/libforesight_zh.dylib` is injected with `DYLD_INSERT_LIBRARIES`. The app's
entitlements allow that (`allow-dyld-environment-variables`, `disable-library-validation`),
so Google's code signature and keychain access are untouched. The shim does two things:

1. **Replaces speech recognition.** It interposes `dlsym`, which Dart FFI uses to find the C
   API, and wraps `OdmlSpeechProcessorTranscribe` and `OdmlSpeechProcessorFinishStream`.
   The audio Foresight passes in (48kHz float, one stream for the mic and one for system
   audio) is resampled to 16kHz and split into phrases with Silero VAD. Each phrase is
   transcribed with SenseVoice-Small on the CPU, typically in 15–85ms. The text goes back in
   Foresight's own result format (`text`, `is_final`, start/end in microseconds).
   Foresight's English engine still loads but never runs. See `shim/sensevoice_engine.c`.
2. **Removes the ASCII filter.** The Dart layer strips everything outside printable ASCII
   from transcripts (`[^\x20-\x7E\n\r]`), which deletes all Chinese. The shim patches that
   regex constant in memory (copy-on-write page; the binary on disk is unchanged) to
   `[\x00-\x08\x7F]+`.

If the SenseVoice files are missing or fail to load, the shim falls back to Foresight's own
engine (English).

### Alternative engine: Gemma 4 E2B

Before SenseVoice, the shim ran Foresight's own pipeline on a repack of the Gemma 4 E2B
model that Foresight downloads. That approach redirects the model path and rewrites the
pipeline options for batch decoding with a transcription prompt. It works, but needs a 3.4GB
copy of Gemma and more memory. To use it:

```bash
uv venv ~/local-llm/venvs/litert --python 3.12
uv pip install -p ~/local-llm/venvs/litert/bin/python litert-lm-api litert-lm
model-build/build.sh                       # repacks models/15 into the ASR bundle
echo gemma > ~/tools/foresight-zh/engine   # switch engines; delete the file to go back
```

`model-build/build.sh` drops the vision and drafter sections and the CPU-only tag on the
audio sections, because Foresight runs speech on the GPU. It also swaps in a
transcription-only template. `make_override.py` generates `pipeline_override.pb`, the options
substituted in Gemma mode.

## When Foresight updates

The shim checks before acting and logs what it skipped:

- **Filter regex not found:** the patch is skipped and Chinese may get stripped again. Find
  the new filter string and update `patch_ascii_filter` in `shim/foresight_zh.c`.
- **Speech C API changed:** the wrappers depend on the current argument layout and result
  struct. Check `shim.log`. Without the shim, the app is unaffected.

## Files

- `shim/foresight_zh.c`: dlsym interposition, ODML hooks, filter patch, engine selection
- `shim/sensevoice_engine.c`: VAD + SenseVoice engine via sherpa-onnx (loaded with `dlopen`)
- `shim/sherpa-onnx-c-api.h`: sherpa-onnx C API header (v1.13.8, Apache-2.0, Xiaomi Corporation)
- `shim/test/sv_harness.c`: feeds a raw 48kHz float32 capture through the engine
- `setup-sensevoice.sh`: downloads the engine; `shim/build.sh`: builds the shim
- `launch.sh`: launcher; `~/Applications/Foresight 中文.app` wraps it
- `model-build/`, `make_override.py`, `pipeline_override.pb`: the Gemma alternative
