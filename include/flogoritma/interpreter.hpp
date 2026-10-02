#pragma once

#include "flowchart.hpp"

#include <QHash>
#include <QJSEngine>
#include <QString>
#include <QVariantMap>

#include <optional>
#include <vector>

namespace flogoritma {

enum class ExecutionEvent { Advanced, InputRequired, Output, Finished };

struct ExecutionResult {
    ExecutionEvent event;
    QString nodeId;
    QString text;
};

class FlowchartInterpreter final {
public:
    explicit FlowchartInterpreter(Flowchart flowchart);

    ExecutionResult step(std::optional<QString> input = std::nullopt);
    bool finished() const noexcept { return finished_; }
    QString currentNodeId() const;
    QVariantMap variables() const;

private:
    const Node& currentNode() const;
    QJSValue evaluate(const QString& expression, const QString& nodeId);
    QVariant parseInput(const QString& input, VariableType type, const QString& nodeId) const;
    QVariant coerceValue(const QJSValue& value, VariableType type, const QString& nodeId) const;
    void setVariable(const QString& name, const QVariant& value);
    void advance(const QString& target, const QString& fromNode);

    std::vector<Node> nodes_;
    QHash<QString, int> nodeIndexes_;
    QHash<QString, VariableType> variableTypes_;
    QJSEngine engine_;
    int currentIndex_ = -1;
    bool finished_ = false;
};

} // namespace flogoritma