#include "flogoritma/area_disegno.hpp"

#include <QBrush>
#include <QColor>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QLineF>
#include <QPainter>
#include <QPen>
#include <QStyleOptionGraphicsItem>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace flogoritma {
namespace {

constexpr qreal LarghezzaBlocco = 160.0;
constexpr qreal AltezzaBlocco = 80.0;

QPainterPath formaPer(TipoBloccoGrafico tipo) {
    const QRectF rettangolo(0.0, 0.0, LarghezzaBlocco, AltezzaBlocco);
    QPainterPath forma;

    switch (tipo) {
    case TipoBloccoGrafico::Input:
    case TipoBloccoGrafico::Output:
        forma.moveTo(28.0, 0.0);
        forma.lineTo(LarghezzaBlocco, 0.0);
        forma.lineTo(LarghezzaBlocco - 28.0, AltezzaBlocco);
        forma.lineTo(0.0, AltezzaBlocco);
        forma.closeSubpath();
        break;
    case TipoBloccoGrafico::Selezione:
    case TipoBloccoGrafico::Ciclo:
        forma.moveTo(LarghezzaBlocco / 2.0, 0.0);
        forma.lineTo(LarghezzaBlocco, AltezzaBlocco / 2.0);
        forma.lineTo(LarghezzaBlocco / 2.0, AltezzaBlocco);
        forma.lineTo(0.0, AltezzaBlocco / 2.0);
        forma.closeSubpath();
        break;
    case TipoBloccoGrafico::Start:
    case TipoBloccoGrafico::Fine:
        forma.addRoundedRect(rettangolo, 24.0, 24.0);
        break;
    case TipoBloccoGrafico::Assegnamento:
        forma.addRect(rettangolo);
        break;
    }
    return forma;
}

QColor colorePer(TipoBloccoGrafico tipo) {
    switch (tipo) {
    case TipoBloccoGrafico::Start: return QColor("#d8f3dc");
    case TipoBloccoGrafico::Input:
    case TipoBloccoGrafico::Output: return QColor("#dbeafe");
    case TipoBloccoGrafico::Selezione: return QColor("#fff1c1");
    case TipoBloccoGrafico::Ciclo: return QColor("#fce7c8");
    case TipoBloccoGrafico::Fine: return QColor("#ffe0e0");
    case TipoBloccoGrafico::Assegnamento: return QColor("#f1f5f9");
    }
    return Qt::white;
}

QPointF puntoSulBordo(const BloccoGrafico* blocco, const QPointF& verso) {
    const QRectF r = blocco->boundingRect();
    const QPointF centro = blocco->mapToScene(r.center());
    const QPointF direzione = verso - centro;
    const qreal dx = std::abs(direzione.x());
    const qreal dy = std::abs(direzione.y());
    const qreal semiLarghezza = r.width() / 2.0;
    const qreal semiAltezza = r.height() / 2.0;

    if (dx < 0.001 && dy < 0.001) {
        return centro + QPointF(0.0, semiAltezza);
    }

    qreal fattore = 0.0;
    if (blocco->tipo() == TipoBloccoGrafico::Selezione ||
        blocco->tipo() == TipoBloccoGrafico::Ciclo) {
        // Il rombo raggiunge il bordo quando |x|/w + |y|/h = 1.
        fattore = 1.0 / (dx / semiLarghezza + dy / semiAltezza);
    } else {
        const qreal scalaX = dx > 0.001 ? semiLarghezza / dx : 1.0e9;
        const qreal scalaY = dy > 0.001 ? semiAltezza / dy : 1.0e9;
        fattore = std::min(scalaX, scalaY);
    }
    return centro + direzione * fattore;
}

} // namespace

class LineaCollegamento final : public QGraphicsPathItem {
public:
    LineaCollegamento(BloccoGrafico* origine, BloccoGrafico* destinazione)
        : origine_(origine), destinazione_(destinazione) {
        setZValue(-1.0);
        setPen(QPen(QColor("#334155"), 2.0));
        setBrush(QBrush(QColor("#334155")));
        origine_->aggiungiCollegamento(this);
        destinazione_->aggiungiCollegamento(this);
        aggiornaPosizione();
    }

