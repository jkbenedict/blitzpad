#pragma once

#include <QString>
#include <QStringList>

// Options > Run (F5): per-language run settings and command building.
// Saved in preferences.ini, one [run-<language>] group per language:
//   mode = run-with | automate | (empty: nothing set up)
//   run-with = python3                (the file is added to the end)
//   automate = "gcc %current_filename% -o %current_name%\n./%current_name%"
//   automate-stop-on-error = true
namespace Run {

enum class Mode { None, RunWith, Automate };

struct Config {
    Mode mode = Mode::None;
    QString runWith;           // one command; the file is passed to it
    QStringList automate;      // commands, run in order
    bool stopOnError = true;   // Automate: stop at the first command that fails
};

Config load(const QString &language);
void save(const QString &language, const Config &c);
QString group(const QString &language);   // "run-qb64-qbasic"

// Quotes a value for /bin/sh: abc -> 'abc', it's -> 'it'\''s'
QString shellQuote(const QString &s);

// Replaces %current_filename% (full path), %current_dir% (its folder) and
// %current_name% (file name without folder or extension), each shell-quoted.
QString expand(const QString &command, const QString &filePath);

// The shell command for "Run with": placeholders expanded, or the quoted
// file path added to the end when the command has none.
QString runWithCommand(const QString &command, const QString &filePath);

// The commands for "Automate as", expanded; blank lines and lines starting
// with # are skipped.
QStringList automateCommands(const QStringList &lines, const QString &filePath);

// A starting suggestion for "Run with" (e.g. "python3" for Python), or empty.
QString suggestedRunWith(const QString &language);

}
