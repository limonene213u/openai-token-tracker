# OpenAI トークントラッカー CLI
![GitHub stars](https://img.shields.io/github/stars/your-repo-name.svg?style=flat&label=Stars)
![GitHub license](https://img.shields.io/github/license/your-repo-name.svg)

OpenAI トークントラッカー CLI は、OpenAI API とのやり取りにおけるトークン使用量を監視・追跡するための軽量なコマンドラインツールです。トークン消費のログを記録することで、請求の透明性を確保します。

## 主な機能
- リクエストごとの**プロンプトトークン**、**完了トークン**、**合計トークン**を追跡
- トークン使用ログを `token_usage_log.json` に保存
- `show_tokens` コマンドで累積トークン使用量を表示
- **Clang** と **cJSON** を使用した効率的な解析
- `openai.json` による簡単な設定
- 日本語、英語、中国語（繁体字）の3言語に対応

## インストール
### 前提条件
- **Linux/macOS/Windows（Windows の場合は WSL を推奨）**
- **Clang または GCC コンパイラ**
- **cURL**
- **cJSON ライブラリ**

### ビルドとインストール
```sh
git clone https://github.com/your-username/openai-token-tracker.git
cd openai-token-tracker
make
```

## 使用方法
### API キーの設定
CLI を使用する前に、API キーを設定する必要があります。環境変数または JSON ファイルで設定できます。

#### 方法1: 環境変数の設定
```sh
export OPENAI_API_KEY="your-api-key"
```

#### 方法2: `openai.json` の設定
```json
{
  "api_key": "your-api-key",
  "model": "gpt-4-turbo",
  "api_url": "https://api.openai.com/v1/chat/completions",
  "preferred_language": "ja"
}
```

### CLI の実行
```sh
./openai_v2
```

### 対話例
```sh
User > こんにちは
Assistant > こんにちは！どのようなお手伝いができますか？
User > show_tokens
総プロンプトトークン: 868
総完了トークン: 33
総トークン: 901
```

## 言語設定
`openai.json` ファイルの `preferred_language` フィールドで優先言語を設定できます。サポートされている言語コード:
- `ja`: 日本語
- `en`: 英語
- `zh`: 中国語（繁体字）

言語設定に応じて、システムプロンプトと CLI の出力が変更されます。

## コントリビューション
プルリクエストを歓迎します！問題を発見した場合は、GitHub で Issue を開いてください。

## ライセンス
このプロジェクトは MIT ライセンスの下で公開されています。

---

# OpenAI Token Tracker CLI
![GitHub stars](https://img.shields.io/github/stars/your-repo-name.svg?style=flat&label=Stars)
![GitHub license](https://img.shields.io/github/license/your-repo-name.svg)

OpenAI Token Tracker CLI is a lightweight command-line tool that helps users monitor and track token usage while interacting with OpenAI's API. It ensures transparency in billing by logging token consumption.

## Features
- Track **prompt tokens**, **completion tokens**, and **total tokens** per request
- Store token usage logs in `token_usage_log.json`
- Display cumulative token usage with `show_tokens` command
- Efficient parsing using **Clang** and **cJSON**
- Easy configuration via `openai.json`
- Support for Japanese, English, and Traditional Chinese languages

## Installation
### Prerequisites
- **Linux/macOS/Windows (WSL recommended for Windows)**
- **Clang or GCC compiler**
- **cURL**
- **cJSON library**

### Build & Install
```sh
git clone https://github.com/your-username/openai-token-tracker.git
cd openai-token-tracker
make
```

## Usage
### Set API Key
Before using the CLI, you need to set up your API key. You can configure it via environment variables or a JSON file.

#### Option 1: Set Environment Variable
```sh
export OPENAI_API_KEY="your-api-key"
```

#### Option 2: Configure `openai.json`
```json
{
  "api_key": "your-api-key",
  "model": "gpt-4-turbo",
  "api_url": "https://api.openai.com/v1/chat/completions",
  "preferred_language": "en"
}
```

### Run the CLI
```sh
./openai_v2
```

### Example Interaction
```sh
User > Hello
Assistant > Hello! How can I assist you today?
User > show_tokens
Total Prompt Tokens: 868
Total Completion Tokens: 33
Total Tokens: 901
```

## Language Settings
You can set the preferred language in the `openai.json` file using the `preferred_language` field. Supported language codes:
- `ja`: Japanese
- `en`: English
- `zh`: Traditional Chinese

The system prompt and CLI output will change according to the language setting.

## Contributions
Pull requests are welcome! If you encounter any issues, feel free to open an issue on GitHub.

## License
This project is licensed under the MIT License.

---

# OpenAI Token Tracker CLI（繁體中文/台灣）
![GitHub stars](https://img.shields.io/github/stars/your-repo-name.svg?style=flat&label=Stars)
![GitHub license](https://img.shields.io/github/license/your-repo-name.svg)

OpenAI Token Tracker CLI 是一款輕量級的命令行工具，幫助用戶監控與 OpenAI API 交互時的 Token 使用情況，確保計費透明。

## 功能特點
- 追蹤每次請求的 **Prompt Tokens**、**Completion Tokens** 及 **Total Tokens**
- Token 使用記錄將保存在 `token_usage_log.json`
- 使用 `show_tokens` 指令顯示累計 Token 使用情況
- 基於 **Clang** 和 **cJSON**，效能優化
- 透過 `openai.json` 輕鬆配置
- 支援日文、英文和繁體中文三種語言

## 安裝方式
### 先決條件
- **Linux/macOS/Windows (Windows 建議使用 WSL)**
- **Clang 或 GCC 編譯器**
- **cURL**
- **cJSON 函式庫**

### 編譯 & 安裝
```sh
git clone https://github.com/your-username/openai-token-tracker.git
cd openai-token-tracker
make
```

## 使用方式
### 設定 API Key
在使用 CLI 之前，需要設置 API Key，可透過環境變數或 JSON 檔案來配置。

#### 方法 1：設置環境變數
```sh
export OPENAI_API_KEY="your-api-key"
```

#### 方法 2：設定 `openai.json`
```json
{
  "api_key": "your-api-key",
  "model": "gpt-4-turbo",
  "api_url": "https://api.openai.com/v1/chat/completions",
  "preferred_language": "zh"
}
```

### 執行 CLI
```sh
./openai_v2
```

### 互動示例
```sh
User > 你好
Assistant > 你好！今天有什麼我可以幫助你的嗎？
User > show_tokens
總 Prompt Tokens: 868
總 Completion Tokens: 33
總 Tokens: 901
```

## 語言設定
您可以在 `openai.json` 檔案中使用 `preferred_language` 欄位設置偏好語言。支援的語言代碼：
- `ja`：日文
- `en`：英文
- `zh`：繁體中文

系統提示和 CLI 輸出將根據語言設定而改變。

## 貢獻方式
歡迎 PR！若發現問題，請在 GitHub 上提交 Issue。

## 授權
本專案採用 MIT License 授權。

Citations:
[1] https://img.shields.io/github/stars/your-repo-name.svg?style=flat&label=Stars
[2] https://img.shields.io/github/license/your-repo-name.svg

---
Perplexity の Eliot より: pplx.ai/share