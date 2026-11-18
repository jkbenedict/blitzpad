#include "fileformatdialog.h"
#include "languages.h"
#include "optionsdialogs.h"
#include "prefs.h"
#include "theme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QTabWidget>
#include <QVBoxLayout>

#include <Qsci/qsciscintilla.h>

static QLineEdit *field(const QString &placeholder, QWidget *parent)
{
    auto *e = new QLineEdit(parent);
    e->setPlaceholderText(placeholder);
    return e;
}

static QStringList splitWords(const QString &s)
{
    return s.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
}

FileFormatDialog::FileFormatDialog(const QString &extension, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Create File Format"));

    // Pick an existing format to edit, or start a new one.
    m_existing = new QComboBox(this);
    m_existing->addItem(tr("(new format)"));
    for (const UserLanguage &u : UserLanguage::loadAll())
        m_existing->addItem(u.name, u.filePath);
    auto *del = new QPushButton(tr("Delete"), this);
    del->setEnabled(false);

    // General
    auto *general = new QWidget(this);
    m_name = field(tr("e.g. My Config Language"), general);
    m_exts = field(tr("e.g. mcl mcfg   (space separated, no dots)"), general);
    m_case = new QCheckBox(tr("Keywords are case sensitive"), general);
    auto *gl = new QFormLayout(general);
    gl->addRow(tr("Name:"), m_name);
    gl->addRow(tr("File extensions:"), m_exts);
    gl->addRow(QString(), m_case);

    // Grammar and symbols
    auto *syntax = new QWidget(this);
    m_lineComment = field(tr("e.g. //  or  #  or  ;"), syntax);
    m_blockStart = field(tr("e.g. /*"), syntax);
    m_blockEnd = field(tr("e.g. */"), syntax);
    m_quotes = field(tr("e.g. \"'"), syntax);
    m_escape = field(tr("e.g. \\"), syntax);
    m_escape->setMaxLength(1);
    m_operators = field(tr("e.g. +-*/=<>!&|;:,.()[]{}"), syntax);
    m_numbers = new QCheckBox(tr("Highlight numbers"), syntax);
    m_keywords = new QPlainTextEdit(syntax);
    m_keywords2 = new QPlainTextEdit(syntax);
    m_keywords->setPlaceholderText(tr("if else while return ...   (space or newline separated)"));
    m_keywords2->setPlaceholderText(tr("int string bool print ...   (types, built-ins)"));
    m_keywords->setMaximumHeight(70);
    m_keywords2->setMaximumHeight(70);
    auto *sl = new QFormLayout(syntax);
    sl->addRow(tr("Line comment:"), m_lineComment);
    sl->addRow(tr("Block comment start:"), m_blockStart);
    sl->addRow(tr("Block comment end:"), m_blockEnd);
    sl->addRow(tr("String quote characters:"), m_quotes);
    sl->addRow(tr("Escape character:"), m_escape);
    sl->addRow(tr("Operators / symbols:"), m_operators);
    sl->addRow(QString(), m_numbers);
    sl->addRow(tr("Keywords:"), m_keywords);
    sl->addRow(tr("Keywords 2:"), m_keywords2);

    // Flow
    auto *flow = new QWidget(this);
    m_foldOpen = field(tr("e.g. {  or  begin if"), flow);
    m_foldClose = field(tr("e.g. }  or  end endif"), flow);
    m_indentFold = new QCheckBox(tr("Fold by indentation instead (Python / YAML style)"), flow);
    auto *fl = new QFormLayout(flow);
    fl->addRow(new QLabel(tr("Code folding: a block opens at one of the opening tokens and closes at a "
                             "closing token. Words (like begin/end) match whole words only."), flow));
    fl->addRow(tr("Block opens with:"), m_foldOpen);
    fl->addRow(tr("Block closes with:"), m_foldClose);
    fl->addRow(QString(), m_indentFold);
    connect(m_indentFold, &QCheckBox::toggled, this, [this](bool on) {
        m_foldOpen->setEnabled(!on);
        m_foldClose->setEnabled(!on);
    });

    // Colours
    auto *colours = new QWidget(this);
    auto *cl = new QGridLayout(colours);
    cl->addWidget(new QLabel(tr("<b>Style</b>"), colours), 0, 0);
    cl->addWidget(new QLabel(tr("<b>Colour</b>"), colours), 0, 1);
    cl->addWidget(new QLabel(tr("<b>Bold</b>"), colours), 0, 2);
    for (int st = 0; st < UserLexer::StyleCount; ++st) {
        cl->addWidget(new QLabel(UserLexer::styleName(st), colours), st + 1, 0);
        auto *b = new ColourButton({}, UserLexer::builtinColour(st), UserLexer::styleName(st), colours);
        auto *bold = new QCheckBox(colours);
        connect(b, &ColourButton::changed, this, &FileFormatDialog::refreshPreview);
        connect(bold, &QCheckBox::toggled, this, &FileFormatDialog::refreshPreview);
        cl->addWidget(b, st + 1, 1);
        cl->addWidget(bold, st + 1, 2);
        m_colours << b;
        m_bold << bold;
    }
    cl->addWidget(new QLabel(tr("Backgrounds, braces and per-style tweaks can also be set later in "
                                "Options > Grammar Colour Overrides."), colours),
                  UserLexer::StyleCount + 1, 0, 1, 3);
    cl->setRowStretch(UserLexer::StyleCount + 2, 1);

    auto *tabs = new QTabWidget(this);
    tabs->addTab(general, tr("General"));
    tabs->addTab(syntax, tr("Grammar && Symbols"));
    tabs->addTab(flow, tr("Flow"));
    tabs->addTab(colours, tr("Colours"));

    m_preview = new QsciScintilla(this);
    m_preview->setUtf8(true);
    m_preview->setMarginType(0, QsciScintilla::NumberMargin);
    m_preview->setMarginLineNumbers(0, true);
    m_preview->setMarginWidth(0, "999");
    m_preview->setFolding(QsciScintilla::BoxedTreeFoldStyle, 2);
    m_preview->setBraceMatching(QsciScintilla::SloppyBraceMatch);
    m_preview->setMinimumHeight(150);
    m_preview->setText(tr("// Type or paste a sample of your format here to try it out.\n"
                          "if (count > 10) {\n    print(\"big\", 3.14)\n} else {\n    /* block\n       comment */\n    print('small')\n}\n"));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &FileFormatDialog::saveFormat);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(del, &QPushButton::clicked, this, &FileFormatDialog::deleteFormat);

    auto *top = new QHBoxLayout;
    top->addWidget(new QLabel(tr("Edit:"), this));
    top->addWidget(m_existing, 1);
    top->addWidget(del);
    auto *lay = new QVBoxLayout(this);
    lay->addLayout(top);
    lay->addWidget(tabs);
    lay->addWidget(new QLabel(tr("Preview (updates as you type):"), this));
    lay->addWidget(m_preview, 1);
    lay->addWidget(buttons);

    // Live preview whenever anything changes.
    for (QLineEdit *e : {m_name, m_exts, m_lineComment, m_blockStart, m_blockEnd, m_quotes, m_escape,
                         m_operators, m_foldOpen, m_foldClose})
        connect(e, &QLineEdit::textChanged, this, &FileFormatDialog::refreshPreview);
    for (QCheckBox *c : {m_case, m_numbers, m_indentFold})
        connect(c, &QCheckBox::toggled, this, &FileFormatDialog::refreshPreview);
    for (QPlainTextEdit *p : {m_keywords, m_keywords2})
        connect(p, &QPlainTextEdit::textChanged, this, &FileFormatDialog::refreshPreview);

    connect(m_existing, &QComboBox::currentIndexChanged, this, [this, del](int i) {
        del->setEnabled(i > 0);
        if (i == 0) {
            UserLanguage u = UserLanguage::defaults();
            u.lineComment = "//"; u.blockCommentStart = "/*"; u.blockCommentEnd = "*/";
            loadInto(u);
            m_originalPath.clear();
            setWindowTitle(tr("Create File Format"));
            return;
        }
        bool ok = false;
        const UserLanguage u = UserLanguage::load(m_existing->itemData(i).toString(), &ok);
        loadInto(u);
        m_originalPath = u.filePath;
        setWindowTitle(tr("Edit File Format: %1").arg(u.name));
    });

    UserLanguage start = UserLanguage::defaults();
    start.lineComment = "//"; start.blockCommentStart = "/*"; start.blockCommentEnd = "*/";
    start.keywords = {"if", "else", "while", "for", "return"};
    start.keywords2 = {"print"};
    if (!extension.isEmpty()) start.extensions = {extension};
    loadInto(start);
    if (!extension.isEmpty()) {
        m_name->setText(extension.toUpper());
        m_name->selectAll();
    }
    m_name->setFocus();
    resize(640, 720);
}

