#include "file_read_tool.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <fmt/core.h>

namespace fs = std::filesystem;

namespace claw::tools {

    std::string FileReadTool::get_name() const {
        return "FileReadTool";
    }

    std::string FileReadTool::get_description() const {
        return
            "Reads the contents of a file at the specified path and returns "
            "it as a string. Use this to inspect source code, configuration "
            "files, logs, or any text file on the user's system.";
    }

    nlohmann::json FileReadTool::get_input_schema() const {
        return {
            {"type", "object"},
            {"properties", {
                {"path", {
                    {"type", "string"},
                    {"description", "Absolute or relative path to the file to read."}
                }}
            }},
            {"required", nlohmann::json::array({"path"})}
        };
    }

    std::string FileReadTool::execute(const nlohmann::json& input) {
        if (!input.contains("path") || !input["path"].is_string()) {
            return "[FileReadTool Error] Input must contain a 'path' string.";
        }

        std::string path = input["path"].get<std::string>();

        if (!fs::exists(path)) {
            return fmt::format("[FileReadTool Error] File not found: {}", path);
        }
        if (!fs::is_regular_file(path)) {
            return fmt::format("[FileReadTool Error] Not a regular file: {}", path);
        }

        // Check file size — refuse to read files larger than 512 KB
        auto file_size = fs::file_size(path);
        if (file_size > 512 * 1024) {
            return fmt::format(
                "[FileReadTool Error] File is too large ({} bytes, max 524288). "
                "Consider reading specific lines with BashTool instead.", file_size
            );
        }

        std::ifstream file(path);
        if (!file.is_open()) {
            return fmt::format("[FileReadTool Error] Cannot open file: {}", path);
        }

        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

} // namespace claw::tools
