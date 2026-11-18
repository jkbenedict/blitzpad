#include "optionsdialogs.h"
#include "languages.h"
#include "prefs.h"
#include "theme.h"

#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>
#include <QVBoxLayout>

#include <Qsci/qscilexer.h>
#include <Qsci/qsciscintilla.h>

// ---- ColourButton ----------------------------------------------------------

ColourButton::ColourButton(const QColor &value, const QColor &fallback, const QString &title, QWidget *parent)
    : QToolButton(parent), m_value(value), m_fallback(fallback), m_title(title)
{
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setPopupMode(QToolButton::MenuButtonPopup);
    setAutoRaise(false);
    setMinimumWidth(150);
    auto *menu = new QMenu(this);
    menu->addAction(tr("Use Default"), this, [this] { setValue({}); emit changed({}); });
    setMenu(menu);
    connect(this, &QToolButton::clicked, this, [this] {
        const QColor start = m_value.isValid() ? m_value : (m_fallback.isValid() ? m_fallback : Qt::black);
        // Qt's own colour picker: the native one can fail on some desktops, like the file dialog did.
        const QColor c = QColorDialog::getColor(start, this, m_title, QColorDialog::DontUseNativeDialog);
        if (c.isValid()) { setValue(c); emit changed(c); }
    });
    refresh();
}

void ColourButton::setValue(const QColor &c)
{
    m_value = c;
    refresh();
}

void ColourButton::refresh()
{
    const QColor shown = m_value.isValid() ? m_value : m_fallback;
    QPixmap pm(28, 16);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setPen(QColor(120, 120, 120));
    if (shown.isValid()) {
        p.setBrush(shown);
    } else {
        p.setBrush(Qt::NoBrush); // no colour at all (e.g. keep syntax colours)
        p.drawLine(1, 15, 27, 1);
    }
    p.drawRect(0, 0, 27, 15);
    p.end();
    setIcon(QIcon(pm));
    setIconSize(pm.size());
    if (m_value.isValid())
        setText(m_value.name());
    else
        setText(m_fallback.isValid() ? tr("Default (%1)").arg(m_fallback.name()) : tr("Default (none)"));
}

// Remembers every key under a prefix so Cancel can put them back.
static QHash<QString, QVariant> snapshot(const QString &prefix)
{
    QHash<QString, QVariant> h;
    QSettings &s = Prefs::settings();
    for (const QString &k : s.allKeys())
        if (k.startsWith(prefix)) h.insert(k, s.value(k));
    return h;
}

static void restore(const QString &prefix, const QHash<QString, QVariant> &h)
{
    QSettings &s = Prefs::settings();
    for (const QString &k : s.allKeys())
        if (k.startsWith(prefix) && !h.contains(k)) s.remove(k);
    for (auto it = h.begin(); it != h.end(); ++it)
        s.setValue(it.key(), it.value());
}

static QTableWidget *colourTable(QWidget *parent)
{
    auto *t = new QTableWidget(parent);
    t->setColumnCount(3);
    t->setHorizontalHeaderLabels({QObject::tr("Element"), QObject::tr("Foreground"), QObject::tr("Background")});
    t->verticalHeader()->hide();
    t->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    t->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    t->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    t->setSelectionMode(QAbstractItemView::NoSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    return t;
}

// ---- Global Colour Preferences ---------------------------------------------

GlobalColoursDialog::GlobalColoursDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Global Colour Preferences"));
    m_snapshot = snapshot("colours/");

    auto *intro = new QLabel(tr("Default colours for every tab. Languages use these unless "
                                "Options > Grammar Colour Overrides sets something else. "
                                "Changes show straight away; Cancel undoes them."), this);
    intro->setWordWrap(true);
    m_table = colourTable(this);

    auto *resetAll = new QPushButton(tr("Reset All to Defaults"), this);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] { Prefs::settings().sync(); accept(); });
    connect(buttons, &QDialogButtonBox::rejected, this, &GlobalColoursDialog::reject);
    connect(resetAll, &QPushButton::clicked, this, [this] {
        for (const auto &g : Theme::globalItems()) {
            if (g.hasFg) Theme::setGlobal(g.key, true, {});
            if (g.hasBg) Theme::setGlobal(g.key, false, {});
        }
        rebuild();
        emit applied();
    });

    auto *bottom = new QHBoxLayout;
    bottom->addWidget(resetAll);
    bottom->addStretch();
    bottom->addWidget(buttons);
    auto *lay = new QVBoxLayout(this);
    lay->addWidget(intro);
    lay->addWidget(m_table, 1);
    lay->addLayout(bottom);
    rebuild();
    resize(640, 460);
}

