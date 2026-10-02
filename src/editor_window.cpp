#include "flogoritma/editor_window.hpp"

#include <QAction>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QHeaderView>
#include <QHash>
#include <QInputDialog>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QStatusBar>
#include <QTableWidget>
#include <QToolBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <array>
#include <exception>
#include <stdexcept>
#include <utility>

namespace flogoritma {
namespace {

NodeType nodeTypeFor(TipoBloccoGrafico type) {
    switch (type) {
    case TipoBloccoGrafico::Start: return NodeType::Start;
    case TipoBloccoGrafico::Input: return NodeType::Input;
    case TipoBloccoGrafico::Output: return NodeType::Output;
    case TipoBloccoGrafico::Assegnamento: return NodeType::Assignment;
    case TipoBloccoGrafico::Selezione: return NodeType::If;
    case TipoBloccoGrafico::Ciclo: return NodeType::While;
    case TipoBloccoGrafico::Fine: return NodeType::End;
    }
    throw std::logic_error("Tipo di blocco grafico sconosciuto");
}

TipoBloccoGrafico blockTypeFor(NodeType type) {
    switch (type) {
    case NodeType::Start: return TipoBloccoGrafico::Start;
    case NodeType::Input: return TipoBloccoGrafico::Input;
    case NodeType::Output: return TipoBloccoGrafico::Output;
    case NodeType::Assignment: return TipoBloccoGrafico::Assegnamento;
    case NodeType::If: return TipoBloccoGrafico::Selezione;
    case NodeType::While: return TipoBloccoGrafico::Ciclo;
    case NodeType::End: return TipoBloccoGrafico::Fine;
    }
    throw std::logic_error("Tipo di nodo sconosciuto");
}

QString defaultText(TipoBloccoGrafico type) {
    switch (type) {
    case TipoBloccoGrafico::Start: return "Inizio";
    case TipoBloccoGrafico::Input: return "Leggi valore";
    case TipoBloccoGrafico::Output: return "Mostra valore";
    case TipoBloccoGrafico::Assegnamento: return "a = 0";
    case TipoBloccoGrafico::Selezione: return "condizione?";
    case TipoBloccoGrafico::Ciclo: return "condizione ciclo?";
    case TipoBloccoGrafico::Fine: return "Fine";
    }
    return {};
}

QString blockLabel(const BloccoGrafico* block) {
    return QStringLiteral("%1  %2").arg(block->id(), block->testo());
}

BloccoGrafico* blockById(const QList<BloccoGrafico*>& blocks, const QString& id) {
    for (BloccoGrafico* block : blocks) {
        if (block->id() == id) return block;
    }
    return nullptr;
}

} // namespace

EditorWindow::EditorWindow(QWidget* parent) : QMainWindow(parent) {
    setMinimumSize(980, 640);
    resize(1280, 820);

    area_ = new AreaDisegno(this);
    setCentralWidget(area_);
    area_->setOnChanged([this] { markModified(); });
    runTimer_ = new QTimer(this);
    runTimer_->setInterval(250);
    connect(runTimer_, &QTimer::timeout, this, &EditorWindow::executeStep);
    connect(area_->scene(), &QGraphicsScene::selectionChanged, this, [this] {
        const int count = area_->scene()->selectedItems().size();
        statusBar()->showMessage(count == 0
                                     ? tr("Pronto")
                                     : tr("%1 blocchi selezionati").arg(count));
    });

    project_ = {tr("Progetto senza titolo"), qEnvironmentVariable("USER", "Utente"),
                QDateTime::currentDateTimeUtc().toString(Qt::ISODate)};
    createActions();
    addBlock(TipoBloccoGrafico::Start);
    setWindowModified(false);
    updateWindowTitle();
    statusBar()->showMessage(tr("Pronto"));
}

void EditorWindow::createActions() {
    QMenu* fileMenu = menuBar()->addMenu(tr("File"));
    QAction* newAction = fileMenu->addAction(tr("Nuovo"));
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &EditorWindow::newDocument);

