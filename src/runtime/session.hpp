#pragma once

#include <string>
#include <vector>
#include <functional>
#include "../api/client.hpp"
#include "../api/types.hpp"
#include "../tools/tool_registry.hpp"

namespace claw::runtime {

    // ── Session Configuration ────────────────────────────────────────────

    struct SessionConfig {
        int    max_tool_iterations = 25;    // Safety cap on tool-use loop depth
        size_t max_history_turns   = 24;    // Max message count before compaction
        std::string system_prompt  =
            "You are Claw Code (C++ Port), an expert AI coding assistant running "
            "in the user's terminal. You have access to tools for executing shell "
            "commands, reading files, and writing files. Use these tools proactively "
            "to help the user. Always explain what you're doing before using a tool. "
            "Be concise, accurate, and helpful.";
    };

    // ── Usage Tracking ───────────────────────────────────────────────────

    struct UsageStats {
        int input_tokens  = 0;
        int output_tokens = 0;
        int tool_calls    = 0;
        int api_calls     = 0;
    };

    // ── Callback for streaming tool events to the CLI ────────────────────

    using ToolEventCallback = std::function<void(
        const std::string& tool_name,
        const std::string& tool_input_summary,
        const std::string& tool_output
    )>;

    // ── Session ──────────────────────────────────────────────────────────
    // The core agent runtime. Manages conversation history and implements
    // the agentic tool-use loop:
    //
    //   User prompt → API call → [tool_use?] → execute tool → send result
    //     → API call → [tool_use?] → ... → [end_turn] → return text
    //
    // This loop is what makes the system an AGENT, not just a chatbot.

    class Session {
    public:
        Session(
            api::Client& client,
            tools::ToolRegistry& registry,
            SessionConfig config = {}
        );

        // Run a full agentic turn: sends the prompt, handles all tool_use
        // loops, and returns the final assistant text when the model stops.
        std::string run_turn(const std::string& user_input);

        // Accessors
        const std::vector<api::Message>& get_history() const;
        UsageStats get_usage() const;
        void clear_history();
        void set_tool_event_callback(ToolEventCallback cb);

    private:
        api::Client&           m_client;
        tools::ToolRegistry&   m_registry;
        SessionConfig          m_config;
        std::vector<api::Message> m_messages;
        UsageStats             m_usage;
        ToolEventCallback      m_tool_event_cb;

        // Execute a single tool call and return the result block
        api::ToolResultBlock execute_tool(const api::ToolUseBlock& call);

        // Compact history to stay within context window limits
        void compact_history();
    };

} // namespace claw::runtime