void GlobalColoursDialog::rebuild()
{
    const auto &items = Theme::globalItems();
    m_table->setRowCount(items.size());
    for (int r = 0; r < items.size(); ++r) {
        const auto &g = items.at(r);
        m_table->setItem(r, 0, new QTableWidgetItem(g.label));
        for (int col : {1, 2}) {
            const bool fg = col == 1;
            if ((fg && !g.hasFg) || (!fg && !g.hasBg)) {
                auto *na = new QTableWidgetItem(QStringLiteral("—"));
                na->setTextAlignment(Qt::AlignCenter);
                m_table->setItem(r, col, na);
                continue;
            }
            const QColor def = fg ? g.defFg : g.defBg;
            const QColor cur = Theme::global(g.key, fg);
            auto *b = new ColourButton(cur == def ? QColor() : cur, def,
                                       QStringLiteral("%1 - %2").arg(g.label, fg ? tr("Foreground") : tr("Background")),
                                       m_table);
            const QString key = g.key;
            connect(b, &ColourButton::changed, this, [this, key, fg](const QColor &c) {
                Theme::setGlobal(key, fg, c);
                emit applied();
            });
            m_table->setCellWidget(r, col, b);
        }
    }
    m_table->resizeRowsToContents();
}

void GlobalColoursDialog::reject()
{
    restore("colours/", m_snapshot);
    emit applied();
    QDialog::reject();
}

// ---- Grammar Colour Overrides ----------------------------------------------

static QString sampleFor(const QString &lang)
{
    if (lang == "C" || lang == "C++" || lang == "Java")
        return "#include <stdio.h>\n// comment\nint main(void) {\n    /* block */\n    const char *s = \"text\";\n"
               "    char c = 'x';\n    for (int i = 0; i < 10; ++i)\n        printf(\"%d\\n\", i);\n    return 0;\n}\n"
               "int broken(void) {   /* the { has no } */\n";
    if (lang == "Bash")
        return "#!/bin/bash\n# comment\nname=\"world\"\nfor i in 1 2 3; do\n  echo \"hi $name $i\"\ndone\n"
               "if [ -f /etc/hosts ]; then cat <<EOF\nhere doc\nEOF\nfi\n{ echo broken\n";
    if (lang == "JavaScript")
        return "// comment\nconst re = /ab+c/i;\nfunction greet(name) {\n  return `hi ${name}` + 'x' + 42;\n}\n"
               "class A { constructor() { this.v = null; } }\nif (true) {\n";
    if (lang == "Python")
        return "# comment\ndef greet(name: str) -> str:\n    \"\"\"Docstring.\"\"\"\n    return f\"hi {name}\" + 'x' * 3\n\n"
               "class A:\n    pass\nprint((1, 2)\n";
    if (lang == "JSON")
        return "{\n  \"name\": \"blitzpad\",\n  \"version\": 5,\n  \"fast\": true,\n  \"list\": [1, 2, null]\n";
    if (lang == "HTML" || lang == "XML")
        return "<!-- comment -->\n<div class=\"box\" id='a'>\n  <p>Text &amp; more</p>\n</div>\n<span>\n";
    if (lang == "CSS")
        return "/* comment */\nbody { color: #333; margin: 0 auto; }\n.box:hover { background: red !important; }\n@media print {\n";
    if (lang == "Markdown")
        return "# Heading\n\nSome *emphasis* and **strong** text with `code`.\n\n- item\n- [link](https://example.com)\n";
    if (lang == "Makefile" || lang == "CMake")
        return "# comment\nCXX := g++\nall: main.o\n\t$(CXX) -o app main.o\nproject(demo)\nset(X \"value\")\n";
    if (lang == "YAML")
        return "# comment\nname: blitzpad\nversion: 5\nlist:\n  - one\n  - \"two\"\n";
    if (lang == "INI / Config")
        return "; comment\n[section]\nkey=value\nother = 42\n";
    if (lang == "Diff")
        return "--- a/file.txt\n+++ b/file.txt\n@@ -1,3 +1,3 @@\n context\n-removed line\n+added line\n";
    if (lang == "SQL")
        return "-- comment\nSELECT id, name FROM users WHERE age > 21 AND name = 'x';\n";
    return "Plain text.\nBraces (like these) and {unclosed ones\n";
}

