#pragma once

#include "flogoritma/flog_node.hpp"

#include <memory>
#include <string>
#include <utility>

namespace flogoritma {

class SelectionNode final : public FlogNode {
public:
    SelectionNode(std::string id, std::string condition, Geometry geometry = {})
        : FlogNode(std::move(id), geometry), condition_(std::move(condition)) {}

    const std::string& condition() const noexcept { return condition_; }
    void setCondition(std::string condition) { condition_ = std::move(condition); }

    void setTrueNext(const FlogNodePtr& node) noexcept { trueNext_ = node; }
    void setFalseNext(const FlogNodePtr& node) noexcept { falseNext_ = node; }
    FlogNodePtr trueNext() const noexcept { return trueNext_.lock(); }
    FlogNodePtr falseNext() const noexcept { return falseNext_.lock(); }

    void execute() override {}
    std::string generateCode() const override {
        return "    if (" + condition_ + ") {\n"
               "        // ramo true\n"
               "    } else {\n"
               "        // ramo false\n"
               "    }\n";
    }

private:
    std::string condition_;
    std::weak_ptr<FlogNode> trueNext_;
    std::weak_ptr<FlogNode> falseNext_;
};

} // namespace flogoritma