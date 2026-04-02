#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace claw::tools {

    // ── ITool ────────────────────────────────────────────────────────────
    // Abstract interface for all agent tools. Each tool must provide:
    //   1. Identity (name, description)
    //   2. An input schema (JSON Schema for the model to understand parameters)
    //   3. An execute method that runs the tool and returns a string result
    //
    // The get_api_definition() method returns the complete tool definition
    // in the format required by the Anthropic Messages API `tools` array.

    class ITool {
    public:
        virtual ~ITool() = default;

        virtual std::string get_name() const = 0;
        virtual std::string get_description() const = 0;
        virtual nlohmann::json get_input_schema() const = 0;
        virtual std::string execute(const nlohmann::json& input) = 0;

        // Returns the full tool definition for the Anthropic API
        nlohmann::json get_api_definition() const {
            return {
                {"name",         get_name()},
                {"description",  get_description()},
                {"input_schema", get_input_schema()}
            };
        }
    };

} // namespace claw::tools
