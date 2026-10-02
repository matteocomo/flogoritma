#include "flogoritma/editor_window.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QIcon>

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName("Flogoritma");
    QCoreApplication::setApplicationVersion("1.0.0");
    QCoreApplication::setOrganizationName("Flogoritma");
    application.setDesktopFileName("flogoritma");
    application.setWindowIcon(QIcon(QStringLiteral(":/icons/flogoritma.svg")));

    flogoritma::EditorWindow editor;
    const QStringList arguments = application.arguments();
    if (arguments.size() > 1 && !editor.openFile(arguments.at(1))) return 1;
    editor.show();
    return application.exec();
}
