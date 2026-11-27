// Tests for Options > Run (F5). Run: ctest --test-dir build
#include "../src/rundialogs.h"
#include "../src/runner.h"
#include "../src/prefs.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++failures; } } while (0)

// Runs a RunWindow to the end (the UI stays responsive: it's an event loop).
static RunWindow *runToEnd(RunWindow::Kind k, const QString &dir, const QStringList &cmds, bool stop)
{
    auto *w = new RunWindow(k, "test", dir, cmds, stop);
    w->setAttribute(Qt::WA_DeleteOnClose, false);
    QElapsedTimer t; t.start();
    while (!w->finished() && t.elapsed() < 10000) QApplication::processEvents(QEventLoop::AllEvents, 50);
    return w;
}

int main(int argc, char **argv)
{
    QTemporaryDir home;
    qputenv("HOME", home.path().toUtf8());
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    // Quoting and placeholders.
    CHECK(Run::shellQuote("abc") == "'abc'");
    CHECK(Run::shellQuote("it's") == "'it'\\''s'");
    const QString f = "/tmp/my dir/game.bas";
    CHECK(Run::expand("qb64 -x %current_filename%", f) == "qb64 -x '/tmp/my dir/game.bas'");
    CHECK(Run::expand("cd %current_dir% && ./%current_name%", f) == "cd '/tmp/my dir' && ./'game'");
    CHECK(Run::expand("x %CURRENT_FILENAME%", f) == "x '/tmp/my dir/game.bas'");
    CHECK(Run::runWithCommand("python3", "/a/b c.py") == "python3 '/a/b c.py'");
    CHECK(Run::runWithCommand("python3 -u %current_filename% --fast", "/a/b.py") == "python3 -u '/a/b.py' --fast");
    CHECK(Run::automateCommands({"# note", "", "  make  ", "cp %current_filename% /tmp/"}, "/a/b.c")
          == QStringList({"make", "cp '/a/b.c' /tmp/"}));
    CHECK(Run::group("QB64 / QBasic") == "run-qb64-qbasic");
    CHECK(Run::group("C++") == "run-cpp");

    // Settings are saved per language.
    CHECK(Run::load("Python").mode == Run::Mode::None);
    Run::Config c; c.mode = Run::Mode::RunWith; c.runWith = "python3";
    Run::save("Python", c);
    Run::Config a; a.mode = Run::Mode::Automate; a.automate = {"gcc %current_filename%", "# note", "./a.out"};
    a.stopOnError = false;
    Run::save("C", a);
    CHECK(Run::load("Python").mode == Run::Mode::RunWith);
    CHECK(Run::load("Python").runWith == "python3");
    CHECK(Run::load("C").mode == Run::Mode::Automate);
    CHECK(Run::load("C").automate.size() == 3);
    CHECK(Run::load("C").stopOnError == false);
    CHECK(Run::load("Ruby").mode == Run::Mode::None);

    // Running for real, in the background.
    QTemporaryDir work;
    QFile file(work.path() + "/it's here.txt");
    file.open(QIODevice::WriteOnly); file.write("hello from the file\n"); file.close();

    RunWindow *w = runToEnd(RunWindow::ErrorScreen, work.path(), {Run::runWithCommand("cat", file.fileName())}, true);
    CHECK(w->finished() && w->succeeded());
    CHECK(w->outputText().contains("hello from the file"));
    delete w;

    w = runToEnd(RunWindow::ErrorScreen, work.path(), {"echo oops >&2; exit 3"}, true);
    CHECK(w->finished() && !w->succeeded());
    CHECK(w->outputText().contains("oops"));
    CHECK(w->outputText().contains("exit code 3"));
    delete w;

    w = runToEnd(RunWindow::Info, work.path(), {"echo one", "false", "echo three"}, true);
    CHECK(w->finished() && !w->succeeded());
    CHECK(w->outputText().contains("one") && !w->outputText().contains("three"));
    delete w;

    w = runToEnd(RunWindow::Info, work.path(), {"echo one", "false", "echo three"}, false);
    CHECK(w->finished() && !w->succeeded());
    CHECK(w->outputText().contains("three"));
    delete w;

    w = runToEnd(RunWindow::Info, work.path(),
                 Run::automateCommands({"cp %current_filename% copy.txt", "test -f copy.txt"}, file.fileName()), true);
    CHECK(w->finished() && w->succeeded());
    CHECK(QFile::exists(work.path() + "/copy.txt"));
    delete w;

    if (failures == 0) std::printf("run: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
