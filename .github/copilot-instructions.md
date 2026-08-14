---
name: project-language-guidelines
description: 本项目 Copilot 交互语言规则：除专业术语外，所有回答和生成文件均使用中文描述。
---

# 项目交互语言规则

## 规则

在回答用户问题以及生成代码、脚本、文档、注释等文件内容时：

- **使用中文**进行说明、解释、注释和描述。
- **保留专业术语**使用英文原文，例如：
  - 类名、函数名、变量名（如 `YangPushPublish`、`deleteVideoEncoding`、`m_encoder`）
  - 编程语言关键字、库名、框架名（如 `CMake`、`Qt`、`OpenSSL`、`ffmpeg`）
  - 协议、格式、工具链名称（如 `WHIP`、`RTC`、`H.264`、`CMakeLists.txt`）
- 文件名、路径、代码块中的标识符保持原样，不做翻译。

## 示例

| 场景 | 正确写法 |
|------|----------|
| 解释函数作用 | `YangPushPublish::change` 函数用于向 `m_capture` 转发状态变更。 |
| 生成脚本注释 | `# 进入 demo/metapushstream7 目录并执行 CMake 构建。` |
| 变量说明 | `m_encoder` 指向当前使用的视频编码器实例。 |