void FileFormatDialog::loadInto(const UserLanguage &u)
{
    m_name->setText(u.name);
    m_exts->setText(u.extensions.join(' '));
    m_case->setChecked(u.caseSensitive);
    m_lineComment->setText(u.lineComment);
    m_blockStart->setText(u.blockCommentStart);
    m_blockEnd->setText(u.blockCommentEnd);
    m_quotes->setText(u.stringDelimiters);
    m_escape->setText(u.escapeChar);
    m_operators->setText(u.operators);
    m_numbers->setChecked(u.highlightNumbers);
    m_keywords->setPlainText(u.keywords.join(' '));
    m_keywords2->setPlainText(u.keywords2.join(' '));
    m_foldOpen->setText(u.foldOpen.join(' '));
    m_foldClose->setText(u.foldClose.join(' '));
    m_indentFold->setChecked(u.foldByIndentation);
    for (int i = 0; i < m_colours.size(); ++i) {
        const QColor c = u.colours.value(i);
        m_colours[i]->setValue(c.isValid() && c != UserLexer::builtinColour(i) ? c : QColor());
        m_bold[i]->setChecked(u.bold.value(i, false));
    }
    refreshPreview();
}

UserLanguage FileFormatDialog::collect() const
{
    UserLanguage u = UserLanguage::defaults();
    u.name = m_name->text().trimmed();
    u.extensions = splitWords(m_exts->text().toLower().remove('.'));
    u.caseSensitive = m_case->isChecked();
    u.lineComment = m_lineComment->text().trimmed();
    u.blockCommentStart = m_blockStart->text().trimmed();
    u.blockCommentEnd = m_blockEnd->text().trimmed();
    u.stringDelimiters = m_quotes->text().remove(' ');
    u.escapeChar = m_escape->text().left(1);
    u.operators = m_operators->text().remove(' ');
    u.highlightNumbers = m_numbers->isChecked();
    u.keywords = splitWords(m_keywords->toPlainText());
    u.keywords2 = splitWords(m_keywords2->toPlainText());
    u.foldOpen = splitWords(m_foldOpen->text());
    u.foldClose = splitWords(m_foldClose->text());
    u.foldByIndentation = m_indentFold->isChecked();
    for (int i = 0; i < m_colours.size(); ++i) {
        u.colours[i] = m_colours[i]->value().isValid() ? m_colours[i]->value() : UserLexer::builtinColour(i);
        u.bold[i] = m_bold[i]->isChecked();
    }
    u.filePath = m_originalPath;
    return u;
}

