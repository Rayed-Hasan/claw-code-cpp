#pragma once

#include <string>
#include <vector>
#include <variant>
#include <nlohmann/json.hpp>

namespace claw::api {

    // ── Content Block Types ──────────────────────────────────────────────
    // Anthropic's Messages API uses arrays of typed content blocks.
    // A single assistant turn can contain both text AND tool_use blocks.

    struct TextBlock {
        std::string text;
    };

    struct ToolUseBlock {
        std::string id;       // Server-assigned tool call ID (e.g. "toolu_01A...")
        std::string name;     // Tool name (e.g. "BashTool")
        nlohmann::json input; // Parsed arguments for the tool
    };

    struct ToolResultBlock {
        std::string tool_use_id; // Must match the ToolUseBlock::id it responds to
        std::string content;     // The tool's output as a string
        bool is_error = false;   // Whether the tool execution failed
    };

    using ContentBlock = std::variant<TextBlock, ToolUseBlock, ToolResultBlock>;

    // ── Message ──────────────────────────────────────────────────────────
    // Represents a single message in the conversation history.
    // Content is an array of blocks to support mixed text + tool_use turns.

    struct Message {
        std::string role;  // "user" or "assistant"
        std::vector<ContentBlock> content;

        // ── Factory methods for common message patterns ──

        static Message user_text(const std::string& text) {
            return {"user", {TextBlock{text}}};
        }

        static Message assistant_text(const std::string& text) {
            return {"assistant", {TextBlock{text}}};
        }

        static Message tool_results(const std::vector<ToolResultBlock>& results) {
            Message msg;
            msg.role = "user";
            for (const auto& r : results) {
                msg.content.push_back(r);
            }
            return msg;
        }

        // ── Serialization to Anthropic API format ──

        nlohmann::json to_json() const {
            nlohmann::json j;
            j["role"] = role;

            nlohmann::json content_array = nlohmann::json::array();
            for (const auto& block : content) {
                std::visit([&content_array](const auto& b) {
                    using T = std::decay_t<decltype(b)>;

                    if constexpr (std::is_same_v<T, TextBlock>) {
                        content_array.push_back({
                            {"type", "text"},
                            {"text", b.text}
                        });
                    }
                    else if constexpr (std::is_same_v<T, ToolUseBlock>) {
                        content_array.push_back({
                            {"type", "tool_use"},
                            {"id", b.id},
                            {"name", b.name},
                            {"input", b.input}
                        });
                    }
                    else if constexpr (std::is_same_v<T, ToolResultBlock>) {
                        nlohmann::json block_json = {
                            {"type", "tool_result"},
                            {"tool_use_id", b.tool_use_id},
                            {"content", b.content}
                        };
                        if (b.is_error) {
                            block_json["is_error"] = true;
                        }
                        content_array.push_back(block_json);
                    }
                }, block);
            }
            j["content"] = content_array;
            return j;
        }
    };

    // ── API Turn Response ────────────────────────────────────────────────
    // Parsed response from the Anthropic Messages API.

    struct TurnResponse {
        std::vector<ContentBlock> content;
        std::string stop_reason;  // "end_turn", "tool_use", "max_tokens"
        int input_tokens = 0;
        int output_tokens = 0;

        // Extract concatenated text from all TextBlocks
        std::string get_text() const {
            std::string result;
            for (const auto& block : content) {
                if (auto* tb = std::get_if<TextBlock>(&block)) {
                    if (!result.empty()) result += "\n";
                    result += tb->text;
                }
            }
            return result;
        }

        // Extract all tool call requests
        std::vector<ToolUseBlock> get_tool_calls() const {
            std::vector<ToolUseBlock> calls;
            for (const auto& block : content) {
                if (auto* tu = std::get_if<ToolUseBlock>(&block)) {
                    calls.push_back(*tu);
                }
            }
            return calls;
        }

        bool has_tool_calls() const {
            return stop_reason == "tool_use";
        }

        // Parse a raw JSON response body from the Anthropic API
        static TurnResponse from_json(const nlohmann::json& j) {
            TurnResponse resp;
            resp.stop_reason = j.value("stop_reason", "end_turn");
            resp.input_tokens = j["usage"].value("input_tokens", 0);
            resp.output_tokens = j["usage"].value("output_tokens", 0);

            for (const auto& block : j["content"]) {
                std::string type = block["type"].get<std::string>();

                if (type == "text") {
                    resp.content.push_back(TextBlock{block["text"].get<std::string>()});
                }
                else if (type == "tool_use") {
                    resp.content.push_back(ToolUseBlock{
                        block["id"].get<std::string>(),
                        block["name"].get<std::string>(),
                        block.value("input", nlohmann::json::object())
                    });
                }
            }
            return resp;
        }
    };

} // namespace claw::api
