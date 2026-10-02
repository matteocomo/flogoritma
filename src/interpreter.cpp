#include "flogoritma/interpreter.hpp"

#include <QJSValue>
#include <QRegularExpression>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace flogoritma {
namespace {

std::runtime_error runtimeError(const QString& nodeId, const QString& message) {
    return std::runtime_error(QString("Nodo %1: %2").arg(nodeId, message).toStdString());
}

bool validIdentifier(const QString& value) {
    static const QRegularExpression pattern(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*$"));
    return pattern.match(value).hasMatch();
}

QVariant defaultValue(VariableType type) {
    switch (type) {
    case VariableType::Integer: return QVariant::fromValue<qlonglong>(0);
    case VariableType::Real: return 0.0;
    case VariableType::Boolean: return false;
    case VariableType::String: return QString();
    }
    throw std::logic_error("Tipo di variabile sconosciuto");
}

void validateExpression(const QString& expression,
                        const QHash<QString, VariableType>& variables,
                        const QString& nodeId) {
    static const QRegularExpression stringPattern(
        QStringLiteral("\"(?:\\\\.|[^\"\\\\])*\"|'(?:\\\\.|[^'\\\\])*'"));
    static const QRegularExpression numberPattern(
        QStringLiteral("(?:\\d+(?:\\.\\d*)?|\\.\\d+)(?:[eE][+-]?\\d+)?"));
    static const QRegularExpression identifierPattern(QStringLiteral("[A-Za-z_][A-Za-z0-9_]*"));
    static const QRegularExpression operatorPattern(
        QStringLiteral("(?:===|!==|==|!=|<=|>=|&&|\\|\\||\\*\\*|[+\\-*/%<>()!?:\\s])"));

    QString remaining = expression;
    remaining.remove(stringPattern);
    if (remaining.contains(QStringLiteral("++")) || remaining.contains(QStringLiteral("--"))) {
        throw runtimeError(nodeId, "Gli operatori di incremento non sono supportati");
    }
    remaining.remove(numberPattern);

    const auto identifiers = identifierPattern.globalMatch(remaining);
    for (auto it = identifiers; it.hasNext();) {
        const QString identifier = it.next().captured();
        if (identifier != "true" && identifier != "false" && !variables.contains(identifier)) {
            throw runtimeError(nodeId, QString("Identificatore non dichiarato: %1").arg(identifier));
        }
    }

    remaining.remove(identifierPattern);
    remaining.remove(operatorPattern);
    if (!remaining.trimmed().isEmpty()) {
        throw runtimeError(nodeId, "Usa solo operatori aritmetici, confronti e operatori booleani");
    }
}

bool outputIsExpression(const QString& source, const QHash<QString, VariableType>& variables) {
    const QString text = source.trimmed();
    if (variables.contains(text) || text == "true" || text == "false") return true;
    if ((text.startsWith('"') && text.endsWith('"')) ||
        (text.startsWith('\'') && text.endsWith('\''))) {
        return true;
    }
    static const QRegularExpression numberPattern(
        QStringLiteral("^(?:\\d+(?:\\.\\d*)?|\\.\\d+)(?:[eE][+-]?\\d+)?$"));
    if (numberPattern.match(text).hasMatch()) return true;
    return text.contains(QRegularExpression(QStringLiteral("[+\\-*/%<>=!&|()?:]")));
}

} // namespace

FlowchartInterpreter::FlowchartInterpreter(Flowchart flowchart) {
    flowchart.validate();
    nodes_ = std::move(flowchart.nodes);

    for (int index = 0; index < static_cast<int>(nodes_.size()); ++index) {
        nodeIndexes_.insert(nodes_[static_cast<std::size_t>(index)].id, index);
        if (nodes_[static_cast<std::size_t>(index)].type == NodeType::Start) {
            currentIndex_ = index;
        }
    }

    for (const Variable& variable : flowchart.variables) {
        if (!validIdentifier(variable.name)) {
            throw jsonError(QString("Invalid variable identifier: %1").arg(variable.name));
        }
        variableTypes_.insert(variable.name, variable.type);
        setVariable(variable.name, defaultValue(variable.type));
    }
}

ExecutionResult FlowchartInterpreter::step(std::optional<QString> input) {
    if (finished_) return {ExecutionEvent::Finished, {}, {}};
    const Node& node = currentNode();

    switch (node.type) {
    case NodeType::Start:
        advance(node.next, node.id);
        return {ExecutionEvent::Advanced, node.id, {}};
    case NodeType::Input: {
        if (!input) return {ExecutionEvent::InputRequired, node.id, node.text};
        const auto variable = variableTypes_.constFind(node.variable);
        if (node.variable.isEmpty() || variable == variableTypes_.cend()) {
            throw runtimeError(node.id, QString("Variabile di input non definita: %1").arg(node.variable));
        }
        setVariable(node.variable, parseInput(*input, variable.value(), node.id));
        advance(node.next, node.id);
        return {ExecutionEvent::Advanced, node.id, {}};
    }
    case NodeType::Output: {
        const QString output = outputIsExpression(node.text, variableTypes_)
                                   ? evaluate(node.text, node.id).toString()
                                   : node.text;
        advance(node.next, node.id);
        return {ExecutionEvent::Output, node.id, output};
    }
    case NodeType::Assignment: {
        static const QRegularExpression assignmentPattern(
            QStringLiteral("^\\s*([A-Za-z_][A-Za-z0-9_]*)\\s*=(?!=)\\s*(.+?)\\s*;?\\s*$"));
        const auto match = assignmentPattern.match(node.text);
        if (!match.hasMatch()) {
            throw runtimeError(node.id, "Assegnazione non valida; formato atteso: variabile = espressione");
        }
        const QString name = match.captured(1);
        const auto variable = variableTypes_.constFind(name);
        if (variable == variableTypes_.cend()) {
            throw runtimeError(node.id, QString("Variabile non dichiarata: %1").arg(name));
        }
        const QJSValue value = evaluate(match.captured(2), node.id);
        setVariable(name, coerceValue(value, variable.value(), node.id));
        advance(node.next, node.id);
        return {ExecutionEvent::Advanced, node.id, {}};
    }
    case NodeType::If:
    case NodeType::While: {
        const QJSValue condition = evaluate(node.text, node.id);
        if (!condition.isBool()) {
            throw runtimeError(node.id, "La condizione deve restituire true o false");
        }
        advance(condition.toBool() ? node.trueNext : node.falseNext, node.id);
        return {ExecutionEvent::Advanced, node.id, {}};
    }
    case NodeType::End:
        finished_ = true;
        currentIndex_ = -1;
        return {ExecutionEvent::Finished, node.id, {}};
    }
    throw runtimeError(node.id, "Tipo di nodo non supportato");
}

QString FlowchartInterpreter::currentNodeId() const {
    return currentIndex_ < 0 ? QString() : nodes_[static_cast<std::size_t>(currentIndex_)].id;
}

QVariantMap FlowchartInterpreter::variables() const {
    QVariantMap result;
    for (auto it = variableTypes_.cbegin(); it != variableTypes_.cend(); ++it) {
        result.insert(it.key(), engine_.globalObject().property(it.key()).toVariant());
    }
    return result;
}

const Node& FlowchartInterpreter::currentNode() const {
    if (currentIndex_ < 0 || currentIndex_ >= static_cast<int>(nodes_.size())) {
        throw std::logic_error("Il diagramma non ha un nodo corrente");
    }
    return nodes_[static_cast<std::size_t>(currentIndex_)];
}

QJSValue FlowchartInterpreter::evaluate(const QString& expression, const QString& nodeId) {
    if (expression.trimmed().isEmpty()) throw runtimeError(nodeId, "Espressione vuota");
    validateExpression(expression, variableTypes_, nodeId);
    const QJSValue value = engine_.evaluate(expression);
    if (value.isError()) throw runtimeError(nodeId, value.toString());
    if (value.isUndefined()) throw runtimeError(nodeId, "L'espressione non produce un valore");
    return value;
}

QVariant FlowchartInterpreter::parseInput(const QString& input, VariableType type,
                                         const QString& nodeId) const {
    bool valid = false;
    switch (type) {
    case VariableType::Integer: {
        const qlonglong value = input.trimmed().toLongLong(&valid);
        if (valid) return QVariant::fromValue(value);
        break;
    }
    case VariableType::Real: {
        const double value = input.trimmed().toDouble(&valid);
        if (valid && std::isfinite(value)) return value;
        break;
    }
    case VariableType::Boolean: {
        const QString value = input.trimmed().toLower();
        if (value == "true" || value == "1") return true;
        if (value == "false" || value == "0") return false;
        break;
    }
    case VariableType::String:
        return input;
    }
    throw runtimeError(nodeId, QString("Valore non valido per il tipo %1")
                                   .arg(variableTypeName(type)));
}

QVariant FlowchartInterpreter::coerceValue(const QJSValue& value, VariableType type,
                                           const QString& nodeId) const {
    switch (type) {
    case VariableType::Integer: {
        const double number = value.toNumber();
        if (!value.isNumber() || !std::isfinite(number) || std::floor(number) != number ||
            number < static_cast<double>(std::numeric_limits<qlonglong>::min()) ||
            number >= 9223372036854775808.0) {
            throw runtimeError(nodeId, "Il valore assegnato deve essere un intero valido");
        }
        return QVariant::fromValue(static_cast<qlonglong>(number));
    }
    case VariableType::Real: {
        const double number = value.toNumber();
        if (!value.isNumber() || !std::isfinite(number)) {
            throw runtimeError(nodeId, "Il valore assegnato deve essere un numero finito");
        }
        return number;
    }
    case VariableType::Boolean:
        if (!value.isBool()) throw runtimeError(nodeId, "Il valore assegnato deve essere booleano");
        return value.toBool();
    case VariableType::String:
        if (!value.isString()) throw runtimeError(nodeId, "Il valore assegnato deve essere una stringa");
        return value.toString();
    }
    throw runtimeError(nodeId, "Tipo di variabile sconosciuto");
}

void FlowchartInterpreter::setVariable(const QString& name, const QVariant& value) {
    engine_.globalObject().setProperty(name, engine_.toScriptValue(value));
}

void FlowchartInterpreter::advance(const QString& target, const QString& fromNode) {
    const auto targetIndex = nodeIndexes_.constFind(target);
    if (target.isEmpty() || targetIndex == nodeIndexes_.cend()) {
        throw runtimeError(fromNode, QString("Successore mancante o inesistente: %1").arg(target));
    }
    currentIndex_ = targetIndex.value();
}

} // namespace flogoritma