    QAction* openAction = fileMenu->addAction(tr("Apri..."));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, [this] { openDocument(); });

    QAction* saveAction = fileMenu->addAction(tr("Salva"));
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, [this] { saveDocument(); });

    QAction* saveAsAction = fileMenu->addAction(tr("Salva con nome..."));
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, [this] { saveDocumentAs(); });
    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction(tr("Esci"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QMenu* editMenu = menuBar()->addMenu(tr("Modifica"));
    QAction* editAction = editMenu->addAction(tr("Modifica blocco..."));
    editAction->setShortcut(Qt::Key_F2);
    connect(editAction, &QAction::triggered, this, &EditorWindow::editSelectedBlock);
    QAction* connectAction = editMenu->addAction(tr("Collega blocchi..."));
    connectAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+L")));
    connect(connectAction, &QAction::triggered, this, &EditorWindow::connectBlocks);
    QAction* deleteAction = editMenu->addAction(tr("Elimina blocchi"));
    deleteAction->setShortcut(QKeySequence::Delete);
    connect(deleteAction, &QAction::triggered, this, &EditorWindow::deleteSelectedBlocks);
    editMenu->addSeparator();
    QAction* autoConnectAction = editMenu->addAction(tr("Collega automaticamente i nuovi blocchi"));
    autoConnectAction->setCheckable(true);
    autoConnectAction->setChecked(autoConnect_);
    connect(autoConnectAction, &QAction::toggled, this, [this](bool enabled) {
        autoConnect_ = enabled;
    });

    QMenu* projectMenu = menuBar()->addMenu(tr("Progetto"));
    QAction* propertiesAction = projectMenu->addAction(tr("Proprietà e variabili..."));
    propertiesAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+P")));
    connect(propertiesAction, &QAction::triggered, this, &EditorWindow::editProjectProperties);

    QMenu* viewMenu = menuBar()->addMenu(tr("Visualizza"));
    QAction* zoomInAction = viewMenu->addAction(tr("Aumenta zoom"));
    zoomInAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+=")));
    connect(zoomInAction, &QAction::triggered, area_, &AreaDisegno::zoomIn);
    QAction* zoomOutAction = viewMenu->addAction(tr("Riduci zoom"));
    zoomOutAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+-")));
    connect(zoomOutAction, &QAction::triggered, area_, &AreaDisegno::zoomOut);
    QAction* fitAction = viewMenu->addAction(tr("Adatta al contenuto"));
    fitAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+0")));
    connect(fitAction, &QAction::triggered, area_, &AreaDisegno::adattaContenuto);

    QMenu* executionMenu = menuBar()->addMenu(tr("Esecuzione"));
    QAction* runAction = executionMenu->addAction(tr("Esegui"));
    runAction->setShortcut(QKeySequence(Qt::Key_F5));
    connect(runAction, &QAction::triggered, this, &EditorWindow::runExecution);
    QAction* stepAction = executionMenu->addAction(tr("Passo singolo"));
    stepAction->setShortcut(QKeySequence(Qt::Key_F6));
    connect(stepAction, &QAction::triggered, this, &EditorWindow::stepExecution);
    QAction* stopAction = executionMenu->addAction(tr("Interrompi"));
    stopAction->setShortcut(QKeySequence(QStringLiteral("Shift+F5")));
    connect(stopAction, &QAction::triggered, this, &EditorWindow::stopExecution);

    QToolBar* toolbar = addToolBar(tr("Strumenti"));
    toolbar->setMovable(false);
    toolbar->addAction(newAction);
    toolbar->addAction(openAction);
    toolbar->addAction(saveAction);
    toolbar->addSeparator();
    toolbar->addAction(editAction);
    toolbar->addAction(connectAction);
    toolbar->addAction(deleteAction);
    toolbar->addSeparator();
    toolbar->addAction(propertiesAction);
    toolbar->addSeparator();
    toolbar->addAction(zoomOutAction);
    toolbar->addAction(zoomInAction);
    toolbar->addAction(fitAction);
    toolbar->addSeparator();
    toolbar->addAction(runAction);
    toolbar->addAction(stepAction);
    toolbar->addAction(stopAction);

    auto* dock = new QDockWidget(tr("Blocchi"), this);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    auto* contents = new QWidget(dock);
    auto* layout = new QVBoxLayout(contents);
    const std::array<std::pair<TipoBloccoGrafico, QString>, 7> palette{{
        {TipoBloccoGrafico::Start, tr("Inizio")},
        {TipoBloccoGrafico::Input, tr("Input")},
        {TipoBloccoGrafico::Output, tr("Output")},
        {TipoBloccoGrafico::Assegnamento, tr("Assegnazione")},
        {TipoBloccoGrafico::Selezione, tr("Selezione (if)")},
        {TipoBloccoGrafico::Ciclo, tr("Ciclo (while)")},
        {TipoBloccoGrafico::Fine, tr("Fine")},
    }};
    for (const auto& entry : palette) {
        auto* button = new QPushButton(entry.second, contents);
        connect(button, &QPushButton::clicked, this,
                [this, type = entry.first] { addBlock(type); });
        layout->addWidget(button);
    }
    layout->addStretch();
    dock->setWidget(contents);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    auto* consoleDock = new QDockWidget(tr("Console di esecuzione"), this);
    console_ = new QPlainTextEdit(consoleDock);
    console_->setReadOnly(true);
    console_->setMaximumBlockCount(2000);
    consoleDock->setWidget(console_);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);
    viewMenu->addAction(consoleDock->toggleViewAction());
}

