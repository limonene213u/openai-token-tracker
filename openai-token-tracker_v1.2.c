#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <unistd.h>
#include <time.h>
#include <cjson/cJSON.h>

#define DEFAULT_API_URL "https://api.openai.com/v1/chat/completions"
#define POLLING_RATE_MS 1000  // 各リクエスト後の待機時間（ミリ秒）

// レスポンス内容を保持する構造体
struct Memory {
    char *response;
    size_t size;
};

// curl の書き込み用コールバック関数
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct Memory *mem = (struct Memory *)userp;
    char *ptr = realloc(mem->response, mem->size + realsize + 1);
    if (ptr == NULL) {
        fprintf(stderr, "メモリ不足\n");
        return 0;
    }
    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->response[mem->size] = '\0';
    return realsize;
}

// ファイル全体を読み込む関数（openai.json 等の設定ファイル用）
static char* read_file(const char* filename) {
    FILE *f = fopen(filename, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    char *data = malloc(len + 1);
    if (!data) {
        fclose(f);
        return NULL;
    }
    fread(data, 1, len, f);
    data[len] = '\0';
    fclose(f);
    return data;
}

// 現在の時刻をISO 8601形式（タイムゾーン付き）で返す関数
// tz: "JST", "Taiwan", "UTC", "ET", "CT", "MT", "PT" のいずれか。見つからなければUTCを返す
char* get_iso8601_timestamp(const char *tz) {
    time_t now = time(NULL);
    int offset = 0;
    const char* tz_str = "Z"; // UTC
    if (strcmp(tz, "JST") == 0) {
        offset = 9 * 3600;
        tz_str = "+09:00";
    } else if (strcmp(tz, "Taiwan") == 0) {
        offset = 8 * 3600;
        tz_str = "+08:00";
    } else if (strcmp(tz, "ET") == 0) {
        offset = -5 * 3600;
        tz_str = "-05:00";
    } else if (strcmp(tz, "CT") == 0) {
        offset = -6 * 3600;
        tz_str = "-06:00";
    } else if (strcmp(tz, "MT") == 0) {
        offset = -7 * 3600;
        tz_str = "-07:00";
    } else if (strcmp(tz, "PT") == 0) {
        offset = -8 * 3600;
        tz_str = "-08:00";
    } else if (strcmp(tz, "UTC") == 0) {
        offset = 0;
        tz_str = "Z";
    }

    time_t local_time = now + offset;
    struct tm tm_local;
    gmtime_r(&local_time, &tm_local);
    char *buffer = malloc(30);
    if (buffer) {
        // YYYY-MM-DDTHH:MM:SS 形式
        strftime(buffer, 30, "%Y-%m-%dT%H:%M:%S", &tm_local);
        if (strcmp(tz_str, "Z") == 0)
            strcat(buffer, "Z");
        else
            strcat(buffer, tz_str);
    }
    return buffer;
}

// API の JSON レスポンスから assistant の応答を抜き出す
char* extract_assistant_response(const char *json_response) {
    cJSON *root = cJSON_Parse(json_response);
    if (!root) {
        fprintf(stderr, "JSON解析エラー\n");
        return NULL;
    }
    cJSON *choices = cJSON_GetObjectItemCaseSensitive(root, "choices");
    if (!cJSON_IsArray(choices)) {
        fprintf(stderr, "Invalid JSON: 'choices' が配列ではありません\n");
        cJSON_Delete(root);
        return NULL;
    }
    cJSON *choice = cJSON_GetArrayItem(choices, 0);
    if (!choice || !cJSON_IsObject(choice)) {
        fprintf(stderr, "Invalid JSON: choices[0] がオブジェクトではありません\n");
        cJSON_Delete(root);
        return NULL;
    }
    cJSON *message = cJSON_GetObjectItemCaseSensitive(choice, "message");
    if (!message || !cJSON_IsObject(message)) {
        fprintf(stderr, "Invalid JSON: 'message' がオブジェクトではありません\n");
        cJSON_Delete(root);
        return NULL;
    }
    cJSON *content = cJSON_GetObjectItemCaseSensitive(message, "content");
    if (!content || !cJSON_IsString(content)) {
        fprintf(stderr, "Invalid JSON: 'content' が文字列ではありません\n");
        cJSON_Delete(root);
        return NULL;
    }
    char *result = strdup(content->valuestring);
    cJSON_Delete(root);
    return result;
}

// レスポンスの "usage" 情報を、タイムスタンプ付きでログ（token_usage_log.json）に追記する
void log_token_usage(const char *json_response, const char *tz) {
    cJSON *root = cJSON_Parse(json_response);
    if (!root) {
        fprintf(stderr, "[ログ] JSON解析エラー\n");
        return;
    }
    cJSON *usage = cJSON_GetObjectItemCaseSensitive(root, "usage");
    if (!usage) {
        cJSON_Delete(root);
        return;
    }
    int prompt_tokens = 0, completion_tokens = 0, total_tokens = 0;
    cJSON *pt = cJSON_GetObjectItemCaseSensitive(usage, "prompt_tokens");
    cJSON *ct = cJSON_GetObjectItemCaseSensitive(usage, "completion_tokens");
    cJSON *tt = cJSON_GetObjectItemCaseSensitive(usage, "total_tokens");
    if (cJSON_IsNumber(pt))
        prompt_tokens = pt->valueint;
    if (cJSON_IsNumber(ct))
        completion_tokens = ct->valueint;
    if (cJSON_IsNumber(tt))
        total_tokens = tt->valueint;
    cJSON_Delete(root);

    // タイムスタンプ作成（ISO 8601形式）
    char *timestamp = get_iso8601_timestamp(tz);

    // 既存のログファイルを読み込む（存在しなければ新規作成）
    FILE *file = fopen("token_usage_log.json", "r");
    cJSON *log_json = NULL;
    if (file) {
        fseek(file, 0, SEEK_END);
        long len = ftell(file);
        rewind(file);
        char *content = malloc(len + 1);
        if (content) {
            fread(content, 1, len, file);
            content[len] = '\0';
            log_json = cJSON_Parse(content);
            free(content);
        }
        fclose(file);
    }
    if (!log_json) {
        log_json = cJSON_CreateArray();
    }

    // 新しいログエントリを作成
    cJSON *entry = cJSON_CreateObject();
    cJSON_AddStringToObject(entry, "timestamp", timestamp);
    cJSON_AddNumberToObject(entry, "prompt_tokens", prompt_tokens);
    cJSON_AddNumberToObject(entry, "completion_tokens", completion_tokens);
    cJSON_AddNumberToObject(entry, "total_tokens", total_tokens);
    cJSON_AddItemToArray(log_json, entry);

    char *log_out = cJSON_PrintUnformatted(log_json);
    FILE *outfile = fopen("token_usage_log.json", "w");
    if (outfile) {
        fputs(log_out, outfile);
        fclose(outfile);
    }
    free(log_out);
    cJSON_Delete(log_json);
    free(timestamp);
}

// ログファイルから累計トークン使用量を計算して表示する関数
void show_token_usage() {
    FILE *file = fopen("token_usage_log.json", "r");
    if (!file) {
        printf("ログファイルが存在しません。\n");
        return;
    }
    fseek(file, 0, SEEK_END);
    long len = ftell(file);
    rewind(file);
    char *content = malloc(len + 1);
    fread(content, 1, len, file);
    content[len] = '\0';
    fclose(file);

    cJSON *log_json = cJSON_Parse(content);
    free(content);
    if (!log_json) {
        printf("ログの解析に失敗しました。\n");
        return;
    }
    int total_prompt = 0, total_completion = 0, total = 0;
    int count = cJSON_GetArraySize(log_json);
    for (int i = 0; i < count; i++) {
        cJSON *entry = cJSON_GetArrayItem(log_json, i);
        cJSON *pt = cJSON_GetObjectItemCaseSensitive(entry, "prompt_tokens");
        cJSON *ct = cJSON_GetObjectItemCaseSensitive(entry, "completion_tokens");
        cJSON *tt = cJSON_GetObjectItemCaseSensitive(entry, "total_tokens");
        if (cJSON_IsNumber(pt))
            total_prompt += pt->valueint;
        if (cJSON_IsNumber(ct))
            total_completion += ct->valueint;
        if (cJSON_IsNumber(tt))
            total += tt->valueint;
    }
    printf("総プロンプトトークン: %d\n総コンプリーショントークン: %d\n総トークン: %d\n",
           total_prompt, total_completion, total);
    cJSON_Delete(log_json);
}

//
// ==== main (openai-cli-tool.c) ====
// ※ openai_core.c は純粋なチャットシステムのコア部分であり、参考用としています。
//     基本的には本ファイル (openai-cli-tool.c) をコンパイルしてご使用ください。
// ※ この実装は v1.2 で、ISO 8601形式タイムスタンプおよびタイムゾーン切替に対応しています。
//
int main(void) {
    // 設定ファイル (openai.json) を読み込む
    char *json_content = read_file("openai.json");
    char *api_key = NULL;
    char *preferred_language = NULL;
    char *system_prompt = NULL;
    char *custom_instructions = NULL;
    char *user_info = NULL;
    char *ai_profile = NULL;
    char *config_model = NULL;
    char *config_api_url = NULL;
    char *config_timezone = NULL;  // 新たにタイムゾーン設定

    if (json_content) {
        cJSON *json = cJSON_Parse(json_content);
        free(json_content);
        if (json) {
            cJSON *item = cJSON_GetObjectItemCaseSensitive(json, "api_key");
            if (cJSON_IsString(item) && (item->valuestring != NULL))
                api_key = strdup(item->valuestring);
            item = cJSON_GetObjectItemCaseSensitive(json, "preferred_language");
            if (cJSON_IsString(item) && (item->valuestring != NULL))
                preferred_language = strdup(item->valuestring);
            item = cJSON_GetObjectItemCaseSensitive(json, "system_prompt");
            if (cJSON_IsString(item) && (item->valuestring != NULL))
                system_prompt = strdup(item->valuestring);
            item = cJSON_GetObjectItemCaseSensitive(json, "custom_instructions");
            if (cJSON_IsString(item) && (item->valuestring != NULL))
                custom_instructions = strdup(item->valuestring);
            item = cJSON_GetObjectItemCaseSensitive(json, "user_info");
            if (cJSON_IsString(item) && (item->valuestring != NULL))
                user_info = strdup(item->valuestring);
            item = cJSON_GetObjectItemCaseSensitive(json, "ai_profile");
            if (cJSON_IsString(item) && (item->valuestring != NULL))
                ai_profile = strdup(item->valuestring);
            item = cJSON_GetObjectItemCaseSensitive(json, "model");
            if (cJSON_IsString(item) && (item->valuestring != NULL))
                config_model = strdup(item->valuestring);
            item = cJSON_GetObjectItemCaseSensitive(json, "api_url");
            if (cJSON_IsString(item) && (item->valuestring != NULL))
                config_api_url = strdup(item->valuestring);
            // 新規：タイムゾーン設定を読み込む
            item = cJSON_GetObjectItemCaseSensitive(json, "timezone");
            if (cJSON_IsString(item) && (item->valuestring != NULL))
                config_timezone = strdup(item->valuestring);
            cJSON_Delete(json);
        }
    }
    // 設定ファイルが不足している場合のフォールバック
    if (!api_key) {
        api_key = getenv("OPENAI_API_KEY");
        if (api_key == NULL) {
            fprintf(stderr, "Error: API key が設定されていません。（openai.json または環境変数 OPENAI_API_KEY）\n");
            return 1;
        }
    }
    if (!preferred_language) {
        preferred_language = getenv("PREFERRED_LANGUAGE");
        if (preferred_language == NULL)
            preferred_language = "ja";
    }
    if (!system_prompt) {
        system_prompt = getenv("SYSTEM_PROMPT");
        if (system_prompt == NULL) {
            if (strcmp(preferred_language, "en") == 0)
                system_prompt = "You are a helpful assistant.";
            else if (strcmp(preferred_language, "zh") == 0)
                system_prompt = "你是個得力的助手。我想用繁體中文交流。";
            else
                system_prompt = "あなたは有能なアシスタントです。";
        }
    }
    if (!custom_instructions) {
        custom_instructions = getenv("CUSTOM_INSTRUCTIONS");
        if (custom_instructions == NULL)
            custom_instructions = "";
    }
    if (!user_info) {
        user_info = getenv("USER_INFO");
        if (user_info == NULL)
            user_info = "";
    }
    if (!ai_profile) {
        ai_profile = getenv("AI_PROFILE");
        if (ai_profile == NULL)
            ai_profile = "";
    }
    // モデル名設定：設定ファイル、環境変数、またはデフォルト ("gpt-4o-mini")
    char *model = config_model;
    if (!model) {
        model = getenv("OPENAI_MODEL");
        if (model == NULL)
            model = "gpt-4o-mini";
    }
    // API URL の設定：設定ファイル、環境変数、またはデフォルト
    char *api_url = config_api_url;
    if (!api_url) {
        api_url = getenv("OPENAI_API_URL");
        if (api_url == NULL)
            api_url = DEFAULT_API_URL;
    }
    // タイムゾーン設定：設定ファイル、環境変数 "TIMEZONE"、またはデフォルト "UTC"
    char *timezone = config_timezone;
    if (!timezone) {
        timezone = getenv("TIMEZONE");
        if (timezone == NULL)
            timezone = "UTC";
    }
    
    // 統合システムプロンプトの作成
    char combined_prompt[8192];
    snprintf(combined_prompt, sizeof(combined_prompt),
             "%s\n%s\n%s\n%s",
             system_prompt, custom_instructions, user_info, ai_profile);
    
    curl_global_init(CURL_GLOBAL_ALL);
    char input[1024];
    
    while (1) {
        printf("User > ");
        if (fgets(input, sizeof(input), stdin) == NULL)
            break;
        input[strcspn(input, "\n")] = '\0';
        
        // "show_tokens" 入力時は集計を表示
        if (strcmp(input, "show_tokens") == 0) {
            show_token_usage();
            continue;
        }
        if (strcmp(input, "exit") == 0)
            break;
        
        // cJSON を使ってリクエスト JSON オブジェクトを構築
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "model", model);
        cJSON *messages = cJSON_CreateArray();
        
        // システムメッセージ追加
        cJSON *sys_msg = cJSON_CreateObject();
        cJSON_AddStringToObject(sys_msg, "role", "system");
        cJSON_AddStringToObject(sys_msg, "content", combined_prompt);
        cJSON_AddItemToArray(messages, sys_msg);
        
        // ユーザーメッセージ追加
        cJSON *user_msg = cJSON_CreateObject();
        cJSON_AddStringToObject(user_msg, "role", "user");
        cJSON_AddStringToObject(user_msg, "content", input);
        cJSON_AddItemToArray(messages, user_msg);
        
        cJSON_AddItemToObject(root, "messages", messages);
        cJSON_AddNumberToObject(root, "temperature", 0.7);
        
        char *json_data = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);
        
        // （デバッグ用：必要なら Request JSON を出力
        // printf("Request JSON: %s\n", json_data);
        //）
        
        CURL *curl = curl_easy_init();
        if (curl) {
            struct curl_slist *headers = NULL;
            char auth_header[256];
            snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, auth_header);
            
            struct Memory chunk;
            chunk.response = malloc(1);
            chunk.size = 0;
            
            curl_easy_setopt(curl, CURLOPT_URL, api_url);
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
            
            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() 失敗: %s\n", curl_easy_strerror(res));
            } else {
                char *assistant_response = extract_assistant_response(chunk.response);
                if (assistant_response) {
                    printf("Assistant > %s\n", assistant_response);
                    free(assistant_response);
                } else {
                    fprintf(stderr, "アシスタントの応答抽出に失敗\n");
                }
                // レスポンスからトークン使用量をログに記録（タイムスタンプ付き）
                log_token_usage(chunk.response, timezone);
            }
            
            free(chunk.response);
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
        }
        free(json_data);
        usleep(POLLING_RATE_MS * 1000);
    }
    
    curl_global_cleanup();
    return 0;
}
