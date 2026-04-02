#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "itool.hpp"

namespace claw::tools {

    // ── ToolRegistry ─────────────────────────────────────────────────────
    // Central registry that owns all tool instances. The runtime uses this to:
    //   1. Generate the `tools` JSON array sent to the Anthropic API
    //   2. Look up a tool by name when the API requests a tool_use execution

    class ToolRegistry {
    public:
        // Register a tool. Ownership is transferred to the registry.
        void register_tool(std::unique_ptr<ITool> tool) {
            std::string name = tool->get_name();
            m_tools[name] = std::move(tool);
            m_order.push_back(name);
        }

        // Look up a tool by exact name. Returns nullptr if not found.
        ITool* get_tool(const std::string& name) const {
            auto it = m_tools.find(name);
            return (it != m_tools.end()) ? it->second.get() : nullptr;
        }

        // Generate the complete `tools` array for the Anthropic API request.
        nlohmann::json get_tool_definitions() const {
            nlohmann::json defs = nlohmann::json::array();
            for (const auto& name : m_order) {
                auto it = m_tools.find(name);
                if (it != m_tools.end()) {
                    defs.push_back(it->second->get_api_definition());
                }
            }
            return defs;
        }

        size_t size() const { return m_tools.size(); }

        // List all registered tool names
        std::vector<std::string> tool_names() const {
            return m_order;
        }

    private:
        std::unordered_map<std::string, std::unique_ptr<ITool>> m_tools;
        std::vector<std::string> m_order;  // Preserve insertion order
    };

} // namespace claw::tools
