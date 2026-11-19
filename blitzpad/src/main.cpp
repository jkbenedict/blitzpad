#include "mainwindow.h"
#include "editor.h"
#include "prefs.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QIcon>
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
    // Links windows to blitzpad.desktop, so GNOME/KDE show the right icon in the
    // dock and taskbar (needed on Wayland).
    QGuiApplication::setDesktopFileName(QStringLiteral("blitzpad"));
    QIcon icon = QIcon::fromTheme(QStringLiteral("blitzpad"));
    if (icon.isNull()) { // not installed yet: use the copy built into the program
        for (int s : {16, 24, 32, 48, 64, 128, 256})
            icon.addFile(QStringLiteral(":/icons/blitzpad-%1.png").arg(s), QSize(s, s));
    }
    QApplication::setWindowIcon(icon);

    // --startup-time: print how long it took until the window was drawn
    // with all files loaded, then quit. Used to keep an eye on speed.
    QStringList files = QApplication::arguments().mid(1);
    const bool timing = files.removeAll("--startup-time") > 0;

    // Make sure ~/.blitzpad/ and its files exist before anything reads them.
    Prefs::ensureConfig();

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
