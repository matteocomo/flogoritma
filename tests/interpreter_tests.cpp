#include "flogoritma/interpreter.hpp"

#include <QCoreApplication>
#include <QFile>

#include <optional>

namespace {

flogoritma::Flowchart loopFlowchart() {
    using namespace flogoritma;
    Flowchart flowchart;
    flowchart.project = {"Loop test", "Test", "2026-10-02T00:00:00Z"};
    flowchart.variables.push_back({"i", VariableType::Integer});
    flowchart.nodes = {
        {"start", NodeType::Start, "Inizio", "init", {}, {}, 0.0, 0.0},
        {"init", NodeType::Assignment, "i = 0", "loop", {}, {}, 0.0, 100.0},
        {"loop", NodeType::While, "i < 3", {}, "out", "end", 0.0, 200.0},
        {"out", NodeType::Output, "i", "increment", {}, {}, 0.0, 300.0},
        {"increment", NodeType::Assignment, "i = i + 1", "loop", {}, {}, 0.0, 400.0},
        {"end", NodeType::End, "Fine", {}, {}, {}, 300.0, 300.0},
    };
    return flowchart;
}

flogoritma::Flowchart inputFlowchart() {
    using namespace flogoritma;
    Flowchart flowchart;
    flowchart.project = {"Input test", "Test", "2026-10-02T00:00:00Z"};
    flowchart.variables.push_back({"value", VariableType::Integer});
    flowchart.nodes = {
        {"start", NodeType::Start, "Inizio", "input", {}, {}, 0.0, 0.0},
        {"input", NodeType::Input, "Inserisci un numero", "output", {}, {}, 0.0, 100.0, "value"},
        {"output", NodeType::Output, "value", "end", {}, {}, 0.0, 200.0},
        {"end", NodeType::End, "Fine", {}, {}, {}, 0.0, 300.0},
    };
    return flowchart;
}

flogoritma::Flowchart conditionalFlowchart() {
    using namespace flogoritma;
    Flowchart flowchart;
    flowchart.project = {"If test", "Test", "2026-10-02T00:00:00Z"};
    flowchart.variables.push_back({"enabled", VariableType::Boolean});
    flowchart.nodes = {
        {"start", NodeType::Start, "Inizio", "if", {}, {}, 0.0, 0.0},
        {"if", NodeType::If, "enabled", {}, "yes", "no", 0.0, 100.0},
        {"yes", NodeType::Output, "'yes'", "end", {}, {}, 0.0, 200.0},
        {"no", NodeType::Output, "'no'", "end", {}, {}, 200.0, 200.0},
        {"end", NodeType::End, "Fine", {}, {}, {}, 0.0, 300.0},
    };
    return flowchart;
}

bool runExampleFlowchart() {
    using namespace flogoritma;
    QFile file(QString::fromUtf8(FLOGORITMA_EXAMPLE_PATH));
    if (!file.open(QIODevice::ReadOnly)) return false;

    FlowchartInterpreter interpreter(Flowchart::fromJson(file.readAll()));
    QStringList outputs;
    int stepCount = 0;
    while (!interpreter.finished() && stepCount < 100) {
        ExecutionResult result = interpreter.step();
        if (result.event == ExecutionEvent::InputRequired) {
            result = interpreter.step(QStringLiteral("0"));
        }
        if (result.event == ExecutionEvent::Output) outputs.append(result.text);
        ++stepCount;
    }
    return interpreter.finished() && outputs == QStringList{"0", "1", "2", "Completato"} &&
           interpreter.variables().value("i").toLongLong() == 3;
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication application(argc, argv);
    using namespace flogoritma;

    FlowchartInterpreter loop(loopFlowchart());
    loop.step();
    loop.step();
    QStringList outputs;
    while (!loop.finished()) {
        const ExecutionResult result = loop.step();
        if (result.event == ExecutionEvent::Output) outputs.append(result.text);
    }
    if (outputs != QStringList{"0", "1", "2"} || loop.variables().value("i").toLongLong() != 3) {
        return 1;
    }

    FlowchartInterpreter conditional(conditionalFlowchart());
    conditional.step();
    conditional.step();
    const ExecutionResult branchOutput = conditional.step();
    if (branchOutput.event != ExecutionEvent::Output || branchOutput.text != "no") return 2;

    Flowchart restricted = conditionalFlowchart();
    restricted.nodes[3].text = "(()=>{while(true){}})()";
    FlowchartInterpreter restrictedInterpreter(std::move(restricted));
    restrictedInterpreter.step();
    restrictedInterpreter.step();
    bool scriptRejected = false;
    try {
        restrictedInterpreter.step();
    } catch (const std::runtime_error&) {
        scriptRejected = true;
    }
    if (!scriptRejected) return 3;

    FlowchartInterpreter input(inputFlowchart());
    input.step();
    const ExecutionResult request = input.step();
    if (request.event != ExecutionEvent::InputRequired || request.text != "Inserisci un numero") {
        return 4;
    }
    bool invalidInputRejected = false;
    try {
        input.step(QStringLiteral("not-a-number"));
    } catch (const std::runtime_error&) {
        invalidInputRejected = true;
    }
    if (!invalidInputRejected || input.currentNodeId() != "input") return 5;
    input.step(QStringLiteral("42"));
    const ExecutionResult output = input.step();
    if (output.event != ExecutionEvent::Output || output.text != "42") return 6;
    input.step();
    if (!input.finished()) return 7;
    if (!runExampleFlowchart()) return 8;
    return 0;
}