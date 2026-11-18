#include "prefs.h"

#include <QDir>
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
