#pragma once

#include <QGraphicsPathItem>
#include <QGraphicsView>
#include <QList>
#include <QSet>
#include <QString>

#include <functional>
#include <utility>

class QGraphicsSceneMouseEvent;
class QWheelEvent;

namespace flogoritma {

class LineaCollegamento;

enum class TipoBloccoGrafico {
    Start,
    Input,
    Output,
    Assegnamento,
    Selezione,
    Ciclo,
    Fine
};

class BloccoGrafico final : public QGraphicsPathItem {
public:
    explicit BloccoGrafico(TipoBloccoGrafico tipo, QString testo,
                           QGraphicsItem* parent = nullptr);
    ~BloccoGrafico() override;

    TipoBloccoGrafico tipo() const noexcept { return tipo_; }
    QString id() const { return id_; }
    void setId(QString id) { id_ = std::move(id); }
    QString testo() const { return testo_; }
    void setTesto(QString testo);
    QString variable() const { return variable_; }
    void setVariable(QString variable);
    void setExecutionActive(bool active);
    void setOnChanged(std::function<void()> callback) { onChanged_ = std::move(callback); }
    BloccoGrafico* successore() const noexcept { return successore_; }
    BloccoGrafico* ramoVero() const noexcept { return ramoVero_; }
    BloccoGrafico* ramoFalso() const noexcept { return ramoFalso_; }

    void setSuccessore(BloccoGrafico* successore);
    void setRami(BloccoGrafico* ramoVero, BloccoGrafico* ramoFalso);

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    friend class LineaCollegamento;

    void aggiungiCollegamento(LineaCollegamento* collegamento);
    void rimuoviCollegamento(LineaCollegamento* collegamento);
    void aggiornaCollegamenti();
    LineaCollegamento* creaCollegamento(BloccoGrafico* destinazione);
    void verificaDestinazione(const BloccoGrafico* destinazione) const;

    TipoBloccoGrafico tipo_;
    QString id_;
    QString testo_;
    QString variable_;
    bool executionActive_ = false;
    std::function<void()> onChanged_;
    QSet<LineaCollegamento*> collegamenti_;
    BloccoGrafico* successore_ = nullptr;
    BloccoGrafico* ramoVero_ = nullptr;
    BloccoGrafico* ramoFalso_ = nullptr;
    LineaCollegamento* collegamentoSuccessore_ = nullptr;
    LineaCollegamento* collegamentoVero_ = nullptr;
    LineaCollegamento* collegamentoFalso_ = nullptr;
};

class AreaDisegno final : public QGraphicsView {
public:
    explicit AreaDisegno(QWidget* parent = nullptr);

    BloccoGrafico* aggiungiBlocco(TipoBloccoGrafico tipo, const QString& testo,
                                  const QPointF& posizione, QString id = {});
    QList<BloccoGrafico*> blocchi() const;
    void svuota();
    void setOnChanged(std::function<void()> callback);
    void collega(BloccoGrafico* origine, BloccoGrafico* destinazione);
    void collegaRami(BloccoGrafico* selezione, BloccoGrafico* ramoVero,
                     BloccoGrafico* ramoFalso);
    BloccoGrafico* collegaAutomaticamente(BloccoGrafico* destinazione,
                                         BloccoGrafico* originePreferita = nullptr);

    void zoomIn();
    void zoomOut();
    void adattaContenuto();

protected:
    void wheelEvent(QWheelEvent* event) override;

private:
    void zoomBy(qreal factor);
    std::function<void()> onChanged_;
};

} // namespace flogoritma