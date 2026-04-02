#include "file_write_tool.hpp"
#include <fstream>
#include <filesystem>
#include <fmt/core.h>

namespace fs = std::filesystem;

namespace claw::tools {

    std::string FileWriteTool::get_name() const {
        return "FileWriteTool";
    }

    std::string FileWriteTool::get_description() const {
        return
            "Writes (or overwrites) content to a file at the specified path. "
            "Creates any necessary parent directories automatically. "
            "Use this to create new source files, modify configurations, "
            "write scripts, or save any text content.";
    }

    nlohmann::json FileWriteTool::get_input_schema() const {
        return {
            {"type", "object"},
            {"properties", {
                {"path", {
                    {"type", "string"},
                    {"description", "Absolute or relative path to the file to write."}
                }},
                {"content", {
                    {"type", "string"},
                    {"description", "The full text content to write to the file."}
                }}
            }},
            {"required", nlohmann::json::array({"path", "content"})}
        };
    }

    std::string FileWriteTool::execute(const nlohmann::json& input) {
        if (!input.contains("path") || !input["path"].is_string()) {
            return "[FileWriteTool Error] Input must contain a 'path' string.";
        }
        if (!input.contains("content") || !input["content"].is_string()) {
            return "[FileWriteTool Error] Input must contain a 'content' string.";
        }

        std::string path    = input["path"].get<std::string>();
        std::string content = input["content"].get<std::string>();

        // Create parent directories if they don't exist
        fs::path file_path(path);
        if (file_path.has_parent_path()) {
            std::error_code ec;
            fs::create_directories(file_path.parent_path(), ec);
            if (ec) {
                return fmt::format(
                    "[FileWriteTool Error] Cannot create directories for {}: {}",
                    path, ec.message()
                );
            }
        }

        std::ofstream file(path, std::ios::trunc);
        if (!file.is_open()) {
            return fmt::format("[FileWriteTool Error] Cannot open file for writing: {}", path);
        }

        file << content;
        file.close();

        if (file.fail()) {
            return fmt::format("[FileWriteTool Error] Write failed for: {}", path);
        }

        auto written_size = fs::file_size(path);
        return fmt::format("Successfully wrote {} bytes to {}", written_size, path);
    }

} // namespace claw::tools
