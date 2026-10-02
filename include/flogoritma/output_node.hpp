#pragma once

#include "flogoritma/flog_node.hpp"

#include <string>
#include <utility>

namespace flogoritma {

class OutputNode final : public FlogNode {
public:
    OutputNode(std::string id, std::string expression, Geometry geometry = {})
        : FlogNode(std::move(id), geometry), expression_(std::move(expression)) {}

    const std::string& expression() const noexcept { return expression_; }
    void setExpression(std::string expression) { expression_ = std::move(expression); }

    void execute() override {}
    std::string generateCode() const override {
        return "    std::cout << " + expression_ + " << '\\n';\n";
    }

private:
    std::string expression_;
};

} // namespace flogoritma