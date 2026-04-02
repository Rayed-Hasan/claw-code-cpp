#include "client.hpp"
#include <httplib.h>
#include <fmt/core.h>

namespace claw::api {

    Client::Client(ClientConfig config) : m_config(std::move(config)) {}

    std::optional<TurnResponse> Client::send_request(
        const std::string& system_prompt,
        const std::vector<Message>& messages,
        const nlohmann::json& tools_json
    ) {
        // ── Build the HTTP client ────────────────────────────────────────
        httplib::Client http(m_config.base_url);
        http.set_read_timeout(m_config.timeout_sec, 0);
        http.set_connection_timeout(10, 0);

        httplib::Headers headers = {
            {"x-api-key",         m_config.api_key},
            {"anthropic-version", m_config.api_version},
            {"content-type",      "application/json"}
        };

        // ── Build the JSON payload ───────────────────────────────────────
        nlohmann::json payload;
        payload["model"]      = m_config.model;
        payload["max_tokens"] = m_config.max_tokens;

        if (!system_prompt.empty()) {
            payload["system"] = system_prompt;
        }

        // Serialize our rich Message objects
        nlohmann::json messages_json = nlohmann::json::array();
        for (const auto& msg : messages) {
            messages_json.push_back(msg.to_json());
        }
        payload["messages"] = messages_json;

        // Include tool definitions if we have any registered tools
        if (!tools_json.empty()) {
            payload["tools"] = tools_json;
        }

        // ── Execute the POST request ─────────────────────────────────────
        std::string body = payload.dump();
        auto res = http.Post("/v1/messages", headers, body, "application/json");

        if (!res) {
            fmt::print(stderr, "[API Error] Request failed: {}\n",
                       httplib::to_string(res.error()));
            return std::nullopt;
        }

        if (res->status != 200) {
            fmt::print(stderr, "[API Error] Status {}: {}\n",
                       res->status, res->body);
            return std::nullopt;
        }

        // ── Parse the response ───────────────────────────────────────────
        try {
            auto response_json = nlohmann::json::parse(res->body);
            return TurnResponse::from_json(response_json);
        } catch (const nlohmann::json::exception& e) {
            fmt::print(stderr, "[API Error] JSON parse failure: {}\n", e.what());
            return std::nullopt;
        }
    }

} // namespace claw::api
