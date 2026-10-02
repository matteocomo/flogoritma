#include "flogoritma/code_exporter.hpp"

#include "flogoritma/end_node.hpp"
#include "flogoritma/output_node.hpp"
#include "flogoritma/selection_node.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

namespace flogoritma {
namespace {

struct FlowResult {
    bool reachesJoin = false;
    bool allPathsEnd = false;
};

class ActiveNodeGuard {
public:
    ActiveNodeGuard(std::unordered_set<const FlogNode*>& activeNodes,
                    const FlogNode* node)
        : activeNodes_(activeNodes), node_(node) {}

    ~ActiveNodeGuard() { activeNodes_.erase(node_); }

private:
    std::unordered_set<const FlogNode*>& activeNodes_;
    const FlogNode* node_;
};

std::string indent(int depth) {
    return std::string(static_cast<std::size_t>(depth) * 4U, ' ');
}

FlowResult emitSequence(const FlogNodePtr& node, const FlogNode* stopAt,
                        int depth, std::unordered_set<const FlogNode*>& activeNodes,
                        std::ostringstream& source) {
    if (!node) {
        return {stopAt != nullptr, false};
    }
    if (node.get() == stopAt) {
        return {true, false};
    }
    if (!activeNodes.insert(node.get()).second) {
        throw std::runtime_error("Il diagramma contiene un ciclo: aggiungere un nodo While prima di esportarlo");
    }
    const ActiveNodeGuard guard(activeNodes, node.get());

    if (std::dynamic_pointer_cast<StartNode>(node)) {
        throw std::runtime_error("StartNode puo comparire solo all'inizio del diagramma");
    }

    if (const auto output = std::dynamic_pointer_cast<OutputNode>(node)) {
        if (output->expression().empty()) {
            throw std::runtime_error("OutputNode senza espressione: " + output->id());
        }
        source << indent(depth) << "std::cout << " << output->expression()
               << " << std::endl;\n";
        return emitSequence(output->next(), stopAt, depth, activeNodes, source);
    }

    if (const auto selection = std::dynamic_pointer_cast<SelectionNode>(node)) {
        if (selection->condition().empty()) {
            throw std::runtime_error("SelectionNode senza condizione: " + selection->id());
        }

        const FlogNodePtr join = selection->next();
        source << indent(depth) << "if (" << selection->condition() << ") {\n";
        const FlowResult trueResult = emitSequence(
            selection->trueNext(), join.get(), depth + 1, activeNodes, source);
        source << indent(depth) << "} else {\n";
        const FlowResult falseResult = emitSequence(
            selection->falseNext(), join.get(), depth + 1, activeNodes, source);
        source << indent(depth) << "}\n";

        if (!join) {
            return {false, trueResult.allPathsEnd && falseResult.allPathsEnd};
        }

        const bool anyBranchReachesJoin = trueResult.reachesJoin || falseResult.reachesJoin;
        if (!anyBranchReachesJoin) {
            return {false, trueResult.allPathsEnd && falseResult.allPathsEnd};
        }

        const FlowResult continuation = emitSequence(
            join, stopAt, depth, activeNodes, source);
        const bool truePathEnds = trueResult.allPathsEnd ||
                                  (trueResult.reachesJoin && continuation.allPathsEnd);
        const bool falsePathEnds = falseResult.allPathsEnd ||
                                   (falseResult.reachesJoin && continuation.allPathsEnd);
        return {continuation.reachesJoin, truePathEnds && falsePathEnds};
    }

    if (std::dynamic_pointer_cast<EndNode>(node)) {
        source << indent(depth) << "return 0;\n";
        return {false, true};
    }

    throw std::runtime_error("Tipo di nodo non supportato nell'esportazione: " + node->id());
}

} // namespace

CodeExporter::CodeExporter(std::shared_ptr<StartNode> startNode)
    : startNode_(std::move(startNode)) {
    if (!startNode_) {
        throw std::invalid_argument("CodeExporter richiede un puntatore StartNode valido");
    }
}

std::string CodeExporter::generate() const {
    std::ostringstream source;
    source << "#include <iostream>\n\nint main() {\n";

    std::unordered_set<const FlogNode*> activeNodes;
    const FlowResult result = emitSequence(
        startNode_->next(), nullptr, 1, activeNodes, source);
    if (!result.allPathsEnd) {
        throw std::runtime_error("Non tutti i percorsi del diagramma raggiungono EndNode");
    }

    source << "}\n";
    return source.str();
}

void CodeExporter::writeToFile(const std::filesystem::path& path) const {
    const std::string generatedSource = generate();
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Impossibile aprire il file: " + path.string());
    }
    file << generatedSource;
    if (!file) {
        throw std::runtime_error("Errore durante la scrittura del file: " + path.string());
    }
}

} // namespace flogoritma