    ~LineaCollegamento() override {
        origine_->rimuoviCollegamento(this);
        if (destinazione_ != origine_) {
            destinazione_->rimuoviCollegamento(this);
        }
    }

    void aggiornaPosizione() {
        const QPointF centroOrigine = origine_->mapToScene(origine_->boundingRect().center());
        const QPointF centroDestinazione = destinazione_->mapToScene(destinazione_->boundingRect().center());
        QPainterPath percorso;
        QPointF punta;
        QPointF direzione;

        if (origine_ == destinazione_) {
            const QRectF r = origine_->mapToScene(origine_->boundingRect()).boundingRect();
            const QPointF inizio(r.right(), r.center().y() - 12.0);
            const QPointF fine(r.right(), r.center().y() + 12.0);
            const QPointF controlloUno(r.right() + 54.0, r.top() - 8.0);
            const QPointF controlloDue(r.right() + 54.0, r.bottom() + 8.0);
            percorso.moveTo(inizio);
            percorso.cubicTo(controlloUno, controlloDue, fine);
            punta = fine;
            direzione = fine - controlloDue;
        } else {
            const QPointF inizio = puntoSulBordo(origine_, centroDestinazione);
            const QPointF fine = puntoSulBordo(destinazione_, centroOrigine);
            percorso.moveTo(inizio);
            percorso.lineTo(fine);
            punta = fine;
            direzione = fine - inizio;
        }

        // Costruisce la punta della freccia nella direzione di arrivo.
        const qreal lunghezza = std::hypot(direzione.x(), direzione.y());
        if (lunghezza > 0.001) {
            const QPointF unita = direzione / lunghezza;
            const QPointF perpendicolare(-unita.y(), unita.x());
            const QPointF base = punta - unita * 11.0;
            percorso.moveTo(punta);
            percorso.lineTo(base + perpendicolare * 4.5);
            percorso.lineTo(base - perpendicolare * 4.5);
            percorso.closeSubpath();
        }
        setPath(percorso);
    }

private:
    BloccoGrafico* origine_;
    BloccoGrafico* destinazione_;
};

BloccoGrafico::BloccoGrafico(TipoBloccoGrafico tipo, QString testo, QGraphicsItem* parent)
    : QGraphicsPathItem(parent), tipo_(tipo), testo_(std::move(testo)) {
    setPath(formaPer(tipo_));
    setPen(QPen(QColor("#1e293b"), 2.0));
    setBrush(QBrush(colorePer(tipo_)));
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
    setCursor(Qt::OpenHandCursor);
}

BloccoGrafico::~BloccoGrafico() {
    while (!collegamenti_.isEmpty()) {
        delete *collegamenti_.begin();
    }
}

void BloccoGrafico::setTesto(QString testo) {
    testo_ = std::move(testo);
    update();
    if (onChanged_) onChanged_();
}

void BloccoGrafico::setVariable(QString variable) {
    variable_ = std::move(variable);
    if (onChanged_) onChanged_();
}

void BloccoGrafico::setExecutionActive(bool active) {
    if (executionActive_ == active) return;
    executionActive_ = active;
    setPen(QPen(executionActive_ ? QColor("#ea580c") : QColor("#1e293b"),
                 executionActive_ ? 4.0 : 2.0));
    update();
}

void BloccoGrafico::setSuccessore(BloccoGrafico* successore) {
    verificaDestinazione(successore);
    if (collegamentoSuccessore_) {
        delete collegamentoSuccessore_;
    }
    successore_ = successore;
    collegamentoSuccessore_ = successore ? creaCollegamento(successore) : nullptr;
    if (onChanged_) onChanged_();
}

void BloccoGrafico::setRami(BloccoGrafico* ramoVero, BloccoGrafico* ramoFalso) {
    if (tipo_ != TipoBloccoGrafico::Selezione && tipo_ != TipoBloccoGrafico::Ciclo) {
        throw std::logic_error("Solo un blocco condizionale puo avere due rami");
    }
    verificaDestinazione(ramoVero);
    verificaDestinazione(ramoFalso);
    if (collegamentoVero_) delete collegamentoVero_;
    if (collegamentoFalso_) delete collegamentoFalso_;
    collegamentoVero_ = ramoVero ? creaCollegamento(ramoVero) : nullptr;
    collegamentoFalso_ = ramoFalso ? creaCollegamento(ramoFalso) : nullptr;
    ramoVero_ = ramoVero;
    ramoFalso_ = ramoFalso;
    if (onChanged_) onChanged_();
}

