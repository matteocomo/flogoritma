#pragma once

#include "flowchart.hpp"
#include "flogoritma/area_disegno.hpp"
#include "flogoritma/interpreter.hpp"

#include <QMainWindow>
#include <QString>

#include <vector>
#include <memory>

class QCloseEvent;
class QPlainTextEdit;
class QTimer;

namespace flogoritma {

class EditorWindow final : public QMainWindow {
public:
    explicit EditorWindow(QWidget* parent = nullptr);
    bool openFile(const QString& path);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void createActions();
    void editProjectProperties();
    void addBlock(TipoBloccoGrafico type);
    void editSelectedBlock();
    void deleteSelectedBlocks();
    void connectBlocks();
    void startExecution();
    void stepExecution();
    void runExecution();
    void stopExecution();
    void executeStep();
    void setExecutionHighlight(const QString& nodeId);
    bool maybeSave();
    bool saveDocument();
    bool saveDocumentAs();
    bool openDocument();
    bool loadDocument(const QString& path);
    void newDocument();
    Flowchart currentFlowchart() const;
    void setFlowchart(const Flowchart& flowchart);
    void updateWindowTitle();
    void markModified();

    AreaDisegno* area_ = nullptr;
    QPlainTextEdit* console_ = nullptr;
    QTimer* runTimer_ = nullptr;
    std::unique_ptr<FlowchartInterpreter> interpreter_;
    ProjectMetadata project_;
    std::vector<Variable> variables_;
    QString currentFile_;
    bool autoConnect_ = true;
    bool loading_ = false;
};

} // namespace flogoritma
