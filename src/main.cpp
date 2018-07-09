#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication a(argc, argv);
    QCommandLineParser parser;
    parser.addOptions({
        {{"p", "port"},
            "Set port to <port>.",
            "port"},
        {{"r", "baud-rate"},
            "Set baud rate to <rate>.",
            "rate"},
        {{"i", "immediate"},
            "Start monitoring the port immediately if possible."},
    });
    parser.addHelpOption();
    parser.process(a);
    const QString port = parser.value("p");
    const QString baudRate = parser.value("r");
    const bool immediate = parser.isSet("i");
    MainWindow w(port, baudRate, immediate);
    w.setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            w.size(),
            qApp->screens()[qApp->desktop()->screenNumber()]->availableGeometry()
        )
    );
    w.show();

    return a.exec();
}
