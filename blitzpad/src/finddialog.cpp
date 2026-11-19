#include "finddialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QShortcut>
#include <QVBoxLayout>

static QComboBox *historyCombo(QWidget *parent)
{
    auto *c = new QComboBox(parent);
    c->setEditable(true);
    c->setInsertPolicy(QComboBox::NoInsert);
    c->setMaxCount(20);
    c->setMinimumWidth(320);
    c->completer()->setCaseSensitivity(Qt::CaseSensitive);
    return c;
}

FindDialog::FindDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Find"));

    m_find = historyCombo(this);
    m_replace = historyCombo(this);
    m_replaceLabel = new QLabel(tr("Replace with:"), this);

    auto *fields = new QGridLayout;
    fields->addWidget(new QLabel(tr("Find what:"), this), 0, 0);
    fields->addWidget(m_find, 0, 1);
    fields->addWidget(m_replaceLabel, 1, 0);
    fields->addWidget(m_replace, 1, 1);

    m_case = new QCheckBox(tr("Match &case"), this);
    m_word = new QCheckBox(tr("Match &whole word only"), this);
    m_wrap = new QCheckBox(tr("Wra&p around"), this);
    m_wrap->setChecked(true);

    auto *modeBox = new QGroupBox(tr("Search Mode"), this);
    m_normal = new QRadioButton(tr("&Normal"), modeBox);
    m_extended = new QRadioButton(tr("&Extended (\\n, \\r, \\t, \\0, \\x...)"), modeBox);
    m_regex = new QRadioButton(tr("Re&gular expression"), modeBox);
    m_normal->setChecked(true);
    auto *modeLay = new QVBoxLayout(modeBox);
    modeLay->addWidget(m_normal);
    modeLay->addWidget(m_extended);
    modeLay->addWidget(m_regex);
    // Whole word has no meaning for a regex (use \b instead).
    connect(m_regex, &QRadioButton::toggled, m_word, [this](bool on) { m_word->setEnabled(!on); });

    auto *opts = new QVBoxLayout;
    opts->addWidget(m_case);
    opts->addWidget(m_word);
    opts->addWidget(m_wrap);
    opts->addStretch();

    auto *lower = new QHBoxLayout;
    lower->addLayout(opts);
    lower->addWidget(modeBox);

    auto *left = new QVBoxLayout;
    left->addLayout(fields);
    left->addLayout(lower);
    m_message = new QLabel(this);
    m_message->setMinimumHeight(m_message->fontMetrics().height() + 4);
    left->addWidget(m_message);

    auto *next = new QPushButton(tr("Find &Next"), this);
    auto *prev = new QPushButton(tr("Find &Previous"), this);
    auto *cnt = new QPushButton(tr("C&ount"), this);
    auto *findAll = new QPushButton(tr("Fin&d All in All Open Tabs"), this);
    m_replaceBtn = new QPushButton(tr("&Replace"), this);
    m_replaceAllBtn = new QPushButton(tr("Replace &All"), this);
    m_replaceTabsBtn = new QPushButton(tr("Replace All in All Open &Tabs"), this);
    auto *close = new QPushButton(tr("Close"), this);
    next->setDefault(true);

    auto *buttons = new QVBoxLayout;
    for (QPushButton *b : {next, prev, cnt, findAll, m_replaceBtn, m_replaceAllBtn, m_replaceTabsBtn})
        buttons->addWidget(b);
    buttons->addStretch();
    buttons->addWidget(close);

    auto *root = new QHBoxLayout(this);
    root->addLayout(left, 1);
    root->addLayout(buttons);

    connect(next, &QPushButton::clicked, this, [this] { remember(); emit findNext(true); });
    connect(prev, &QPushButton::clicked, this, [this] { remember(); emit findNext(false); });
    connect(cnt, &QPushButton::clicked, this, [this] { remember(); emit count(); });
    connect(findAll, &QPushButton::clicked, this, [this] { remember(); emit findAllInTabs(); });
    connect(m_replaceBtn, &QPushButton::clicked, this, [this] { remember(); emit replaceOne(); });
    connect(m_replaceAllBtn, &QPushButton::clicked, this, [this] { remember(); emit replaceAll(); });
    connect(m_replaceTabsBtn, &QPushButton::clicked, this, [this] { remember(); emit replaceAllInTabs(); });
    connect(close, &QPushButton::clicked, this, &QDialog::hide);
    // Shift+Enter in the dialog searches backwards.
    auto *back = new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Return), this);
    connect(back, &QShortcut::activated, this, [this] { remember(); emit findNext(false); });
    connect(m_find, &QComboBox::editTextChanged, this, [this] { setMessage({}); });
}

void FindDialog::seed(const QString &text)
{
    if (!text.isEmpty() && !text.contains('\n'))
        m_find->setEditText(text);
    setMessage({});
    show();
    raise();
    activateWindow();
    m_find->setFocus();
    m_find->lineEdit()->selectAll();
}

void FindDialog::setReplaceVisible(bool on)
{
    setWindowTitle(on ? tr("Replace") : tr("Find"));
    for (QWidget *w : {static_cast<QWidget *>(m_replace), static_cast<QWidget *>(m_replaceLabel),
                       static_cast<QWidget *>(m_replaceBtn), static_cast<QWidget *>(m_replaceAllBtn),
                       static_cast<QWidget *>(m_replaceTabsBtn)})
        w->setVisible(on);
    adjustSize();
}

void FindDialog::showFind(const QString &s)    { setReplaceVisible(false); seed(s); }
void FindDialog::showReplace(const QString &s) { setReplaceVisible(true); seed(s); }

SearchOptions FindDialog::options() const
{
    SearchOptions o;
    o.find = m_find->currentText();
    o.replace = m_replace->currentText();
    o.mode = m_regex->isChecked() ? SearchMode::Regex
           : m_extended->isChecked() ? SearchMode::Extended : SearchMode::Normal;
    o.matchCase = m_case->isChecked();
    o.wholeWord = m_word->isChecked() && !m_regex->isChecked();
    o.wrap = m_wrap->isChecked();
    return o;
}

void FindDialog::setMessage(const QString &text, bool error)
{
    m_message->setStyleSheet(error ? "color: #c0392b;" : "");
    m_message->setText(text);
}

void FindDialog::remember()
{
    for (QComboBox *c : {m_find, m_replace}) {
        const QString t = c->currentText();
        if (t.isEmpty()) continue;
        const int i = c->findText(t, Qt::MatchExactly | Qt::MatchCaseSensitive);
        if (i == 0) continue;
        if (i > 0) c->removeItem(i);
        c->insertItem(0, t);
        c->setCurrentIndex(0);
    }
}