void BloccoGrafico::paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
                          QWidget* widget) {
    QGraphicsPathItem::paint(painter, option, widget);
    painter->save();
    painter->setPen(QColor("#0f172a"));
    painter->drawText(boundingRect().adjusted(12.0, 8.0, -12.0, -8.0),
                      Qt::AlignCenter | Qt::TextWordWrap, testo_);
    painter->restore();
}

QVariant BloccoGrafico::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionChange && scene()) {
        QPointF posizione = value.toPointF();
        const QRectF area = scene()->sceneRect();
        const QRectF limiti = boundingRect();
        posizione.setX(std::clamp(posizione.x(), area.left() - limiti.left(),
                                  area.right() - limiti.right()));
        posizione.setY(std::clamp(posizione.y(), area.top() - limiti.top(),
                                  area.bottom() - limiti.bottom()));
        return posizione;
    }

    const QVariant result = QGraphicsPathItem::itemChange(change, value);
    if (change == ItemPositionHasChanged) {
        aggiornaCollegamenti();
        if (onChanged_) onChanged_();
    }
    return result;
}

void BloccoGrafico::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    setCursor(Qt::ClosedHandCursor);
    QGraphicsPathItem::mousePressEvent(event);
}

void BloccoGrafico::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    // ItemIsMovable aggiorna la posizione; itemChange ridisegna gli archi.
    QGraphicsPathItem::mouseMoveEvent(event);
    aggiornaCollegamenti();
}

void BloccoGrafico::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsPathItem::mouseReleaseEvent(event);
    setCursor(Qt::OpenHandCursor);
}

void BloccoGrafico::aggiungiCollegamento(LineaCollegamento* collegamento) {
    collegamenti_.insert(collegamento);
}

void BloccoGrafico::rimuoviCollegamento(LineaCollegamento* collegamento) {
    collegamenti_.remove(collegamento);
    if (collegamentoSuccessore_ == collegamento) {
        collegamentoSuccessore_ = nullptr;
        successore_ = nullptr;
    }
    if (collegamentoVero_ == collegamento) {
        collegamentoVero_ = nullptr;
        ramoVero_ = nullptr;
    }
    if (collegamentoFalso_ == collegamento) {
        collegamentoFalso_ = nullptr;
        ramoFalso_ = nullptr;
    }
}

void BloccoGrafico::aggiornaCollegamenti() {
    for (LineaCollegamento* collegamento : collegamenti_) {
        collegamento->aggiornaPosizione();
    }
}

LineaCollegamento* BloccoGrafico::creaCollegamento(BloccoGrafico* destinazione) {
    auto* collegamento = new LineaCollegamento(this, destinazione);
    scene()->addItem(collegamento);
    return collegamento;
}

void BloccoGrafico::verificaDestinazione(const BloccoGrafico* destinazione) const {
    if (destinazione && (!scene() || destinazione->scene() != scene())) {
        throw std::invalid_argument("I blocchi collegati devono appartenere alla stessa scena");
    }
}

AreaDisegno::AreaDisegno(QWidget* parent) : QGraphicsView(parent) {
    auto* scena = new QGraphicsScene(this);
    scena->setSceneRect(0.0, 0.0, 1400.0, 900.0);
    setScene(scena);
    setRenderHint(QPainter::Antialiasing, true);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setBackgroundBrush(QColor("#f8fafc"));
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setMinimumSize(800, 600);
}

BloccoGrafico* AreaDisegno::aggiungiBlocco(TipoBloccoGrafico tipo, const QString& testo,
                                          const QPointF& posizione, QString id) {
    auto* blocco = new BloccoGrafico(tipo, testo);
    if (id.isEmpty()) {
        quint64 candidate = 1;
        bool collision;
        do {
            collision = false;
            id = QStringLiteral("n%1").arg(candidate++);
            for (BloccoGrafico* existing : blocchi()) {
                if (existing->id() == id) {
                    collision = true;
                    break;
                }
            }
        } while (collision);
    }
    blocco->setId(std::move(id));
    blocco->setOnChanged(onChanged_);
    scene()->addItem(blocco);
    blocco->setPos(posizione);
    return blocco;
}