void EditorWindow::editProjectProperties() {
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Proprietà del progetto"));
    dialog.resize(480, 420);

    auto* layout = new QVBoxLayout(&dialog);
    auto* form = new QFormLayout;
    auto* name = new QLineEdit(project_.name, &dialog);
    auto* author = new QLineEdit(project_.author, &dialog);
    form->addRow(tr("Nome progetto"), name);
    form->addRow(tr("Autore"), author);
    layout->addLayout(form);

    auto* table = new QTableWidget(&dialog);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({tr("Nome variabile"), tr("Tipo")});
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(table);

    const auto addVariableRow = [table](const Variable& variable) {
        const int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(variable.name));
        auto* type = new QComboBox(table);
        type->addItem(QStringLiteral("integer"), static_cast<int>(VariableType::Integer));
        type->addItem(QStringLiteral("real"), static_cast<int>(VariableType::Real));
        type->addItem(QStringLiteral("boolean"), static_cast<int>(VariableType::Boolean));
        type->addItem(QStringLiteral("string"), static_cast<int>(VariableType::String));
        type->setCurrentIndex(type->findData(static_cast<int>(variable.type)));
        table->setCellWidget(row, 1, type);
    };
    for (const Variable& variable : variables_) addVariableRow(variable);

    auto* variableButtons = new QWidget(&dialog);
    auto* variableButtonLayout = new QHBoxLayout(variableButtons);
    variableButtonLayout->setContentsMargins(0, 0, 0, 0);
    auto* addButton = new QPushButton(tr("Aggiungi variabile"), variableButtons);
    auto* removeButton = new QPushButton(tr("Rimuovi"), variableButtons);
    variableButtonLayout->addWidget(addButton);
    variableButtonLayout->addWidget(removeButton);
    variableButtonLayout->addStretch();
    layout->addWidget(variableButtons);
    connect(addButton, &QPushButton::clicked, &dialog, [&, table, addVariableRow] {
        addVariableRow({QStringLiteral("variabile_%1").arg(table->rowCount() + 1),
                        VariableType::Integer});
        table->selectRow(table->rowCount() - 1);
        table->editItem(table->item(table->rowCount() - 1, 0));
    });
    connect(removeButton, &QPushButton::clicked, &dialog, [table] {
        if (table->currentRow() >= 0) table->removeRow(table->currentRow());
    });

    ProjectMetadata editedProject = project_;
    std::vector<Variable> editedVariables = variables_;
    auto* dialogButtons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    layout->addWidget(dialogButtons);
    connect(dialogButtons, &QDialogButtonBox::accepted, &dialog, [&] {
        try {
            editedProject.name = name->text().trimmed();
            editedProject.author = author->text().trimmed();
            editedVariables.clear();
            for (int row = 0; row < table->rowCount(); ++row) {
                const auto* type = qobject_cast<QComboBox*>(table->cellWidget(row, 1));
                editedVariables.push_back({table->item(row, 0)->text().trimmed(),
                    static_cast<VariableType>(type->currentData().toInt())});
            }
            Flowchart candidate = currentFlowchart();
            candidate.project = editedProject;
            candidate.variables = editedVariables;
            candidate.validate();
            dialog.accept();
        } catch (const std::exception& error) {
            QMessageBox::warning(&dialog, tr("Proprietà non valide"),
                                 QString::fromUtf8(error.what()));
        }
    });
    connect(dialogButtons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) return;
    project_ = std::move(editedProject);
    variables_ = std::move(editedVariables);
    markModified();
    updateWindowTitle();
}

