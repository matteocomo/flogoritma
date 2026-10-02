#pragma once

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QString>

#include <cmath>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace flogoritma {

enum class VariableType { Integer, Real, Boolean, String };
enum class NodeType { Start, Input, Output, Assignment, If, While, End };

inline std::runtime_error jsonError(const QString& message) {
    return std::runtime_error(message.toStdString());
}

inline QString requiredString(const QJsonObject& object, const QString& key) {
    const QJsonValue value = object.value(key);
    if (!value.isString()) {
        throw jsonError(QString("Expected string field: %1").arg(key));
    }
    return value.toString();
}

inline QString variableTypeName(VariableType type) {
    switch (type) {
    case VariableType::Integer: return "integer";
    case VariableType::Real: return "real";
    case VariableType::Boolean: return "boolean";
    case VariableType::String: return "string";
    }
    throw jsonError("Unknown variable type");
}

inline VariableType variableTypeFromName(const QString& name) {
    if (name == "integer") return VariableType::Integer;
    if (name == "real") return VariableType::Real;
    if (name == "boolean") return VariableType::Boolean;
    if (name == "string") return VariableType::String;
    throw jsonError(QString("Unknown variable type: %1").arg(name));
}

inline QString nodeTypeName(NodeType type) {
    switch (type) {
    case NodeType::Start: return "start";
    case NodeType::Input: return "input";
    case NodeType::Output: return "output";
    case NodeType::Assignment: return "assignment";
    case NodeType::If: return "if";
    case NodeType::While: return "while";
    case NodeType::End: return "end";
    }
    throw jsonError("Unknown node type");
}

inline NodeType nodeTypeFromName(const QString& name) {
    if (name == "start") return NodeType::Start;
    if (name == "input") return NodeType::Input;
    if (name == "output") return NodeType::Output;
    if (name == "assignment") return NodeType::Assignment;
    if (name == "if") return NodeType::If;
    if (name == "while") return NodeType::While;
    if (name == "end") return NodeType::End;
    throw jsonError(QString("Unknown node type: %1").arg(name));
}

struct ProjectMetadata {
    QString name;
    QString author;
    QString createdAt;

    QJsonObject toJson() const {
        return {{"name", name}, {"author", author}, {"createdAt", createdAt}};
    }

    static ProjectMetadata fromJson(const QJsonObject& object) {
        return {requiredString(object, "name"),
                requiredString(object, "author"),
                requiredString(object, "createdAt")};
    }
};

struct Variable {
    QString name;
    VariableType type;

    QJsonObject toJson() const {
        return {{"name", name}, {"type", variableTypeName(type)}};
    }

    static Variable fromJson(const QJsonObject& object) {
        return {requiredString(object, "name"),
                variableTypeFromName(requiredString(object, "type"))};
    }
};

struct Node {
    QString id;
    NodeType type;
    QString text;
    QString next;
    QString trueNext;
    QString falseNext;
    double x = 0.0;
    double y = 0.0;
    QString variable;

    QJsonObject toJson() const {
        QJsonObject object{{"id", id}, {"type", nodeTypeName(type)}, {"text", text},
                           {"x", x}, {"y", y}};
        if (type == NodeType::Input) {
            object.insert("variable", variable);
        }
        if (type == NodeType::If || type == NodeType::While) {
            object.insert("trueNext", trueNext);
            object.insert("falseNext", falseNext);
        } else if (type != NodeType::End) {
            object.insert("next", next);
        }
        return object;
    }

    static Node fromJson(const QJsonObject& object) {
        Node node{requiredString(object, "id"),
                  nodeTypeFromName(requiredString(object, "type")),
                  requiredString(object, "text"), {}, {}, {}};
        const QJsonValue xValue = object.value("x");
        const QJsonValue yValue = object.value("y");
        if ((object.contains("x") && !xValue.isDouble()) ||
            (object.contains("y") && !yValue.isDouble())) {
            throw jsonError("Node positions must be numbers");
        }
        node.x = xValue.toDouble();
        node.y = yValue.toDouble();
        if (node.type == NodeType::Input && object.contains("variable")) {
            const QJsonValue variableValue = object.value("variable");
            if (!variableValue.isString()) {
                throw jsonError("Input variable must be a string");
            }
            node.variable = variableValue.toString();
        }
        if (node.type == NodeType::If || node.type == NodeType::While) {
            node.trueNext = requiredString(object, "trueNext");
            node.falseNext = requiredString(object, "falseNext");
        } else if (node.type != NodeType::End) {
            node.next = requiredString(object, "next");
        }
        return node;
    }
};

