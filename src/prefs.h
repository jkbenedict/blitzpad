#pragma once

#include <QFont>
#include <QString>
#include <QVariant>

class QSettings;

// All settings live in ~/.blitzpad/preferences.ini (plain INI, safe to edit by hand).
// Every option is written out on first start, so the file lists everything.
namespace Prefs {
QSettings &settings();
QString filePath();

// Reads a value, writing the default into the file if it isn't there yet.
QVariant get(const QString &key, const QVariant &def);

// Editor font from [editor] font-family / font-size (empty or 0 = system monospace).
QFont editorFont();
}