void EditorWindow::addBlock(TipoBloccoGrafico type) {
    if (type == TipoBloccoGrafico::Start) {
        for (BloccoGrafico* block : area_->blocchi()) {
            if (block->tipo() == TipoBloccoGrafico::Start) {
                statusBar()->showMessage(tr("Il diagramma puo avere un solo blocco iniziale"), 4000);
                return;
            }
        }
    }

    BloccoGrafico* preferredSource = nullptr;
    const QList<QGraphicsItem*> selectedItems = area_->scene()->selectedItems();
    if (selectedItems.size() == 1) {
        preferredSource = dynamic_cast<BloccoGrafico*>(selectedItems.first());
    }

    const int index = area_->blocchi().size();
    const QPoint center = area_->viewport()->rect().center();
    const QPointF position = area_->mapToScene(center) + QPointF(28.0 * (index % 6),
                                                                 28.0 * (index % 6));
    BloccoGrafico* block = area_->aggiungiBlocco(type, defaultText(type), position);
    if (type == TipoBloccoGrafico::Input && !variables_.empty()) {
        block->setVariable(variables_.front().name);
    }
    BloccoGrafico* source = nullptr;
    if (autoConnect_) {
        source = area_->collegaAutomaticamente(block, preferredSource);
    }
    if (source) {
        const bool falseBranch =
            (source->tipo() == TipoBloccoGrafico::Selezione ||
             source->tipo() == TipoBloccoGrafico::Ciclo) && source->ramoFalso() == block;
        block->setPos(source->pos() + (falseBranch ? QPointF(240.0, 0.0)
                                                   : QPointF(0.0, 130.0)));
        statusBar()->showMessage(tr("Collegato automaticamente da %1").arg(source->testo()), 3000);
    }
    block->setSelected(true);
    markModified();
}

void EditorWindow::editSelectedBlock() {
    const QList<QGraphicsItem*> selected = area_->scene()->selectedItems();
    if (selected.size() != 1) {
        statusBar()->showMessage(tr("Seleziona un solo blocco da modificare"), 4000);
        return;
    }
    auto* block = dynamic_cast<BloccoGrafico*>(selected.first());
    if (!block) return;

    if (block->tipo() == TipoBloccoGrafico::Input) {
        QDialog dialog(this);
        dialog.setWindowTitle(tr("Configura input"));
        QFormLayout form(&dialog);
        auto* prompt = new QLineEdit(block->testo(), &dialog);
        auto* variable = new QComboBox(&dialog);
        variable->setEditable(true);
        for (const Variable& declared : variables_) variable->addItem(declared.name);
        variable->setCurrentText(block->variable());
        form.addRow(tr("Prompt"), prompt);
        form.addRow(tr("Memorizza in"), variable);
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                             Qt::Horizontal, &dialog);
        form.addRow(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        if (dialog.exec() == QDialog::Accepted) {
            block->setTesto(prompt->text());
            block->setVariable(variable->currentText().trimmed());
        }
        return;
    }

    bool accepted = false;
    const QString text = QInputDialog::getMultiLineText(
        this, tr("Modifica blocco"), tr("Testo o espressione:"), block->testo(), &accepted);
    if (accepted) block->setTesto(text);
}

