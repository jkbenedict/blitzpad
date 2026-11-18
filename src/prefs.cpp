#include "prefs.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QFontDatabase>
#include <QSettings>

namespace Prefs {

QString filePath()
{
    const QString dir = QDir::homePath() + "/.blitzpad";
    QDir().mkpath(dir);
    return dir + "/preferences.ini";
}

QSettings &settings()
{
    static QSettings s(filePath(), QSettings::IniFormat);
    return s;
}

QVariant get(const QString &key, const QVariant &def)
{
    QSettings &s = settings();
    if (!s.contains(key)) {
        s.setValue(key, def);
        return def;
    }
    return s.value(key);
}

// Default value of every [editor] and [view] option. Editor code reads these
// keys with the same defaults, so the file and the program always agree.
static const QList<QPair<const char *, QVariant>> &optionDefaults()
{
    static const QList<QPair<const char *, QVariant>> d = {
        {"editor/font-family", QString()},
        {"editor/font-size", 0},
        {"editor/tab-width", 4},
        {"editor/use-tabs", false},
        {"editor/line-numbers", true},
        {"editor/folding", true},
        {"editor/brace-matching", true},
        {"editor/auto-indent", true},
        {"editor/indent-guides", true},
        {"editor/highlight-current-line", true},
        {"editor/caret-width", 1},
        {"editor/long-line-marker-column", 0},
        {"view/wordWrap", false},
        {"view/showWhitespace", false},
        {"view/useNativeFileDialogs", false},
        {"file-types/ask-about-unknown-formats", true},
        {"file-types/plain-text-extensions", QString()},
    };
    return d;
}

bool ensureConfig()
{
    const QString dir = QDir::homePath() + "/.blitzpad";
    QDir().mkpath(dir + "/languages");
    const bool firstStart = !QFile::exists(filePath());

    QSettings &s = settings();
    for (const auto &kv : optionDefaults())
        if (!s.contains(kv.first)) s.setValue(kv.first, kv.second);

    const QString readme = dir + "/languages/README.txt";
    if (!QFile::exists(readme)) {
        QFile f(readme);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream(&f) <<
                "Your own file formats for BlitzPad live here, one .ini file each.\n"
                "Create and edit them with Options > Create File Format... in BlitzPad.\n\n"
                "[language]  name, extensions (space separated), case-sensitive\n"
                "[syntax]    line-comment, block-comment-start, block-comment-end,\n"
                "            string-delimiters, escape-char, keywords, keywords2 (space\n"
                "            separated), operators, numbers\n"
                "[flow]      fold-open, fold-close (space separated, e.g. { or begin),\n"
                "            fold-by-indentation (true for Python/YAML-style folding)\n"
                "[colours]   #rrggbb per style; [bold] true/false per style\n";
        }
    }
    s.sync();
    return firstStart;
}

QFont editorFont()
{
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    const QString family = get("editor/font-family", QString()).toString().trimmed();
    const int size = get("editor/font-size", 0).toInt();
    if (!family.isEmpty()) f.setFamily(family);
    if (size > 0) f.setPointSize(size);
    f.setStyleHint(QFont::Monospace);
    return f;
}

}
