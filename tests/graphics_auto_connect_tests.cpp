#include "flogoritma/area_disegno.hpp"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    using namespace flogoritma;

    AreaDisegno area;
    BloccoGrafico* start = area.aggiungiBlocco(TipoBloccoGrafico::Start, "Inizio", {0, 0});
    BloccoGrafico* input = area.aggiungiBlocco(TipoBloccoGrafico::Input, "Leggi", {0, 100});
    if (area.collegaAutomaticamente(input, start) != start || start->successore() != input) {
        return 1;
    }

    BloccoGrafico* output = area.aggiungiBlocco(TipoBloccoGrafico::Output, "Mostra", {0, 200});
    if (area.collegaAutomaticamente(output) != input || input->successore() != output) {
        return 2;
    }

    BloccoGrafico* selection = area.aggiungiBlocco(TipoBloccoGrafico::Selezione, "x > 0", {0, 300});
    if (area.collegaAutomaticamente(selection, output) != output ||
        output->successore() != selection) {
        return 3;
    }

    BloccoGrafico* trueOutput = area.aggiungiBlocco(TipoBloccoGrafico::Output, "Vero", {0, 400});
    BloccoGrafico* falseOutput = area.aggiungiBlocco(TipoBloccoGrafico::Output, "Falso", {200, 300});
    if (area.collegaAutomaticamente(trueOutput, selection) != selection ||
        selection->ramoVero() != trueOutput ||
        area.collegaAutomaticamente(falseOutput, selection) != selection ||
        selection->ramoFalso() != falseOutput) {
        return 4;
    }

    BloccoGrafico* ambiguous = area.aggiungiBlocco(TipoBloccoGrafico::Assegnamento, "x = 0", {400, 400});
    if (area.collegaAutomaticamente(ambiguous) != nullptr) return 5;
    return 0;
}