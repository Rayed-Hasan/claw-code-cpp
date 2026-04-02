#include "session.hpp"
#include <fmt/core.h>

namespace claw::runtime {

    Session::Session(
        api::Client& client,
        tools::ToolRegistry& registry,
        SessionConfig config
    ) : m_client(client), m_registry(registry), m_config(std::move(config)) {}

    void Session::set_tool_event_callback(ToolEventCallback cb) {
        m_tool_event_cb = std::move(cb);
    }

    const std::vector<api::Message>& Session::get_history() const {
        return m_messages;
    }

    UsageStats Session::get_usage() const {
        return m_usage;
    }

    void Session::clear_history() {
        m_messages.clear();
        m_usage = {};
    }

    void Session::compact_history() {
        if (m_messages.size() > m_config.max_history_turns) {
            size_t trim = m_messages.size() - m_config.max_history_turns;
            m_messages.erase(m_messages.begin(),
                             m_messages.begin() + static_cast<long>(trim));
        }
    }

    // ── Core Agentic Turn Loop ───────────────────────────────────────────
    //
    // This is the heart of the agent. It implements the loop:
    //
    //   1. Add user message to history
    //   2. Send full history + tool definitions to the API
    //   3. If stop_reason == "tool_use":
    //      a. Add the assistant's response (with tool_use blocks) to history
    //      b. Execute each requested tool
    //      c. Add tool results to history as a "user" message
    //      d. Go to step 2
    //   4. If stop_reason == "end_turn":
    //      a. Add the assistant's response to history
    //      b. Return the text to the user

    std::string Session::run_turn(const std::string& user_input) {
        if (user_input.empty()) return "";

        // Step 1: Add user message
        m_messages.push_back(api::Message::user_text(user_input));
        compact_history();

        // Get tool definitions once (they don't change mid-turn)
        nlohmann::json tools_json = m_registry.get_tool_definitions();

        std::string final_text;
        int iterations = 0;

        // Step 2–4: The agentic loop
        while (iterations < m_config.max_tool_iterations) {
            iterations++;
            m_usage.api_calls++;

            auto response = m_client.send_request(
                m_config.system_prompt, m_messages, tools_json
            );

            if (!response) {
                // API call failed — remove the user message and report
                if (!m_messages.empty()) m_messages.pop_back();
                return "[Error] Failed to get a response from the API.";
            }

            // Track usage
            m_usage.input_tokens  += response->input_tokens;
            m_usage.output_tokens += response->output_tokens;

            // Collect any text the model produced in this turn
            std::string turn_text = response->get_text();
            if (!turn_text.empty()) {
                if (!final_text.empty()) final_text += "\n";
                final_text += turn_text;
            }

            // ── Step 3: Handle tool_use ──────────────────────────────────
            if (response->has_tool_calls()) {
                // Add the full assistant message (text + tool_use blocks) to history
                api::Message assistant_msg;
                assistant_msg.role = "assistant";
                assistant_msg.content = response->content;
                m_messages.push_back(std::move(assistant_msg));

                // Execute each tool and collect results
                auto tool_calls = response->get_tool_calls();
                std::vector<api::ToolResultBlock> results;

                for (const auto& call : tool_calls) {
                    m_usage.tool_calls++;
                    auto result = execute_tool(call);
                    results.push_back(std::move(result));
                }

                // Add tool results as a "user" message (Anthropic API format)
                m_messages.push_back(api::Message::tool_results(results));
                compact_history();

                // Continue the loop — the model will process tool results
                continue;
            }

            // ── Step 4: end_turn — we're done ────────────────────────────
            api::Message assistant_msg;
            assistant_msg.role = "assistant";
            assistant_msg.content = response->content;
            m_messages.push_back(std::move(assistant_msg));

            break;
        }

        if (iterations >= m_config.max_tool_iterations) {
            final_text += "\n[Warning] Reached maximum tool iteration limit.";
        }

        return final_text;
    }

    // ── Tool Execution ───────────────────────────────────────────────────

    api::ToolResultBlock Session::execute_tool(const api::ToolUseBlock& call) {
        api::ToolResultBlock result;
        result.tool_use_id = call.id;

        auto* tool = m_registry.get_tool(call.name);
        if (!tool) {
            result.content = fmt::format("[Error] Unknown tool: {}", call.name);
            result.is_error = true;
            return result;
        }

        // Notify the CLI layer about the tool execution (for display)
        if (m_tool_event_cb) {
            m_tool_event_cb(
                call.name,
                call.input.dump(),
                "" // output not yet known
            );
        }

        try {
            result.content = tool->execute(call.input);
        } catch (const std::exception& e) {
            result.content = fmt::format("[Error] Tool '{}' threw exception: {}",
                                         call.name, e.what());
            result.is_error = true;
        }

        // Notify CLI with the result
        if (m_tool_event_cb) {
            m_tool_event_cb(call.name, call.input.dump(), result.content);
        }

        return result;
    }

} // namespace claw::runtime