void EditorWindow::deleteSelectedBlocks() {
    const QList<QGraphicsItem*> selected = area_->scene()->selectedItems();
    if (selected.isEmpty()) return;
    int startsSelected = 0;
    int startsInDocument = 0;
    for (QGraphicsItem* item : selected) {
        if (auto* block = dynamic_cast<BloccoGrafico*>(item)) {
            if (block->tipo() == TipoBloccoGrafico::Start) ++startsSelected;
        }
    }
    for (BloccoGrafico* block : area_->blocchi()) {
        if (block->tipo() == TipoBloccoGrafico::Start) ++startsInDocument;
    }
    if (startsSelected == startsInDocument) {
        QMessageBox::information(this, tr("Blocco iniziale richiesto"),
                                 tr("Un diagramma deve mantenere un blocco iniziale."));
        return;
    }
    for (QGraphicsItem* item : selected) {
        if (auto* block = dynamic_cast<BloccoGrafico*>(item)) {
            area_->scene()->removeItem(block);
            delete block;
        }
    }
    markModified();
}

void EditorWindow::connectBlocks() {
    const QList<BloccoGrafico*> blocks = area_->blocchi();
    if (blocks.size() < 2) {
        statusBar()->showMessage(tr("Aggiungi almeno due blocchi per creare un collegamento"), 4000);
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Collega blocchi"));
    QFormLayout form(&dialog);
    auto* source = new QComboBox(&dialog);
    auto* next = new QComboBox(&dialog);
    auto* trueTarget = new QComboBox(&dialog);
    auto* falseTarget = new QComboBox(&dialog);
    next->addItem(tr("Non collegato"), QString());
    trueTarget->addItem(tr("Non collegato"), QString());
    falseTarget->addItem(tr("Non collegato"), QString());

    for (BloccoGrafico* block : blocks) {
        if (block->tipo() != TipoBloccoGrafico::Fine) {
            source->addItem(blockLabel(block), block->id());
            source->setItemData(source->count() - 1, static_cast<int>(block->tipo()), Qt::UserRole + 1);
        }
        next->addItem(blockLabel(block), block->id());
        trueTarget->addItem(blockLabel(block), block->id());
        falseTarget->addItem(blockLabel(block), block->id());
    }
    form.addRow(tr("Origine"), source);
    form.addRow(tr("Successore"), next);
    form.addRow(tr("Ramo vero"), trueTarget);
    form.addRow(tr("Ramo falso"), falseTarget);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                         Qt::Horizontal, &dialog);
    form.addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    const auto updateRows = [source, next, trueTarget, falseTarget, &form] {
        const auto type = static_cast<TipoBloccoGrafico>(
            source->currentData(Qt::UserRole + 1).toInt());
        const bool isBranch = type == TipoBloccoGrafico::Selezione ||
                              type == TipoBloccoGrafico::Ciclo;
        form.labelForField(next)->setVisible(!isBranch);
        next->setVisible(!isBranch);
        form.labelForField(trueTarget)->setVisible(isBranch);
        trueTarget->setVisible(isBranch);
        form.labelForField(falseTarget)->setVisible(isBranch);
        falseTarget->setVisible(isBranch);
    };
    connect(source, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog,
            [updateRows](int) { updateRows(); });
    updateRows();

    if (dialog.exec() != QDialog::Accepted) return;
    BloccoGrafico* origin = blockById(blocks, source->currentData().toString());
    if (!origin) return;
    try {
        if (origin->tipo() == TipoBloccoGrafico::Selezione ||
            origin->tipo() == TipoBloccoGrafico::Ciclo) {
            area_->collegaRami(origin,
                               blockById(blocks, trueTarget->currentData().toString()),
                               blockById(blocks, falseTarget->currentData().toString()));
        } else {
            area_->collega(origin, blockById(blocks, next->currentData().toString()));
        }
    } catch (const std::exception& error) {
        QMessageBox::warning(this, tr("Collegamento non riuscito"), QString::fromUtf8(error.what()));
    }
}

