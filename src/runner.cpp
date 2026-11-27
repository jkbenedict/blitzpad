#include "runner.h"
#include "prefs.h"

#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>

namespace Run {

QString group(const QString &language)
{
    QString out = language.toLower();
    out.replace("c++", "cpp");
    out.replace("c#", "csharp");
    out.replace(QRegularExpression("[^a-z0-9]+"), "-");
    out.remove(QRegularExpression("^-+|-+$"));
    return "run-" + (out.isEmpty() ? QStringLiteral("plain-text") : out);
}

Config load(const QString &language)
{
    QSettings &s = Prefs::settings();
    const QString g = group(language) + "/";
    Config c;
    const QString mode = s.value(g + "mode").toString();
    c.mode = mode == "run-with" ? Mode::RunWith : mode == "automate" ? Mode::Automate : Mode::None;
    c.runWith = s.value(g + "run-with").toString();
    const QString script = s.value(g + "automate").toString();
    c.automate = script.isEmpty() ? QStringList() : script.split('\n');
    c.stopOnError = s.value(g + "automate-stop-on-error", true).toBool();
    // A mode whose command is empty counts as nothing set up.
    if (c.mode == Mode::RunWith && c.runWith.trimmed().isEmpty()) c.mode = Mode::None;
    if (c.mode == Mode::Automate && automateCommands(c.automate, "x").isEmpty()) c.mode = Mode::None;
    return c;
}

void save(const QString &language, const Config &c)
{
    QSettings &s = Prefs::settings();
    const QString g = group(language) + "/";
    s.setValue(g + "mode", c.mode == Mode::RunWith ? "run-with" : c.mode == Mode::Automate ? "automate" : "");
    s.setValue(g + "run-with", c.runWith);
    s.setValue(g + "automate", c.automate.join('\n'));
    s.setValue(g + "automate-stop-on-error", c.stopOnError);
    s.sync();
}

QString shellQuote(const QString &v)
{
    QString out = v;
    out.replace('\'', QStringLiteral("'\\''"));
    return '\'' + out + '\'';
}

QString expand(const QString &command, const QString &filePath)
{
    const QFileInfo fi(filePath);
    QString out = command;
    out.replace("%current_filename%", shellQuote(fi.absoluteFilePath()), Qt::CaseInsensitive);
    out.replace("%current_dir%", shellQuote(fi.absolutePath()), Qt::CaseInsensitive);
    out.replace("%current_name%", shellQuote(fi.completeBaseName()), Qt::CaseInsensitive);
    return out;
}

static bool hasPlaceholder(const QString &c)
{
    return c.contains("%current_filename%", Qt::CaseInsensitive) || c.contains("%current_dir%", Qt::CaseInsensitive)
        || c.contains("%current_name%", Qt::CaseInsensitive);
}

QString runWithCommand(const QString &command, const QString &filePath)
{
    const QString c = command.trimmed();
    if (hasPlaceholder(c)) return expand(c, filePath);
    return c + ' ' + shellQuote(QFileInfo(filePath).absoluteFilePath());
}

QStringList automateCommands(const QStringList &lines, const QString &filePath)
{
    QStringList out;
    for (const QString &l : lines) {
        const QString t = l.trimmed();
        if (t.isEmpty() || t.startsWith('#')) continue;
        out << expand(t, filePath);
    }
    return out;
}

QString suggestedRunWith(const QString &language)
{
    static const QHash<QString, QString> s = {
        {"Python", "python3"}, {"Bash", "bash"}, {"JavaScript", "node"}, {"TypeScript", "npx tsx"},
        {"PHP", "php"}, {"Perl", "perl"}, {"Ruby", "ruby"}, {"Lua", "lua"}, {"Tcl", "tclsh"},
        {"Groovy", "groovy"}, {"Octave", "octave --no-gui"}, {"Go", "go run"},
        {"QB64 / QBasic", "qb64 -x %current_filename% -o %current_name% && ./%current_name%"},
        {"C", "gcc -Wall %current_filename% -o %current_name% && ./%current_name%"},
        {"C++", "g++ -Wall %current_filename% -o %current_name% && ./%current_name%"},
        {"Rust", "rustc %current_filename% -o %current_name% && ./%current_name%"},
        {"Makefile", "make -f"},
    };
    return s.value(language);
}

}
