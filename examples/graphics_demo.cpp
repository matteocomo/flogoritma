#include "flogoritma/area_disegno.hpp"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    flogoritma::AreaDisegno area;
    auto* inizio = area.aggiungiBlocco(flogoritma::TipoBloccoGrafico::Start,
                                      "Inizio", {120.0, 80.0});
    auto* input = area.aggiungiBlocco(flogoritma::TipoBloccoGrafico::Input,
                                     "Leggi valore", {120.0, 210.0});
    auto* scelta = area.aggiungiBlocco(flogoritma::TipoBloccoGrafico::Selezione,
                                      "valore > 0?", {120.0, 340.0});
    auto* output = area.aggiungiBlocco(flogoritma::TipoBloccoGrafico::Output,
                                      "Mostra valore", {390.0, 300.0});
    auto* fine = area.aggiungiBlocco(flogoritma::TipoBloccoGrafico::Fine,
                                    "Fine", {120.0, 520.0});

    area.collega(inizio, input);
    area.collega(input, scelta);
    area.collegaRami(scelta, output, fine);
    area.collega(output, fine);

    area.setWindowTitle("Flogoritma - Area di disegno");
    area.show();
    return app.exec();
}