void EditorWindow::startExecution() {
    runTimer_->stop();
    try {
        interpreter_ = std::make_unique<FlowchartInterpreter>(currentFlowchart());
        console_->clear();
        setExecutionHighlight({});
        statusBar()->showMessage(tr("Esecuzione avviata"));
    } catch (const std::exception& error) {
        interpreter_.reset();
        console_->appendPlainText(tr("Errore: %1").arg(QString::fromUtf8(error.what())));
        statusBar()->showMessage(tr("Impossibile avviare il diagramma"), 5000);
    }
}

void EditorWindow::stepExecution() {
    if (!interpreter_ || interpreter_->finished()) startExecution();
    if (interpreter_) executeStep();
}

void EditorWindow::runExecution() {
    if (!interpreter_ || interpreter_->finished()) startExecution();
    if (interpreter_) runTimer_->start();
}

void EditorWindow::stopExecution() {
    runTimer_->stop();
    interpreter_.reset();
    setExecutionHighlight({});
    statusBar()->showMessage(tr("Esecuzione interrotta"), 3000);
}

void EditorWindow::executeStep() {
    if (!interpreter_) return;
    try {
        ExecutionResult result = interpreter_->step();
        if (result.event == ExecutionEvent::InputRequired) {
            setExecutionHighlight(result.nodeId);
            bool accepted = false;
            const QString prompt = result.text.isEmpty() ? tr("Inserisci un valore") : result.text;
            const QString input = QInputDialog::getText(
                this, tr("Input richiesto"), prompt, QLineEdit::Normal, {}, &accepted);
            if (!accepted) {
                runTimer_->stop();
                statusBar()->showMessage(tr("Esecuzione in pausa sul nodo di input"));
                return;
            }
            result = interpreter_->step(input);
        }

        setExecutionHighlight(result.nodeId);
        if (result.event == ExecutionEvent::Output) console_->appendPlainText(result.text);
        if (result.event == ExecutionEvent::Finished) {
            runTimer_->stop();
            console_->appendPlainText(tr("Esecuzione completata."));
        }

        QStringList values;
        const QVariantMap variables = interpreter_->variables();
        for (auto it = variables.cbegin(); it != variables.cend(); ++it) {
            values.append(QStringLiteral("%1 = %2").arg(it.key(), it.value().toString()));
        }
        statusBar()->showMessage(values.isEmpty() ? tr("Nodo: %1").arg(result.nodeId)
                                                   : values.join(QStringLiteral("  |  ")));
    } catch (const std::exception& error) {
        runTimer_->stop();
        console_->appendPlainText(tr("Errore: %1").arg(QString::fromUtf8(error.what())));
        statusBar()->showMessage(tr("Esecuzione interrotta per errore"), 5000);
    }
}

void EditorWindow::setExecutionHighlight(const QString& nodeId) {
    for (BloccoGrafico* block : area_->blocchi()) {
        block->setExecutionActive(!nodeId.isEmpty() && block->id() == nodeId);
    }
}

bool EditorWindow::maybeSave() {
    if (!isWindowModified()) return true;
    const auto result = QMessageBox::warning(
        this, tr("Modifiche non salvate"), tr("Salvare le modifiche prima di continuare?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (result == QMessageBox::Save) return saveDocument();
    return result == QMessageBox::Discard;
}

bool EditorWindow::saveDocument() {
    if (currentFile_.isEmpty()) return saveDocumentAs();
    try {
        const QByteArray bytes = currentFlowchart().toJsonBytes();
        QSaveFile file(currentFile_);
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
            QMessageBox::critical(this, tr("Salvataggio non riuscito"), file.errorString());
            return false;
        }
        setWindowModified(false);
        updateWindowTitle();
        statusBar()->showMessage(tr("Salvato: %1").arg(QFileInfo(currentFile_).fileName()), 4000);
        return true;
    } catch (const std::exception& error) {
        QMessageBox::warning(this, tr("Diagramma non valido"), QString::fromUtf8(error.what()));
        return false;
    }
}

bool EditorWindow::saveDocumentAs() {
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Salva diagramma"), currentFile_.isEmpty() ? QStringLiteral("diagramma.flog") : currentFile_,
        tr("Progetti Flogoritma (*.flog)"));
    if (path.isEmpty()) return false;
    const QString previousFile = currentFile_;
    currentFile_ = path.endsWith(".flog", Qt::CaseInsensitive) ? path : path + ".flog";
    if (!saveDocument()) {
        currentFile_ = previousFile;
        updateWindowTitle();
        return false;
    }
    return true;
}

