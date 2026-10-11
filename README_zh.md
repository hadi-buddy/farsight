# foresight-zh

[English](README.md) · **简体中文**

让 macOS 上的 Google AI Edge Foresight 支持中文（以及中英混合）转写，无需修改应用本身。
另外可选：让 Foresight 的聊天改用更大的本地模型（Qwen3.8-27B），代替自带的 Gemma 4 E2B。

非官方项目，与 Google 无关。仓库中不包含任何 Google 代码或模型权重。

## 使用方法

- 打开 **`~/Applications/Foresight 中文.app`**（或运行 `~/tools/foresight-zh/launch.sh`）。
  如果 Foresight 已在运行，它会先退出，再带着 shim 重新启动。
- 用其他方式启动 Foresight（Dock、Spotlight、登录项、点击通知）不会加载 shim：应用照常可用，
  但只能转写英文。下面的可选守护程序可以自动解决这个问题。
- 文字按句出现：每次停顿后显示一段，而不是逐词显示。
- 日志：`~/Library/Logs/foresight-zh/shim.log`。如需同时记录每条转写结果（调试用）：
  `FORESIGHT_ZH_TRACE=1 ~/tools/foresight-zh/launch.sh`。

- 本地 Qwen 服务运行时，聊天走 Qwen；没运行时走 Foresight 自带的 Gemma（见下文"聊天改用本地模型"）。

## 安装步骤

shim 和启动脚本默认本仓库位于 `~/tools/foresight-zh`。