struct Flowchart {
    static constexpr int CurrentFormatVersion = 1;

    ProjectMetadata project;
    std::vector<Variable> variables;
    std::vector<Node> nodes;

    QJsonObject toJson() const {
        validate();

        QJsonArray variableArray;
        for (const Variable& variable : variables) {
            variableArray.append(variable.toJson());
        }

        QJsonArray nodeArray;
        for (const Node& node : nodes) {
            nodeArray.append(node.toJson());
        }

        return {{"formatVersion", CurrentFormatVersion},
                {"project", project.toJson()},
                {"variables", variableArray},
                {"nodes", nodeArray}};
    }

    QByteArray toJsonBytes(QJsonDocument::JsonFormat format = QJsonDocument::Indented) const {
        return QJsonDocument(toJson()).toJson(format);
    }

    static Flowchart fromJson(const QByteArray& json) {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            throw jsonError(QString("Invalid .flog JSON: %1").arg(parseError.errorString()));
        }

        const QJsonObject root = document.object();
        const QJsonValue version = root.value("formatVersion");
        if (!version.isDouble() || version.toInt() != CurrentFormatVersion) {
            throw jsonError("Unsupported or missing formatVersion");
        }

        const QJsonValue projectValue = root.value("project");
        const QJsonValue variablesValue = root.value("variables");
        const QJsonValue nodesValue = root.value("nodes");
        if (!projectValue.isObject() || !variablesValue.isArray() || !nodesValue.isArray()) {
            throw jsonError("Expected project object and variables/nodes arrays");
        }

        Flowchart flowchart;
        flowchart.project = ProjectMetadata::fromJson(projectValue.toObject());
        for (const QJsonValue& value : variablesValue.toArray()) {
            if (!value.isObject()) throw jsonError("Each variable must be an object");
            flowchart.variables.push_back(Variable::fromJson(value.toObject()));
        }
        int nodeIndex = 0;
        for (const QJsonValue& value : nodesValue.toArray()) {
            if (!value.isObject()) throw jsonError("Each node must be an object");
            const QJsonObject object = value.toObject();
            Node node = Node::fromJson(object);
            if (node.type == NodeType::Input && node.variable.isEmpty() &&
                flowchart.variables.size() == 1) {
                node.variable = flowchart.variables.front().name;
            }
            if (!object.contains("x")) node.x = 100.0 + (nodeIndex % 4) * 300.0;
            if (!object.contains("y")) node.y = 80.0 + (nodeIndex / 4) * 150.0;
            flowchart.nodes.push_back(std::move(node));
            ++nodeIndex;
        }

        flowchart.validate();
        return flowchart;
    }

    void validate() const {
        if (project.name.isEmpty() || project.author.isEmpty() || project.createdAt.isEmpty()) {
            throw jsonError("Project name, author, and createdAt are required");
        }

        std::unordered_set<std::string> variableNames;
        for (const Variable& variable : variables) {
            if (variable.name.isEmpty() || !variableNames.insert(variable.name.toStdString()).second) {
                throw jsonError("Variable names must be non-empty and unique");
            }
        }

        std::unordered_set<std::string> ids;
        int startCount = 0;
        for (const Node& node : nodes) {
            if (node.id.isEmpty() || !ids.insert(node.id.toStdString()).second) {
                throw jsonError("Node IDs must be non-empty and unique");
            }
            if (!std::isfinite(node.x) || !std::isfinite(node.y)) {
                throw jsonError("Node positions must be finite numbers");
            }
            if (node.type == NodeType::Start) ++startCount;
        }
        if (startCount != 1) throw jsonError("A flowchart must contain exactly one start node");

        const auto requireTarget = [&ids](const QString& target) {
            if (!target.isEmpty() && ids.count(target.toStdString()) == 0) {
                throw jsonError(QString("Unknown node reference: %1").arg(target));
            }
        };
        for (const Node& node : nodes) {
            if (node.type == NodeType::If || node.type == NodeType::While) {
                requireTarget(node.trueNext);
                requireTarget(node.falseNext);
            } else if (node.type != NodeType::End) {
                requireTarget(node.next);
            }
            if (node.type == NodeType::Input && !node.variable.isEmpty() &&
                variableNames.count(node.variable.toStdString()) == 0) {
                throw jsonError(QString("Unknown input variable: %1").arg(node.variable));
            }
        }
    }
};

} // namespace flogoritma