bool EditorWindow::openDocument() {
    if (!maybeSave()) return false;
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Apri diagramma"), currentFile_, tr("Progetti Flogoritma (*.flog);;Tutti i file (*)"));
    return path.isEmpty() ? false : loadDocument(path);
}

bool EditorWindow::openFile(const QString& path) {
    return loadDocument(path);
}

bool EditorWindow::loadDocument(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Apertura non riuscita"), file.errorString());
        return false;
    }
    try {
        const Flowchart flowchart = Flowchart::fromJson(file.readAll());
        setFlowchart(flowchart);
        currentFile_ = path;
        setWindowModified(false);
        updateWindowTitle();
        statusBar()->showMessage(tr("Aperto: %1").arg(QFileInfo(path).fileName()), 4000);
        return true;
    } catch (const std::exception& error) {
        QMessageBox::warning(this, tr("File .flog non valido"), QString::fromUtf8(error.what()));
        return false;
    }
}

void EditorWindow::newDocument() {
    if (!maybeSave()) return;
    loading_ = true;
    area_->svuota();
    project_ = {tr("Progetto senza titolo"), qEnvironmentVariable("USER", "Utente"),
                QDateTime::currentDateTimeUtc().toString(Qt::ISODate)};
    variables_.clear();
    currentFile_.clear();
    addBlock(TipoBloccoGrafico::Start);
    loading_ = false;
    setWindowModified(false);
    updateWindowTitle();
}

Flowchart EditorWindow::currentFlowchart() const {
    Flowchart flowchart;
    flowchart.project = project_;
    flowchart.variables = variables_;
    for (BloccoGrafico* block : area_->blocchi()) {
        Node node{block->id(), nodeTypeFor(block->tipo()), block->testo(), {}, {}, {},
                  block->pos().x(), block->pos().y(), block->variable()};
        if (block->tipo() == TipoBloccoGrafico::Selezione ||
            block->tipo() == TipoBloccoGrafico::Ciclo) {
            if (block->ramoVero()) node.trueNext = block->ramoVero()->id();
            if (block->ramoFalso()) node.falseNext = block->ramoFalso()->id();
        } else if (block->tipo() != TipoBloccoGrafico::Fine && block->successore()) {
            node.next = block->successore()->id();
        }
        flowchart.nodes.push_back(std::move(node));
    }
    return flowchart;
}

void EditorWindow::setFlowchart(const Flowchart& flowchart) {
    flowchart.validate();
    loading_ = true;
    area_->svuota();
    project_ = flowchart.project;
    variables_ = flowchart.variables;
    QHash<QString, BloccoGrafico*> blocks;
    for (const Node& node : flowchart.nodes) {
        BloccoGrafico* block = area_->aggiungiBlocco(
            blockTypeFor(node.type), node.text, QPointF(node.x, node.y), node.id);
        block->setVariable(node.variable);
        blocks.insert(node.id, block);
    }
    for (const Node& node : flowchart.nodes) {
        BloccoGrafico* block = blocks.value(node.id);
        if (node.type == NodeType::If || node.type == NodeType::While) {
            block->setRami(blocks.value(node.trueNext), blocks.value(node.falseNext));
        } else if (node.type != NodeType::End) {
            block->setSuccessore(blocks.value(node.next));
        }
    }
    loading_ = false;
}

void EditorWindow::updateWindowTitle() {
    const QString documentName = currentFile_.isEmpty()
                                     ? project_.name
                                     : QFileInfo(currentFile_).fileName();
    setWindowTitle(QStringLiteral("%1[*] - Flogoritma").arg(documentName));
}

void EditorWindow::markModified() {
    if (!loading_) setWindowModified(true);
}

void EditorWindow::closeEvent(QCloseEvent* event) {
    if (maybeSave()) event->accept();
    else event->ignore();
}

} // namespace flogoritma
