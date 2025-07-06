
#pragma once

#include "MCPTool.hpp"
#include <string>

class BraveSearchTool : public MCPTool {
public:
    BraveSearchTool();
    nlohmann::json call(const nlohmann::json& args) override;
};
