# Changelog

Farsight (hadi-buddy/farsight), built on michaelpersonal/foresight-zh @ 8f15aea.

## 2026-10-18 — Persian orthography, VAD pre-roll, RTL experiment

- **Context:** the first live run showed «می شه» / «طرح های» without ZWNJ, and a clipped first
  syllable («ظه ضبط»).
- **Change:**
  - `shim/persian_text.c` (new): ي/ى→ی, ك→ک, Arabic-Indic→Persian digits; ZWNJ for «می/نمی + verb»,
    «word + ها/های/…», «…ه + ای». Whole tokens only. Tests in `shim/test/text_test.c`.
  - `shim/sensevoice_engine.c`: per-stream 32 s ring of 16 kHz audio; each phrase is decoded with
    0.35 s of audio before the VAD start. Output goes through ITN, then `persian_fix`.
  - `launch.sh`: `FARSIGHT_LANG=fa` starts Foresight with that UI language, to test whether the
    app lays out right-to-left.
- **Status:** unit tests pass on Linux; to verify on macOS.

## 2026-10-18 — First live run on macOS

- **Context:** first end-to-end test on the maintainer's Mac (Apple Silicon, Foresight with Gemma 4 E4B).
- **Result:** `config: engine=persian`, `filter patch: applied`, `asr (persian): ready`; Persian speech
  shows up in Foresight's Transcript tab phrase by phrase, letters joined correctly.
- **Seen in the test:** the first syllable of a phrase can be clipped by VAD («ظه ضبط» for «...ه ضبط»);
  no zero-width non-joiner in «می شه», «طرح های».
- **Status:** working. Next: ZWNJ/Arabic-letter normalization, VAD pre-roll.

## 2026-10-18 — Resumable downloads in setup-persian.sh

- **Context:** on a slow or unstable connection the sherpa-onnx download (silent `curl | tar`)
  looked frozen, and any drop meant starting over.
- **Change:** a `fetch` helper downloads to `<file>.part` with a progress bar and resumes with
  `curl -C -` after a drop (up to 50 tries; a stall under 1 KB/s for 60 s counts as a drop).
  Used for sherpa-onnx, Shenava and Silero VAD.
- **Status:** done.

## 2026-10-17 — Rename to Farsight; new README

- **Context:** the fork should stand as its own project, findable by people searching for
  Persian speech-to-text on macOS, instead of reading as a copy of foresight-zh.
- **Change:**
  - Paths: repo `~/tools/foresight-zh` → `~/tools/farsight`; logs `~/Library/Logs/farsight`;
    models `~/Library/Application Support/farsight` (was `~/local-llm/models/special/foresight-zh`).
  - `shim/foresight_zh.c` → `shim/farsight.c`, output `libfarsight.dylib`; env vars
    `FORESIGHT_ZH_*` → `FARSIGHT_*`; LaunchAgent label `io.github.hadi-buddy.farsight.watcher`.
  - `launch.sh` finds the shim next to itself instead of a hard-coded path.
  - `README.md` and `README_fa.md` rewritten from scratch: Persian-first, install steps,
    limitations, credits to foresight-zh and Shenava.
- **Status:** done; GitHub repo renamed to `farsight`.

## 2026-10-17 — Remove the Chinese README

- **Context:** this fork only targets Persian.
- **Change:** deleted `README_zh.md` and its language links in `README.md` and `README_fa.md`.
  The Chinese code paths (SenseVoice engine, CJK spacing) stay as a fallback.
- **Status:** done.

## 2026-10-17 — Persian as the default engine

- **Context:** the fork should transcribe Persian without any extra configuration.
- **Change:** `load_config` in `shim/foresight_zh.c` defaults to `persian` when
  `~/tools/foresight-zh/engine` is absent. `setup-persian.sh` removes that file instead of
  writing it. Fallback order: Persian → SenseVoice → Foresight's own English engine.
- **Status:** done; verified on macOS 2026-10-18.

## 2026-10-17 — Persian transcription engine

- **Context:** SenseVoice-Small only covers zh/en/yue/ja/ko, so it cannot transcribe Persian.
- **Change:**
  - `shim/sensevoice_engine.c`: `sv_available(home, lang)` selects SenseVoice or
    Shenava Rizeh v1.0 (Persian NeMo CTC, Apache-2.0) via `model_config.nemo_ctc`.
    The model's ONNX metadata disables NeMo feature normalization, so the feature config
    is left untouched.
  - `shim/persian_itn.c` (new, written from scratch): spoken Persian numbers → Persian
    digits; a lone «یک» stays a word. Tests: `shim/test/itn_test.sh` (17 cases, passing).
  - `shim/chat_bridge.c`: Arabic-script share ≥ 30% → ask the chat model for Persian output.
  - `setup-persian.sh`, `README_fa.md`, Persian section in `README.md`.
- **Status:** verified on macOS 2026-10-18 (see "First live run").
