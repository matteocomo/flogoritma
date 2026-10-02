#pragma once

#include "flogoritma/start_node.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace flogoritma {

class CodeExporter {
public:
    explicit CodeExporter(std::shared_ptr<StartNode> startNode);

    std::string generate() const;
    void writeToFile(const std::filesystem::path& path = "main.cpp") const;

private:
    std::shared_ptr<StartNode> startNode_;
};

} // namespace flogoritma