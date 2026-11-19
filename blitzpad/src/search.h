#pragma once

#include <QByteArray>
#include <QString>

class QsciScintilla;

// Notepad++-style search modes.
enum class SearchMode { Normal, Extended, Regex };

struct SearchOptions {
    QString find;
    QString replace;
    SearchMode mode = SearchMode::Normal;
    bool matchCase = false;
    bool wholeWord = false;
    bool wrap = true;
};

struct Match {
    long start = -1;
    long end = -1;
    bool valid() const { return start >= 0; }
};

// Thin layer over Scintilla's target search. Regex mode uses Scintilla's
// C++11 (ECMAScript) regex engine; in replacements \0 to \9 insert groups
// and \n, \t, \r work as escapes.
namespace Search {

// Turns Extended-mode escapes (\n \r \t \0 \\ \xHH \uHHHH) into real characters.
QByteArray unescapeExtended(const QString &s);

// Result of a search call: a match, no match, or a bad regex.
enum class Status { Found, NotFound, BadPattern };

Status find(QsciScintilla *ed, const SearchOptions &o, long from, long to, Match *out);

// Finds the next (or previous) match from the caret/selection, wrapping if
// asked, and selects it. Sets *wrapped when it had to go around.
Status findNext(QsciScintilla *ed, const SearchOptions &o, bool forward, bool *wrapped = nullptr);

// Replaces the current selection if it is a match, then finds the next one.
Status replaceOne(QsciScintilla *ed, const SearchOptions &o);

// Returns the number of replacements, or -1 for a bad regex. One undo step.
int replaceAll(QsciScintilla *ed, const SearchOptions &o);

// Returns the number of matches, or -1 for a bad regex.
int count(QsciScintilla *ed, const SearchOptions &o);

}
