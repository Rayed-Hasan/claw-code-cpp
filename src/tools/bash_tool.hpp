#pragma once
#include "itool.hpp"

namespace claw::tools {

    // ── BashTool ─────────────────────────────────────────────────────────
    // Executes shell commands via a process pipe and returns stdout+stderr.
    // Includes a configurable timeout and output size cap to prevent
    // runaway commands from hanging the agent.

    class BashTool : public ITool {
    public:
        explicit BashTool(int timeout_sec = 30, size_t max_output_bytes = 50 * 1024);

        std::string get_name() const override;
        std::string get_description() const override;
        nlohmann::json get_input_schema() const override;
        std::string execute(const nlohmann::json& input) override;

    private:
        int m_timeout_sec;
        size_t m_max_output_bytes;
    };

} // namespace claw::tools