void FileFormatDialog::refreshPreview()
{
    UserLanguage u = collect();
    if (u.name.isEmpty()) u.name = "preview";
    QsciLexer *old = m_preview->lexer();
    m_preview->setLexer(new UserLexer(u, m_preview));
    delete old;
    m_preview->recolor();
}

void FileFormatDialog::saveFormat()
{
    UserLanguage u = collect();
    if (u.name.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Give the format a name."));
        m_name->setFocus();
        return;
    }
    if (u.extensions.isEmpty()) {
        QMessageBox::warning(this, windowTitle(), tr("Add at least one file extension, e.g. %1.").arg("mcl"));
        m_exts->setFocus();
        return;
    }
    // Don't shadow a built-in language, or another of your formats.
    if (const Language *l = Languages::find(u.name)) {
        const bool sameFile = l->userDefined && !m_originalPath.isEmpty()
                              && m_existing->currentText() == u.name;
        if (!sameFile) {
            QMessageBox::warning(this, windowTitle(), tr("There is already a language called \"%1\". "
                                                         "Choose another name.").arg(u.name));
            m_name->setFocus();
            return;
        }
    }
    QString err;
    if (!u.save(&err)) {
        QMessageBox::warning(this, windowTitle(), err);
        return;
    }
    Languages::reloadUserLanguages();
    Theme::forget(u.name);
    if (!m_originalPath.isEmpty() && m_existing->currentText() != u.name)
        Theme::forget(m_existing->currentText());
    Theme::writeAllDefaults();
    Prefs::settings().sync();
    m_savedName = u.name;
    accept();
}

void FileFormatDialog::deleteFormat()
{
    const int i = m_existing->currentIndex();
    if (i <= 0) return;
    const QString name = m_existing->currentText();
    if (QMessageBox::question(this, windowTitle(), tr("Delete the file format \"%1\"?").arg(name)) != QMessageBox::Yes)
        return;
    QFile::remove(m_existing->itemData(i).toString());
    Languages::reloadUserLanguages();
    Theme::forget(name);
    m_existing->removeItem(i);
    m_existing->setCurrentIndex(0);
}
