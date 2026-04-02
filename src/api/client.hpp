#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "types.hpp"

namespace claw::api {

    // ── API Client Configuration ─────────────────────────────────────────

    struct ClientConfig {
        std::string api_key;
        std::string model        = "claude-sonnet-4-20250514";
        std::string base_url     = "https://api.anthropic.com";
        std::string api_version  = "2023-06-01";
        int         max_tokens   = 8192;
        int         timeout_sec  = 120;  // Read timeout for long generations
    };

    // ── API Client ───────────────────────────────────────────────────────
    // Handles all HTTP communication with the Anthropic Messages API.
    // Supports tool definitions so the model can request tool execution.

    class Client {
    public:
        explicit Client(ClientConfig config);

        // Send a full request to the Messages API.
        // `tools_json` is the array of tool definitions (from ToolRegistry).
        // Returns nullopt on network/parse failure.
        std::optional<TurnResponse> send_request(
            const std::string& system_prompt,
            const std::vector<Message>& messages,
            const nlohmann::json& tools_json = nlohmann::json::array()
        );

        const ClientConfig& config() const { return m_config; }

    private:
        ClientConfig m_config;
    };

} // namespace claw::api
