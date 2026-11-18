#include "mainwindow.h"
#include "editor.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QTimer>
#include <cstdio>
#include <cstring>

int main(int argc, char *argv[])
{
    // Checked before QApplication so it works without a display.
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--version") == 0) {
            std::printf("BlitzPad %s\n", BLITZPAD_VERSION);
            return 0;
        }
    }

    QElapsedTimer startup;
    startup.start();

    QApplication app(argc, argv);
    QApplication::setOrganizationName("BlitzPad");
    QApplication::setApplicationName("BlitzPad");
    QApplication::setApplicationVersion(BLITZPAD_VERSION);

    // --startup-time: print how long it took until the window was drawn
    // with all files loaded, then quit. Used to keep an eye on speed.
    QStringList files = QApplication::arguments().mid(1);
    const bool timing = files.removeAll("--startup-time") > 0;

    MainWindow w;
    if (files.isEmpty())
        w.newTab();
    for (const QString &f : files)
        w.openFile(f);
    w.show();

    if (timing) {
        QTimer::singleShot(0, &app, [&] {
            std::printf("startup: %lld ms\n", static_cast<long long>(startup.elapsed()));
            std::fflush(stdout);
            QApplication::exit(0);
        });
    }
    return app.exec();
}
