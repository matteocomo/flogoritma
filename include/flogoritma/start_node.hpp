#pragma once

#include "flogoritma/flog_node.hpp"

namespace flogoritma {

class StartNode final : public FlogNode {
public:
    using FlogNode::FlogNode;

    void execute() override {}
    std::string generateCode() const override { return "int main() {\n"; }
};

} // namespace flogoritma