#include "bash_tool.hpp"
#include <array>
#include <memory>
#include <fmt/core.h>

#ifdef _WIN32
#define POPEN  _popen
#define PCLOSE _pclose
#else
#define POPEN  popen
#define PCLOSE pclose
#endif

namespace claw::tools {

    BashTool::BashTool(int timeout_sec, size_t max_output_bytes)
        : m_timeout_sec(timeout_sec), m_max_output_bytes(max_output_bytes) {}

    std::string BashTool::get_name() const {
        return "BashTool";
    }

    std::string BashTool::get_description() const {
        return
            "Executes a shell command on the user's system and returns the "
            "combined stdout and stderr output. Use this tool for running "
            "build commands, listing files, checking git status, installing "
            "packages, and any other terminal operations.";
    }

    nlohmann::json BashTool::get_input_schema() const {
        return {
            {"type", "object"},
            {"properties", {
                {"command", {
                    {"type", "string"},
                    {"description", "The shell command to execute."}
                }}
            }},
            {"required", nlohmann::json::array({"command"})}
        };
    }

    std::string BashTool::execute(const nlohmann::json& input) {
        if (!input.contains("command") || !input["command"].is_string()) {
            return "[BashTool Error] Input must contain a 'command' string.";
        }

        std::string command = input["command"].get<std::string>();

        // Redirect stderr into stdout so we capture everything
        command += " 2>&1";

        // On Windows, wrap with a timeout via cmd; on Unix use timeout
#ifdef _WIN32
        // Windows doesn't have a simple `timeout` for processes, so we
        // rely on the output size cap and keep it straightforward.
#else
        command = fmt::format("timeout {} {}", m_timeout_sec, command);
#endif

        std::array<char, 4096> buffer;
        std::string result;
        result.reserve(4096);

        std::unique_ptr<FILE, decltype(&PCLOSE)> pipe(POPEN(command.c_str(), "r"), PCLOSE);
        if (!pipe) {
            return "[BashTool Error] Failed to open process pipe.";
        }

        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
            result += buffer.data();

            // Enforce output size cap
            if (result.size() > m_max_output_bytes) {
                result += fmt::format(
                    "\n\n[BashTool] Output truncated at {} bytes.",
                    m_max_output_bytes
                );
                break;
            }
        }

        if (result.empty()) {
            return "<command executed successfully with no output>";
        }
        return result;
    }

} // namespace claw::tools
