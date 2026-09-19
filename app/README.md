# cocompanion

Waveshare **ESP32-S3-Touch-AMOLED-2.16** 上的一个多应用伴侣设备固件，基于 ESP-IDF + LVGL v9。

四个应用，一个 launcher 主菜单统一入口：

| 应用 | 说明 |
|------|------|
| **AI Chat** | 联网大模型对话（OpenAI 兼容接口，DeepSeek / OpenAI / Qwen 等均可） |
| **Calculator** | 四则运算计算器（支持连续运算、小数、退格、除零检测） |
| **Clock** | 时钟（SNTP 同步）+ 秒表 + 倒计时 |
| **Settings** | 屏幕亮度、背光开关、Wi-Fi 状态、关于本机 |

## 硬件

- 主控：ESP32-S3（16MB Flash / 8MB Octal PSRAM）
- 屏幕：2.16" AMOLED，480×480，CO5300（QSPI）
- 触摸：CST9217

## 依赖

- ESP-IDF **v5.5.5**（官方示例测试版本）
- 依赖组件（由 `idf.py` 自动拉取）：
  - `waveshare/esp32_s3_touch_amoled_2_16` ^2.0.1
  - `lvgl/lvgl` 9.x
  - `espressif/cjson`

## 构建

```bash
# 1. 激活 ESP-IDF 环境（按你的安装方式，例如）
. $IDF_PATH/export.sh

# 2. 进入项目目录
cd app

# 3. 选择目标芯片（首次）
idf.py set-target esp32s3

# 4. 配置 Wi-Fi 和 AI 参数（必做，见下）
idf.py menuconfig
#   -> cocompanion -> Wi-Fi      填入 SSID / 密码
#   -> cocompanion -> AI Chat    填入 endpoint / API key / model

# 5. 编译烧录
idf.py build
idf.py -p /dev/cu.usbmodemXXX flash monitor
```

> 第一次构建会自动下载依赖组件（BSP、LVGL），需要联网，耗时较长。

## AI Chat 配置

任意 **OpenAI 兼容** 的 chat completions 端点都可以，常见示例：

| 服务 | Endpoint | Model |
|------|----------|-------|
| DeepSeek | `https://api.deepseek.com/chat/completions` | `deepseek-chat` |
| OpenAI | `https://api.openai.com/v1/chat/completions` | `gpt-4o-mini` |
| 阿里 Qwen | `https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions` | `qwen-turbo` |

HTTPS 证书使用 IDF 内置 CA bundle（`esp_crt_bundle_attach`），无需手动配置证书。

> 默认 system prompt 要求模型**用英文简洁回复**（内置 Montserrat 字体不含中文字形）。
> 需要中文显示时，见下方「中文字体」一节。

## 中文字体

内置字体只覆盖 ASCII。要显示中文（AI 中文回复 / 中文 UI）：

1. 准备 TTF 字体（如思源黑体子集）。
2. 用 [LVGL Font Converter](https://lvgl.io/tools/fontconverter) 生成 C 字库（记得勾选需要的常用汉字 + 字母数字）。
3. 将生成的 `.c` 放入 `main/`，在 `CMakeLists.txt` 的 `SRCS` 中登记，并在代码里 `lv_obj_set_style_text_font(..., &your_font, 0)` 替换。
4. 在 `ai_client.c` 的 system prompt 中把 "in English only" 改为 "用中文回答"。

## 目录结构

```
app/
├── CMakeLists.txt
├── partitions.csv          # 8M app + 7M storage
├── sdkconfig.defaults      # 芯片 / PSRAM / LVGL 默认配置
└── main/
    ├── main.c              # 入口：显示 + UI + 网络初始化
    ├── ui.h / ui.c         # 应用框架（launcher 切换、主题、通用控件）
    ├── app_launcher.c      # 主菜单
    ├── app_calculator.c    # 计算器
    ├── app_clock.c         # 时钟 / 秒表 / 倒计时
    ├── app_settings.c      # 设置
    ├── app_ai.c            # AI 对话界面
    ├── ai_client.h/c       # 大模型 HTTP 客户端
    ├── wifi_mgr.h/c        # Wi-Fi 连接 + SNTP
    └── Kconfig.projbuild   # menuconfig 配置项
```

## 已知限制

- 背光开关调用 BSP 的 `backlight_on/off`，开启时会固定恢复为 100% 亮度（BSP 行为），与亮度滑块非独立联动。
- 未实现软键盘，AI 对话输入使用预设快捷问题；可扩展接入 LVGL keyboard 或语音输入。
- 时钟依赖网络 SNTP 同步；未联网时从 1970-01-01 起算。
