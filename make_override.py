#!/usr/bin/env python3
"""Writes pipeline_override.pb: the SpeechPipelineOptionsProto the shim substitutes for Foresight's own."""
import sys

def varint(n):
    out = b""
    while True:
        b = n & 0x7F; n >>= 7
        if n: out += bytes([b | 0x80])
        else: return out + bytes([b])

def f_varint(num, v): return varint(num << 3) + varint(v)
def f_bytes(num, b):
    if isinstance(b, str): b = b.encode()
    return varint(num << 3 | 2) + varint(len(b)) + b

INSTRUCTION = ("请逐字转写这段语音，保持说话时使用的语言。"
               "中文一律用简体字书写，英文单词保持英文原样。只输出转写内容，不要翻译或解释。")

prompt = (f_bytes(1, INSTRUCTION) + f_bytes(2, "<|turn>user\n") + f_bytes(3, "<|turn>model\n")
          + f_bytes(4, "<turn|>\n"))
advanced = f_varint(1, int(sys.argv[2]) if len(sys.argv) > 2 else 1600)   # max_spectrogram_columns_per_session
litertlm = (f_varint(1, 48000) + f_varint(2, 1) + f_varint(3, 0) + f_varint(4, 0)
            + f_bytes(5, prompt) + f_bytes(6, advanced))
spo = f_bytes(1000, litertlm)            # SpeechProcessorOptionsProto extension 1000
pipeline = f_bytes(1, spo)               # SpeechPipelineOptionsProto.speech_processor_options
out = sys.argv[1] if len(sys.argv) > 1 else "pipeline_override.pb"
open(out, "wb").write(pipeline)
print(f"wrote {out} ({len(pipeline)} bytes)")
