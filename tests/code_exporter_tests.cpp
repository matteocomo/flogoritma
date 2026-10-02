#include "flogoritma/code_exporter.hpp"
#include "flogoritma/end_node.hpp"
#include "flogoritma/output_node.hpp"
#include "flogoritma/selection_node.hpp"

#include <memory>
#include <stdexcept>

int main() {
    using namespace flogoritma;

    const auto start = std::make_shared<StartNode>("start");
    const auto selection = std::make_shared<SelectionNode>("if_1", "ready");
    const auto finished = std::make_shared<EndNode>("end");
    const auto unfinished = std::make_shared<OutputNode>("output_1", "value");
    start->setNext(selection);
    selection->setTrueNext(unfinished);
    selection->setFalseNext(finished);

    try {
        CodeExporter(start).generate();
    } catch (const std::runtime_error&) {
        return 0;
    }

    return 1;
}