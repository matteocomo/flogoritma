#include "flogoritma/code_exporter.hpp"
#include "flogoritma/end_node.hpp"
#include "flogoritma/output_node.hpp"
#include "flogoritma/selection_node.hpp"

#include <iostream>
#include <memory>

int main() {
    using namespace flogoritma;

    const auto start = std::make_shared<StartNode>("start");
    const auto selection = std::make_shared<SelectionNode>("if_1", "42 > 0");
    const auto positive = std::make_shared<OutputNode>("out_positive", "42");
    const auto negative = std::make_shared<OutputNode>("out_negative", "-42");
    const auto end = std::make_shared<EndNode>("end");

    start->setNext(selection);
    selection->setTrueNext(positive);
    selection->setFalseNext(negative);
    positive->setNext(end);
    negative->setNext(end);

    try {
        CodeExporter exporter(start);
        exporter.writeToFile("main.cpp");
        std::cout << "Codice C++ esportato in main.cpp\n";
    } catch (const std::exception& error) {
        std::cerr << "Esportazione non riuscita: " << error.what() << '\n';
        return 1;
    }
}