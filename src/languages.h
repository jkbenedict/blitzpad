#pragma once

#include <QString>
#include <QStringList>

class QsciLexer;
class QObject;

// A language the editor can colour. "Plain Text" has no lexer.
struct Language {
    QString name;
    QStringList extensions;   // lower case, no dot
    QStringList filenames;    // exact basenames, e.g. "Makefile"
    QString lineComment;      // for Toggle Comment; empty if the language has none
    bool userDefined = false; // made with Options > Create File Format
};

namespace Languages {

// Built-in languages plus your own formats from ~/.blitzpad/languages.
const QList<Language> &all();
const Language *find(const QString &name);

// Re-reads ~/.blitzpad/languages (after creating or editing a format).
void reloadUserLanguages();

// Picks a language from the file name, a #! line or the first bytes of the
// file. Returns an empty string when the format can't be recognised (an
// unknown extension); files with no extension fall back to Plain Text.
QString detect(const QString &filePath, const QByteArray &firstLine);

// Creates the lexer for a language, or nullptr for Plain Text.
QsciLexer *createLexer(const QString &name, QObject *parent);

}
