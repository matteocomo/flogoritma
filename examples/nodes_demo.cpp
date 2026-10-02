#include "flogoritma/end_node.hpp"
#include "flogoritma/output_node.hpp"
#include "flogoritma/selection_node.hpp"
#include "flogoritma/start_node.hpp"

#include <iostream>
#include <memory>

int main() {
    using namespace flogoritma;

    const auto start = std::make_shared<StartNode>("start");
    const auto selection = std::make_shared<SelectionNode>("if_1", "value > 0");
    const auto output = std::make_shared<OutputNode>("output_1", "value");
    const auto end = std::make_shared<EndNode>("end");

    start->setNext(selection);
    selection->setTrueNext(output);
    selection->setFalseNext(end);
    output->setNext(end);

    std::cout << start->generateCode()
              << selection->generateCode()
              << output->generateCode()
              << end->generateCode();
}