1. 安装 Foresight 并打开一次。
2. 下载转写引擎（约 270MB）：[sherpa-onnx](https://github.com/k2-fsa/sherpa-onnx) C 库、
   SenseVoice-Small（int8）和 Silero VAD，存放在 `~/local-llm/models/special/foresight-zh/`：

   ```bash
   ./setup-sensevoice.sh
   ```

3. 构建 shim：

   ```bash
   shim/build.sh
   ```

4. 可选：生成 Dock / Spotlight 启动器：

   ```bash
   osacompile -o ~/Applications/"Foresight 中文.app" -e 'do shell script "$HOME/tools/foresight-zh/launch.sh"'
   ```

5. 可选：安装自动重启守护程序。这个 LaunchAgent 每 5 秒检查一次，如果 Foresight 没有加载 shim，
   就通过 `launch.sh` 重新启动它。Dock 直接启动、通知、登录项、更新等情况都能覆盖。重启发生在
   普通启动后几秒内，所以这几秒内不要开始录音。5 分钟内最多重启 3 次。

   ```bash
   ./install-watcher.sh              # 安装并启动
   ./install-watcher.sh --uninstall  # 卸载
   touch ~/Library/Logs/foresight-zh/watcher.paused   # 暂停（删除该文件即恢复）
   ```

   日志：`~/Library/Logs/foresight-zh/watcher.log`。

## 工作原理

Foresight 的实时转写由 Google 闭源的 ODML 转写库完成。Dart 界面层通过导出的 C API 调用它
（`OdmlCreateInferenceEngine`、`OdmlCreateSpeechPipelineFromProto`、
`OdmlSpeechProcessorTranscribe`、`OdmlSpeechProcessorFinishStream` 等）。它自带的模型
（`models/3`）只支持英文。

`shim/libforesight_zh.dylib` 通过 `DYLD_INSERT_LIBRARIES` 注入。应用的 entitlements 允许这样做
（`allow-dyld-environment-variables`、`disable-library-validation`），因此 Google 的代码签名和
钥匙串访问都不受影响。shim 做两件事：

1. **替换语音识别。** 拦截 `dlsym`（Dart FFI 用它查找 C API），接管
   `OdmlSpeechProcessorTranscribe` 和 `OdmlSpeechProcessorFinishStream`。Foresight 传入的音频
   （48kHz 浮点，麦克风和系统声音各一路）先重采样到 16kHz，再用 Silero VAD 按停顿切分成句，
   每句在 CPU 上用 SenseVoice-Small 转写（通常 15–85 毫秒）。结果按 Foresight 自己的格式返回
   （`text`、`is_final`、以微秒计的起止时间）。Foresight 自带的英文引擎仍会加载，但不再运行。
   见 `shim/sensevoice_engine.c`。
2. **去掉 ASCII 过滤。** Dart 层会用正则 `[^\x20-\x7E\n\r]` 删掉转写文本中所有非可打印 ASCII
   字符，中文因此被全部删除。shim 在内存中（写时复制页面，磁盘上的程序不变）把这个正则常量
   改成 `[\x00-\x08\x7F]+`。

如果 SenseVoice 文件缺失或加载失败，shim 会退回 Foresight 自带的引擎（英文）。

### 备选引擎：Gemma 4 E2B

在 SenseVoice 之前，shim 让 Foresight 自己的转写流程运行一个由 Gemma 4 E2B（Foresight 自己
下载的模型）重新打包的模型：重定向模型路径，并把转写选项改成批处理加转写提示词。这个方案可用，
但需要一份 3.4GB 的 Gemma 副本，内存占用也更大。启用方法：

```bash
uv venv ~/local-llm/venvs/litert --python 3.12
uv pip install -p ~/local-llm/venvs/litert/bin/python litert-lm-api litert-lm
model-build/build.sh                       # 把 models/15 重新打包成 ASR 模型
echo gemma > ~/tools/foresight-zh/engine   # 切换引擎；删除这个文件即恢复默认
```

`model-build/build.sh` 会去掉视觉部分、drafter，以及音频部分的"仅限 CPU"标记（Foresight 在
GPU 上运行语音模型），并换上仅用于转写的模板。`make_override.py` 生成 Gemma 模式下替换的
选项文件 `pipeline_override.pb`。

## 聊天改用本地模型

Foresight 的聊天（询问笔记和日历）在本机运行 Gemma 4 E2B。shim 可以把它转到本地的
OpenAI 兼容服务。默认目标是 `~/local-llm` 里的 Qwen3.8-27B Splash 服务：

```bash
~/local-llm/runtime/bin/splash-serve start qwen38-27b-splash
echo 'http://127.0.0.1:8000/v1 incoai/Qwen3.8-27B-Splash {"reasoning_effort":"none"}' > ~/tools/foresight-zh/chat
```

`chat` 文件只有一行：`<base_url> [模型] [{额外请求字段}]`，或者 `off`。没有这个文件时默认
使用 `http://127.0.0.1:8096/v1 qwen3.8-27b-mtp`（llama.cpp）。修改后需要重启 Foresight。

- shim 不会自己启动模型服务：`~/local-llm` 同一时间只运行一个模型服务，启动一个会停掉其他的。
  服务没运行、或者端口上是别的模型时，聊天用 Gemma。每 5 秒重新检查一次。
- 采样参数交给服务端（与模型的模板模式匹配）。每次回复最多 2048 个 token，因为 llama.cpp
  在取消后仍会继续生成。
- 用 Foresight 的实际问题测试（M5 Max）：Splash 上的 Qwen3.8-27B 约每秒 212 字符，llama.cpp
  约 91。Qwen3.8-Flash-Next（MTPLX）速度相近，但有一题忽略了检索到的笔记，转而调用日历工具。

原理：Foresight 的聊天通过 LiteRT-LM 的对话 C API 传递 JSON 消息
（`litert_lm_create_conversation_v2`、`litert_lm_conversation_send_message_stream` 等）。shim
用同样的 `dlsym` 拦截接管这些函数，把对话（系统提示词、笔记上下文、历史、`search_calendar`
工具）转换成 OpenAI 聊天请求，按 Foresight 的格式流式返回回复，并把工具调用转换成 Foresight
的工具调用消息。引擎的其他用途（摘要、音频、图片）仍由 Gemma 处理。`FORESIGHT_ZH_TRACE=1`
会记录其余的 `litert_lm_*` 调用，并把 JSON 参数保存到 `~/Library/Logs/foresight-zh/lm/`。
`shim/test/chat_harness.c` 可以把保存的对话重新跑一遍。

## Foresight 更新之后

shim 在动手前会先检查，跳过的步骤会写进日志：

- **找不到过滤正则：** 跳过补丁，中文可能又会被删掉。找到新的过滤字符串，更新
  `shim/foresight_zh.c` 中的 `patch_ascii_filter`。
- **聊天 C API 有变化：** 查看 `shim.log` 中的 `chat:` 行，把 `chat` 设为 `off`，再用
  `FORESIGHT_ZH_TRACE=1` 重新跟踪。
- **语音 C API 有变化：** 接管逻辑依赖当前的参数布局和结果结构。查看 `shim.log`。不加载 shim
  时应用不受影响。

## 文件说明

- `shim/foresight_zh.c`：dlsym 拦截、ODML 钩子、过滤器补丁、引擎选择
- `shim/sensevoice_engine.c`：基于 sherpa-onnx 的 VAD + SenseVoice 引擎（通过 `dlopen` 加载）
- `shim/chat_bridge.c`：把聊天转到本地 OpenAI 兼容服务（libcurl）
- `shim/cJSON.c`、`shim/cJSON.h`：[cJSON](https://github.com/DaveGamble/cJSON) v1.7.18（MIT）
- `shim/test/chat_harness.c`：把保存的 Foresight 对话送进聊天转接层测试
- `shim/sherpa-onnx-c-api.h`：sherpa-onnx C API 头文件（v1.13.8，Apache-2.0，Xiaomi Corporation）
- `shim/test/sv_harness.c`：把 48kHz float32 原始录音送进引擎做测试
- `setup-sensevoice.sh`：下载引擎；`shim/build.sh`：构建 shim
- `launch.sh`：启动脚本；`~/Applications/Foresight 中文.app` 是它的封装
- `watcher.sh`、`install-watcher.sh`：自动重启的 LaunchAgent（通过
  `~/Library/Logs/foresight-zh/shim.pid` 判断 shim 是否已加载）
- `model-build/`、`make_override.py`、`pipeline_override.pb`：Gemma 备选方案
