# foresight-zh

[English](README.md) · **简体中文**

让 macOS 上的 Google AI Edge Foresight 支持中文（以及中英混合）转写，无需修改应用本身。

非官方项目，与 Google 无关。仓库中不包含任何 Google 代码或模型权重：ASR 模型包是在本机
用 Foresight 自己下载的 Gemma 模型构建的。

## 使用方法

- 打开 **`~/Applications/Foresight 中文.app`**（或运行 `~/tools/foresight-zh/launch.sh`）。
  如果 Foresight 已在运行，它会先退出，再带着 shim 重新启动。
- 用其他方式启动 Foresight（Dock、Spotlight、登录项、点击通知）不会加载 shim。应用照常可用，
  但只能转写英文。
- 日志：`~/Library/Logs/foresight-zh/shim.log`。如需同时记录每条转写结果（调试用）：
  `FORESIGHT_ZH_TRACE=1 ~/tools/foresight-zh/launch.sh`。

## 工作原理

Foresight 的实时转写使用一个独立的本地 ASR 模型（`models/3`，仅支持英文，流式 USM 编码器 +
小型解码器），由 Google 闭源的 ODML 转写库运行。Dart 界面层通过导出的 C API 调用它
（`OdmlCreateInferenceEngine`、`OdmlCreateSpeechPipelineFromProto`、
`OdmlSpeechProcessorTranscribe` 等）。

`shim/libforesight_zh.dylib` 通过 `DYLD_INSERT_LIBRARIES` 注入。应用的 entitlements 允许这样做
（`allow-dyld-environment-variables`、`disable-library-validation`），因此 Google 的代码签名和
钥匙串访问都不受影响。shim 做三件事：

1. **模型重定向。** 拦截 `dlsym`（Dart FFI 用它查找符号），把 ASR 引擎指向
   `~/local-llm/models/special/foresight-zh/zh_asr_gemma4e2b_v3.litertlm`。这个文件把
   Foresight 已下载的 Gemma 4 E2B 模型（`models/15`）中的音频编码器、适配器和解码器重新打包成
   仅用于转写的模型包。音频编码器按应用要求在 GPU 上运行。
2. **选项改写。** 应用请求的是带中间结果的流式模式，而 Gemma 的音频编码器只支持批处理，会报
   "Streaming audio encoder is not available"。shim 用 `pipeline_override.pb` 替换这些选项：
   关闭流式，开启 VAD 分段，并使用 Gemma 对话格式的提示词，要求输出简体中文、英文保持原样。
   重新生成：`python3 make_override.py pipeline_override.pb`。
3. **过滤器补丁。** Dart 层会用正则 `[^\x20-\x7E\n\r]` 删掉转写文本中所有非可打印 ASCII 字符，
   中文因此被全部删除。shim 在内存中（写时复制页面，磁盘上的程序不变）把这个正则常量改成
   `[\x00-\x08\x7F]+`。

文字按句出现：每次停顿后显示一段，而不是逐词显示。

## Foresight 更新之后

shim 在动手前会先检查，跳过的步骤会写进日志：

- **找不到过滤正则：** 跳过补丁，中文可能又会被删掉。找到新的过滤字符串，更新
  `shim/foresight_zh.c` 中的 `patch_ascii_filter`。
- **语音 C API 有变化：** 模型重定向和选项改写依赖当前的参数布局。查看 `shim.log` 中的错误。
  不加载 shim 时应用不受影响。
- **Gemma 模型的 ID 或路径有变化：** 用 `model-build/build.sh` 重新构建 ASR 模型包。

构建 shim（clone 之后必须执行）：`cd shim && clang -arch arm64 -dynamiclib -O2 -Wall -o libforesight_zh.dylib foresight_zh.c`。

## 文件说明

- `shim/foresight_zh.c`：shim 源码；`shim/libforesight_zh.dylib`：编译好的 shim
- `launch.sh`：启动脚本；`~/Applications/Foresight 中文.app` 是它的封装
- `make_override.py`、`pipeline_override.pb`：shim 替换进去的语音处理选项
- `model-build/`：Gemma ASR 模型包的构建方法
- `~/local-llm/models/special/foresight-backup/`：Foresight 原版 ASR 模型（加密版和解密版），
  留作参考