GrammarColoursDialog::GrammarColoursDialog(const QString &language, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Grammar Colour Overrides"));
    m_snapshot = snapshot("grammar-");

    m_lang = new QComboBox(this);
    for (const Language &l : Languages::all())
        m_lang->addItem(l.name);
    m_lang->setCurrentText(language);

    auto *intro = new QLabel(tr("Per-language colours. \"Default\" follows the language's built-in colour "
                                "or your Global Colour Preferences. Changes show straight away; Cancel undoes them."), this);
    intro->setWordWrap(true);
    m_table = colourTable(this);

    m_preview = new QsciScintilla(this);
    m_preview->setUtf8(true);
    m_preview->setReadOnly(false);
    m_preview->setMarginType(0, QsciScintilla::NumberMargin);
    m_preview->setMarginLineNumbers(0, true);
    m_preview->setMarginWidth(0, "999");
    m_preview->setBraceMatching(QsciScintilla::SloppyBraceMatch);
    m_preview->setCaretLineVisible(true);
    m_preview->setMinimumHeight(170);
    m_preview->setToolTip(tr("Preview. Put the cursor next to a brace to see matched/unmatched colours."));

    auto *resetLang = new QPushButton(tr("Reset This Language"), this);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] { Prefs::settings().sync(); accept(); });
    connect(buttons, &QDialogButtonBox::rejected, this, &GrammarColoursDialog::reject);
    connect(resetLang, &QPushButton::clicked, this, [this] {
        const QString lang = m_lang->currentText();
        for (const auto &it : Theme::grammarItems(lang)) {
            Theme::setGrammarOverride(lang, it.key, true, {});
            Theme::setGrammarOverride(lang, it.key, false, {});
        }
        rebuild();
        emit applied();
    });
    connect(m_lang, &QComboBox::currentTextChanged, this, &GrammarColoursDialog::rebuild);

    auto *top = new QHBoxLayout;
    top->addWidget(new QLabel(tr("Language:"), this));
    top->addWidget(m_lang, 1);
    auto *bottom = new QHBoxLayout;
    bottom->addWidget(resetLang);
    bottom->addStretch();
    bottom->addWidget(buttons);
    auto *lay = new QVBoxLayout(this);
    lay->addLayout(top);
    lay->addWidget(intro);
    lay->addWidget(m_table, 2);
    lay->addWidget(new QLabel(tr("Preview:"), this));
    lay->addWidget(m_preview, 1);
    lay->addLayout(bottom);
    rebuild();
    resize(720, 680);
}

void GrammarColoursDialog::refreshPreview()
{
    const QString lang = m_lang->currentText();
    QsciLexer *old = m_preview->lexer();
    QsciLexer *lx = Languages::createLexer(lang, m_preview);
    m_preview->setLexer(lx);
    delete old;
    if (!lx) m_preview->setFont(Prefs::editorFont());
    Theme::apply(m_preview, lang);
}

void GrammarColoursDialog::rebuild()
{
    const QString lang = m_lang->currentText();
    const auto items = Theme::grammarItems(lang);
    m_table->setRowCount(items.size());
    for (int r = 0; r < items.size(); ++r) {
        const auto &it = items.at(r);
        m_table->setItem(r, 0, new QTableWidgetItem(it.label));
        for (int col : {1, 2}) {
            const bool fg = col == 1;
            auto *b = new ColourButton(Theme::grammarOverride(lang, it.key, fg),
                                       Theme::grammarDefault(lang, it, fg),
                                       QStringLiteral("%1: %2 - %3").arg(lang, it.label, fg ? tr("Foreground") : tr("Background")),
                                       m_table);
            const QString key = it.key;
            connect(b, &ColourButton::changed, this, [this, lang, key, fg](const QColor &c) {
                Theme::setGrammarOverride(lang, key, fg, c);
                refreshPreview();
                emit applied();
            });
            m_table->setCellWidget(r, col, b);
        }
    }
    m_table->resizeRowsToContents();
    m_preview->setText(sampleFor(lang));
    refreshPreview();
}

void GrammarColoursDialog::reject()
{
    restore("grammar-", m_snapshot);
    emit applied();
    QDialog::reject();
}
