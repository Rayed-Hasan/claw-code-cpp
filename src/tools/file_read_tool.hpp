#pragma once
#include "itool.hpp"

namespace claw::tools {

    class FileReadTool : public ITool {
    public:
        std::string get_name() const override;
        std::string get_description() const override;
        nlohmann::json get_input_schema() const override;
        std::string execute(const nlohmann::json& input) override;
    };

} // namespace claw::tools
