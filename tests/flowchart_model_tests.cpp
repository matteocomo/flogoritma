#include "flowchart.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

int main() {
    using namespace flogoritma;

    Flowchart original;
    original.project = {"Test", "Tester", "2026-10-02T00:00:00Z"};
    original.variables.push_back({"count", VariableType::Integer});
    original.nodes.push_back({"start", NodeType::Start, "Inizio", "input", {}, {}, 123.5, 45.25});
    original.nodes.push_back({"input", NodeType::Input, "Inserisci count", "end", {}, {},
                              260.0, 45.25, "count"});
    original.nodes.push_back({"end", NodeType::End, "Fine", {}, {}, {}, 400.0, 210.0});

    const QByteArray serialized = original.toJsonBytes();
    const Flowchart loaded = Flowchart::fromJson(serialized);
    if (loaded.variables.size() != 1 || loaded.variables[0].name != "count" ||
        loaded.variables[0].type != VariableType::Integer || loaded.nodes.size() != 3 ||
        loaded.nodes[0].x != 123.5 || loaded.nodes[0].y != 45.25 ||
        loaded.nodes[1].variable != "count" || loaded.nodes[2].x != 400.0) {
        return 1;
    }

    QJsonObject legacyRoot = QJsonDocument::fromJson(serialized).object();
    QJsonArray legacyNodes = legacyRoot.value("nodes").toArray();
    for (int index = 0; index < legacyNodes.size(); ++index) {
        QJsonObject node = legacyNodes[index].toObject();
        node.remove("x");
        node.remove("y");
        if (node.value("type").toString() == "input") node.remove("variable");
        legacyNodes[index] = node;
    }
    legacyRoot.insert("nodes", legacyNodes);
    const Flowchart legacy = Flowchart::fromJson(QJsonDocument(legacyRoot).toJson());
    if (legacy.nodes[0].x == legacy.nodes[1].x && legacy.nodes[0].y == legacy.nodes[1].y) {
        return 2;
    }
    if (legacy.nodes[1].variable != "count") return 3;

    QJsonObject root = QJsonDocument::fromJson(serialized).object();
    QJsonArray nodes = root.value("nodes").toArray();
    QJsonObject firstNode = nodes[0].toObject();
    firstNode.insert("next", "missing-node");
    nodes[0] = firstNode;
    root.insert("nodes", nodes);

    bool unknownReferenceRejected = false;
    try {
        Flowchart::fromJson(QJsonDocument(root).toJson());
    } catch (const std::runtime_error&) {
        unknownReferenceRejected = true;
    }
    if (!unknownReferenceRejected) return 4;

    QJsonObject unknownInputRoot = QJsonDocument::fromJson(serialized).object();
    QJsonArray unknownInputNodes = unknownInputRoot.value("nodes").toArray();
    QJsonObject inputNode = unknownInputNodes[1].toObject();
    inputNode.insert("variable", "missing-variable");
    unknownInputNodes[1] = inputNode;
    unknownInputRoot.insert("nodes", unknownInputNodes);
    try {
        Flowchart::fromJson(QJsonDocument(unknownInputRoot).toJson());
    } catch (const std::runtime_error&) {
        return 0;
    }

    QJsonObject malformedRoot = QJsonDocument::fromJson(serialized).object();
    QJsonArray malformedNodes = malformedRoot.value("nodes").toArray();
    QJsonObject malformedFirstNode = malformedNodes[0].toObject();
    malformedFirstNode.insert("x", "not-a-number");
    malformedNodes[0] = malformedFirstNode;
    malformedRoot.insert("nodes", malformedNodes);
    try {
        Flowchart::fromJson(QJsonDocument(malformedRoot).toJson());
    } catch (const std::runtime_error&) {
        return 0;
    }
    return 5;
}