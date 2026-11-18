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
};

namespace Languages {

const QList<Language> &all();

// Picks a language from the file name, falling back to a shebang check
// on the first line, then to Plain Text.
QString detect(const QString &filePath, const QByteArray &firstLine);

// Creates the lexer for a language, or nullptr for Plain Text.
QsciLexer *createLexer(const QString &name, QObject *parent);

}
