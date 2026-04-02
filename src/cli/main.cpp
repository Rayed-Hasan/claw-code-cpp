#include <iostream>
#include <string>
#include <cstdlib>
#include <CLI/CLI.hpp>
#include <fmt/core.h>
#include <fmt/color.h>

#include "../api/client.hpp"
#include "../api/types.hpp"
#include "../runtime/session.hpp"
#include "../tools/tool_registry.hpp"
#include "../tools/bash_tool.hpp"
#include "../tools/file_read_tool.hpp"
#include "../tools/file_write_tool.hpp"

// ── Build the default tool registry ──────────────────────────────────────

static claw::tools::ToolRegistry build_tool_registry() {
    claw::tools::ToolRegistry registry;
    registry.register_tool(std::make_unique<claw::tools::BashTool>());
    registry.register_tool(std::make_unique<claw::tools::FileReadTool>());
    registry.register_tool(std::make_unique<claw::tools::FileWriteTool>());
    return registry;
}

// ── Resolve the API key from environment ─────────────────────────────────

static std::string get_api_key() {
    const char* key = std::getenv("ANTHROPIC_API_KEY");
    if (!key || std::string(key).empty()) {
        fmt::print(stderr,
            fg(fmt::color::red),
            "[Error] ANTHROPIC_API_KEY environment variable not set.\n"
            "Set it with:  $env:ANTHROPIC_API_KEY=\"sk-ant-...\"\n"
        );
        std::exit(1);
    }
    return std::string(key);
}

// ── Tool event display callback ──────────────────────────────────────────

static void on_tool_event(
    const std::string& tool_name,
    const std::string& input_summary,
    const std::string& output
) {
    if (output.empty()) {
        // Tool is about to execute
        fmt::print(fg(fmt::color::yellow), "\n  [Tool] {} ", tool_name);
        // Show a short preview of the input
        std::string preview = input_summary.substr(0, 80);
        if (input_summary.size() > 80) preview += "...";
        fmt::print(fg(fmt::color::dim_gray), "{}\n", preview);
    }
    // We don't print the full output here — the model will summarize it
}

// ── Interactive REPL Mode ────────────────────────────────────────────────

static int run_repl(claw::api::Client& client, claw::tools::ToolRegistry& registry) {
    claw::runtime::Session session(client, registry);
    session.set_tool_event_callback(on_tool_event);

    fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold,
        "\n  Claw Code (C++ Port) v0.1.0\n");
    fmt::print(fg(fmt::color::dim_gray),
        "  Tools: ");
    for (const auto& name : registry.tool_names()) {
        fmt::print(fg(fmt::color::green), "{} ", name);
    }
    fmt::print("\n");
    fmt::print(fg(fmt::color::dim_gray),
        "  Type '/quit' to exit, '/clear' to reset conversation.\n\n");

    while (true) {
        fmt::print(fg(fmt::color::cyan) | fmt::emphasis::bold, "> ");
        
        std::string input;
        if (!std::getline(std::cin, input)) break;  // EOF
        if (input.empty()) continue;

        // Handle slash commands
        if (input == "/quit" || input == "/exit") {
            fmt::print(fg(fmt::color::dim_gray), "Goodbye.\n");
            break;
        }
        if (input == "/clear") {
            session.clear_history();
            fmt::print(fg(fmt::color::dim_gray), "Conversation cleared.\n\n");
            continue;
        }
        if (input == "/usage") {
            auto usage = session.get_usage();
            fmt::print(fg(fmt::color::dim_gray),
                "  Input: {} tokens | Output: {} tokens | "
                "API calls: {} | Tool calls: {}\n\n",
                usage.input_tokens, usage.output_tokens,
                usage.api_calls, usage.tool_calls
            );
            continue;
        }

        // Run the agentic turn
        std::string response = session.run_turn(input);
        fmt::print("\n{}\n\n", response);
    }

    // Print final usage on exit
    auto usage = session.get_usage();
    fmt::print(fg(fmt::color::dim_gray),
        "\n  Session totals — In: {} | Out: {} | API: {} | Tools: {}\n",
        usage.input_tokens, usage.output_tokens,
        usage.api_calls, usage.tool_calls
    );

    return 0;
}

// ── One-Shot Prompt Mode ─────────────────────────────────────────────────

static int run_prompt(
    claw::api::Client& client,
    claw::tools::ToolRegistry& registry,
    const std::string& prompt
) {
    claw::runtime::Session session(client, registry);
    session.set_tool_event_callback(on_tool_event);

    std::string response = session.run_turn(prompt);
    fmt::print("\n{}\n", response);

    auto usage = session.get_usage();
    fmt::print(fg(fmt::color::dim_gray),
        "\n  [In: {} | Out: {} | API: {} | Tools: {}]\n",
        usage.input_tokens, usage.output_tokens,
        usage.api_calls, usage.tool_calls
    );

    return 0;
}

// ── Main ─────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    CLI::App app{"Claw Code (C++ Port) — AI Coding Agent CLI"};

    std::string prompt_text;
    std::string model = "claude-sonnet-4-20250514";

    app.add_option("--model,-m", model, "Model to use (default: claude-sonnet-4-20250514)");

    auto* prompt_cmd = app.add_subcommand("prompt", "Send a one-shot prompt");
    prompt_cmd->add_option("message", prompt_text, "The prompt text")->required();

    CLI11_PARSE(app, argc, argv);

    // Set up API client and tools
    std::string api_key = get_api_key();

    claw::api::ClientConfig config;
    config.api_key = api_key;
    config.model   = model;

    claw::api::Client client(std::move(config));
    auto registry = build_tool_registry();

    // Route to the right mode
    if (prompt_cmd->parsed()) {
        return run_prompt(client, registry, prompt_text);
    }

    // Default: interactive REPL
    return run_repl(client, registry);
}
