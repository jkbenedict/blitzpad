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

// Creates ~/.blitzpad/ and ~/.blitzpad/languages/ if missing, and writes any
// missing [editor] / [view] option with its default. Returns true if
// preferences.ini did not exist before (first start).
bool ensureConfig();

// Editor font from [editor] font-family / font-size (empty or 0 = system monospace).
QFont editorFont();
}
