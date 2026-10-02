#pragma once

#include "flogoritma/geometry.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace flogoritma {

class FlogNode;
using FlogNodePtr = std::shared_ptr<FlogNode>;

class FlogNode {
public:
    FlogNode(std::string id, Geometry geometry = {})
        : id_(std::move(id)), geometry_(geometry) {
        if (id_.empty()) {
            throw std::invalid_argument("FlogNode id cannot be empty");
        }
    }

    virtual ~FlogNode() = default;

    const std::string& id() const noexcept { return id_; }
    const Geometry& geometry() const noexcept { return geometry_; }
    void setGeometry(Geometry geometry) noexcept { geometry_ = geometry; }

    void setNext(const FlogNodePtr& node) noexcept { next_ = node; }
    FlogNodePtr next() const noexcept { return next_.lock(); }

    virtual void execute() = 0;
    virtual std::string generateCode() const = 0;

private:
    std::string id_;
    Geometry geometry_;
    std::weak_ptr<FlogNode> next_;
};

} // namespace flogoritma