QList<BloccoGrafico*> AreaDisegno::blocchi() const {
    QList<BloccoGrafico*> result;
    for (QGraphicsItem* item : scene()->items()) {
        if (auto* block = dynamic_cast<BloccoGrafico*>(item)) {
            result.append(block);
        }
    }
    return result;
}

void AreaDisegno::svuota() {
    scene()->clear();
}

void AreaDisegno::setOnChanged(std::function<void()> callback) {
    onChanged_ = std::move(callback);
    for (BloccoGrafico* block : blocchi()) {
        block->setOnChanged(onChanged_);
    }
}

void AreaDisegno::zoomIn() {
    zoomBy(1.2);
}

void AreaDisegno::zoomOut() {
    zoomBy(1.0 / 1.2);
}

void AreaDisegno::adattaContenuto() {
    const QRectF bounds = scene()->itemsBoundingRect().adjusted(-80.0, -80.0, 80.0, 80.0);
    if (bounds.isEmpty()) {
        resetTransform();
        return;
    }
    fitInView(bounds, Qt::KeepAspectRatio);
}

void AreaDisegno::zoomBy(qreal factor) {
    const qreal currentScale = transform().m11();
    const qreal targetScale = std::clamp(currentScale * factor, 0.25, 2.5);
    const qreal scaleFactor = targetScale / currentScale;
    scale(scaleFactor, scaleFactor);
}

void AreaDisegno::wheelEvent(QWheelEvent* event) {
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        const qreal factor = std::pow(1.0015, event->angleDelta().y());
        zoomBy(factor);
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent(event);
}

void AreaDisegno::collega(BloccoGrafico* origine, BloccoGrafico* destinazione) {
    if (!origine) throw std::invalid_argument("Il blocco origine non puo essere nullo");
    origine->setSuccessore(destinazione);
}

void AreaDisegno::collegaRami(BloccoGrafico* selezione, BloccoGrafico* ramoVero,
                              BloccoGrafico* ramoFalso) {
    if (!selezione) throw std::invalid_argument("Il blocco di selezione non puo essere nullo");
    selezione->setRami(ramoVero, ramoFalso);
}

BloccoGrafico* AreaDisegno::collegaAutomaticamente(BloccoGrafico* destinazione,
                                                   BloccoGrafico* originePreferita) {
    if (!destinazione || destinazione->scene() != scene() ||
        destinazione->tipo() == TipoBloccoGrafico::Start) {
        return nullptr;
    }

    const auto haUscitaLibera = [](const BloccoGrafico* block) {
        if (block->tipo() == TipoBloccoGrafico::Fine) return false;
        if (block->tipo() == TipoBloccoGrafico::Selezione ||
            block->tipo() == TipoBloccoGrafico::Ciclo) {
            return !block->ramoVero() || !block->ramoFalso();
        }
        return !block->successore();
    };

    BloccoGrafico* source = originePreferita;
    if (source) {
        if (source == destinazione || source->scene() != scene() || !haUscitaLibera(source)) {
            return nullptr;
        }
    } else {
        for (BloccoGrafico* candidate : blocchi()) {
            if (candidate == destinazione || !haUscitaLibera(candidate)) continue;
            if (source) return nullptr;
            source = candidate;
        }
    }
    if (!source) return nullptr;

    if (source->tipo() == TipoBloccoGrafico::Selezione ||
        source->tipo() == TipoBloccoGrafico::Ciclo) {
        BloccoGrafico* trueTarget = source->ramoVero();
        BloccoGrafico* falseTarget = source->ramoFalso();
        if (!trueTarget) trueTarget = destinazione;
        else falseTarget = destinazione;
        collegaRami(source, trueTarget, falseTarget);
    } else {
        collega(source, destinazione);
    }
    return source;
}

} // namespace flogoritma