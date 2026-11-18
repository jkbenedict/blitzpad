#pragma once

#include <QColor>
#include <QList>
#include <QString>

class QsciLexer;
class QsciScintilla;

// Colours for the editor, stored in ~/.blitzpad/preferences.ini.
//
//   [colours]                 global defaults, e.g. text-fg=#000000
//   [grammar-cpp]             per-language overrides, e.g. 001-comment-fg=default
//
// "default" means "inherit": a grammar style falls back to the lexer's own
// colour, and plain text / backgrounds fall back to the global colours.
namespace Theme {

struct GlobalItem {
    QString key;     // e.g. "text"
    QString label;   // e.g. "Default text"
    bool hasFg;
    bool hasBg;
    QColor defFg;    // invalid = "no colour" (e.g. keep syntax colours in a selection)
    QColor defBg;
};
const QList<GlobalItem> &globalItems();

// Current global colour (saved value, else the built-in default).
QColor global(const QString &key, bool fg);
// Saves a global colour; an invalid colour resets it to the default.
void setGlobal(const QString &key, bool fg, const QColor &c);

// One row in the Grammar Colour Overrides window.
struct GrammarItem {
    QString key;     // e.g. "001-comment", "brace-matched", "brace-unmatched"
    QString label;   // e.g. "Comment"
    int style;       // Scintilla style number, or -1 for the brace rows
};
// Rows for a language: every lexer style it has, then the two brace rows.
QList<GrammarItem> grammarItems(const QString &language);
QString groupFor(const QString &language);

// Saved override (invalid if "default").
QColor grammarOverride(const QString &language, const QString &key, bool fg);
void setGrammarOverride(const QString &language, const QString &key, bool fg, const QColor &c);
// What the colour is when there is no override.
QColor grammarDefault(const QString &language, const GrammarItem &item, bool fg);

// Writes every colour, shortcut placeholder and option that is missing,
// so preferences.ini always lists everything that can be changed.
void writeAllDefaults();

// Applies global colours and the editor's language overrides.
void apply(QsciScintilla *ed, const QString &language);

}
