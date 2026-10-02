#pragma once

#include "flogoritma/flog_node.hpp"

namespace flogoritma {

class EndNode final : public FlogNode {
public:
    using FlogNode::FlogNode;

    void execute() override {}
    std::string generateCode() const override { return "    return 0;\n}\n"; }
};

} // namespace flogoritma