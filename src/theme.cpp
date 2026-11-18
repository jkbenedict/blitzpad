#include "theme.h"
#include "languages.h"
#include "prefs.h"

#include <QHash>
#include <QRegularExpression>
#include <QSettings>

#include <Qsci/qscilexer.h>
#include <Qsci/qsciscintilla.h>

#include <memory>

namespace Theme {

const QList<GlobalItem> &globalItems()
{
    static const QList<GlobalItem> items = {
        {"text",            "Default text",                         true,  true,  QColor("#000000"), QColor("#ffffff")},
        {"current-line",    "Current line",                         false, true,  {},                QColor("#e8e8ff")},
        {"selection",       "Selection",                            true,  true,  {},                QColor("#c6dcf5")},
        {"caret",           "Caret (cursor)",                       true,  false, QColor("#000000"), {}},
        {"line-numbers",    "Line numbers",                         true,  true,  QColor("#5a5a5a"), QColor("#eeeeee")},
        {"fold-margin",     "Fold margin",                          false, true,  {},                QColor("#f4f4f4")},
        {"brace-matched",   "Matched brace",                        true,  true,  QColor("#0000ff"), QColor("#d6f5d6")},
        {"brace-unmatched", "Unmatched brace (broken grammar)",     true,  true,  QColor("#ffffff"), QColor("#d93636")},
        {"indent-guides",   "Indentation guides",                   true,  false, QColor("#c8c8c8"), {}},
        {"whitespace",      "Whitespace and line-ending symbols",   true,  false, QColor("#b0b0b0"), {}},
    };
    return items;
}

static QString suffix(bool fg) { return fg ? "-fg" : "-bg"; }

static QString colourToText(const QColor &c) { return c.isValid() ? c.name() : QStringLiteral("default"); }

static QColor textToColour(const QString &s)
{
    const QColor c(s.trimmed());
    return c.isValid() ? c : QColor();
}

static const GlobalItem *findGlobal(const QString &key)
{
    for (const GlobalItem &g : globalItems())
        if (g.key == key) return &g;
    return nullptr;
}

QColor global(const QString &key, bool fg)
{
    const GlobalItem *g = findGlobal(key);
    if (!g) return {};
    const QColor def = fg ? g->defFg : g->defBg;
    const QVariant v = Prefs::settings().value("colours/" + key + suffix(fg));
    if (!v.isValid()) return def;
    const QColor c = textToColour(v.toString());
    return c.isValid() ? c : def;
}

void setGlobal(const QString &key, bool fg, const QColor &c)
{
    const GlobalItem *g = findGlobal(key);
    const QColor def = g ? (fg ? g->defFg : g->defBg) : QColor();
    Prefs::settings().setValue("colours/" + key + suffix(fg), colourToText(c.isValid() ? c : def));
}

// ---- grammar ---------------------------------------------------------------

static QString slug(const QString &s)
{
    QString out = s.toLower();
    out.replace("c++", "cpp");
    out.replace(QRegularExpression("[^a-z0-9]+"), "-");
    return out.remove(QRegularExpression("^-+|-+$"));
}

QString groupFor(const QString &language) { return "grammar-" + slug(language); }

// A pristine lexer per language, kept to read its built-in colours.
static QsciLexer *pristine(const QString &language)
{
    static QHash<QString, std::shared_ptr<QsciLexer>> cache;
    auto it = cache.find(language);
    if (it == cache.end())
        it = cache.insert(language, std::shared_ptr<QsciLexer>(Languages::createLexer(language, nullptr)));
    return it->get();
}

QList<GrammarItem> grammarItems(const QString &language)
{
    QList<GrammarItem> items;
    if (QsciLexer *lx = pristine(language)) {
        for (int s = 0; s < 256; ++s) {
            const QString d = lx->description(s);
            if (!d.isEmpty())
                items.append({QStringLiteral("%1-%2").arg(s, 3, 10, QLatin1Char('0')).arg(slug(d)), d, s});
        }
    } else {
        items.append({"000-text", "Text", 0});
    }
    items.append({"brace-matched", "Matched brace", -1});
    items.append({"brace-unmatched", "Unmatched brace (broken grammar, e.g. { with no })", -1});
    return items;
}

QColor grammarOverride(const QString &language, const QString &key, bool fg)
{
    return textToColour(Prefs::settings().value(groupFor(language) + "/" + key + suffix(fg)).toString());
}

void setGrammarOverride(const QString &language, const QString &key, bool fg, const QColor &c)
{
    Prefs::settings().setValue(groupFor(language) + "/" + key + suffix(fg), colourToText(c));
}

QColor grammarDefault(const QString &language, const GrammarItem &item, bool fg)
{
    if (item.style < 0)
        return global(item.key, fg); // brace rows inherit the global brace colours
    QsciLexer *lx = pristine(language);
    if (!lx)
        return global("text", fg);
    if (fg) {
        const QColor c = lx->color(item.style);
        return c == lx->defaultColor() ? global("text", true) : c;
    }
    const QColor p = lx->paper(item.style);
    return p == lx->defaultPaper() ? global("text", false) : p;
}

void writeAllDefaults()
{
    QSettings &s = Prefs::settings();
    for (const GlobalItem &g : globalItems()) {
        for (bool fg : {true, false}) {
            if ((fg && !g.hasFg) || (!fg && !g.hasBg)) continue;
            const QString k = "colours/" + g.key + suffix(fg);
            if (!s.contains(k)) s.setValue(k, colourToText(fg ? g.defFg : g.defBg));
        }
    }
    for (const Language &l : Languages::all()) {
        for (const GrammarItem &it : grammarItems(l.name)) {
            for (bool fg : {true, false}) {
                const QString k = groupFor(l.name) + "/" + it.key + suffix(fg);
                if (!s.contains(k)) s.setValue(k, "default");
            }
        }
    }
}

static QColor effective(const QString &language, const GrammarItem &it, bool fg)
{
    const QColor o = grammarOverride(language, it.key, fg);
    return o.isValid() ? o : grammarDefault(language, it, fg);
}

void apply(QsciScintilla *ed, const QString &language)
{
    const QColor textFg = global("text", true), textBg = global("text", false);
    QsciLexer *lx = ed->lexer();
    const QList<GrammarItem> items = grammarItems(language);
    const GrammarItem *matched = nullptr, *unmatched = nullptr;
    for (const GrammarItem &it : items) {
        if (it.key == "brace-matched") matched = &it;
        else if (it.key == "brace-unmatched") unmatched = &it;
    }

    if (lx) {
        lx->setDefaultColor(textFg);
        lx->setDefaultPaper(textBg);
        for (const GrammarItem &it : items) {
            if (it.style < 0) continue;
            lx->setColor(effective(language, it, true), it.style);
            lx->setPaper(effective(language, it, false), it.style);
        }
    } else {
        // Plain text: style 0 only.
        const GrammarItem &t = items.first();
        ed->setColor(effective(language, t, true));
        ed->setPaper(effective(language, t, false));
    }

    ed->setCaretLineBackgroundColor(global("current-line", false));
    const QColor selFg = global("selection", true);
    if (selFg.isValid()) ed->setSelectionForegroundColor(selFg);
    else ed->resetSelectionForegroundColor();
    ed->setSelectionBackgroundColor(global("selection", false));
    ed->setCaretForegroundColor(global("caret", true));
    ed->setMarginsForegroundColor(global("line-numbers", true));
    ed->setMarginsBackgroundColor(global("line-numbers", false));
    ed->setFoldMarginColors(global("fold-margin", false), global("fold-margin", false));
    ed->setMatchedBraceForegroundColor(effective(language, *matched, true));
    ed->setMatchedBraceBackgroundColor(effective(language, *matched, false));
    ed->setUnmatchedBraceForegroundColor(effective(language, *unmatched, true));
    ed->setUnmatchedBraceBackgroundColor(effective(language, *unmatched, false));
    ed->setIndentationGuidesForegroundColor(global("indent-guides", true));
    ed->setIndentationGuidesBackgroundColor(textBg);
    ed->setWhitespaceForegroundColor(global("whitespace", true));
}

}
