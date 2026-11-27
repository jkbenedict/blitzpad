#include "rundialogs.h"
#include "prefs.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QPointer>
#include <QTimer>
#include <QVBoxLayout>

static const char *kPlaceholderHelp =
    "<b>%current_filename%</b> is the full path of the file, <b>%current_dir%</b> its folder and "
    "<b>%current_name%</b> its name without folder or extension. Each is quoted for you, so "
    "spaces in names are fine. Commands run with <tt>/bin/sh</tt> in the file's folder.";

// ---- Run with... -------------------------------------------------------------

RunWithDialog::RunWithDialog(const QString &language, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Run with... (%1)").arg(language));
    const Run::Config c = Run::load(language);
    auto *lay = new QVBoxLayout(this);
    auto *intro = new QLabel(tr("The command F5 runs for <b>%1</b> files. The file is added to the end "
                                "of the command, unless you place %current_filename% yourself. "
                                "Output and errors appear in the Error Screen.").arg(language.toHtmlEscaped()));
    intro->setWordWrap(true);
    lay->addWidget(intro);
    m_cmd = new QLineEdit(c.runWith);
    m_cmd->setFont(Prefs::editorFont());
    const QString suggestion = Run::suggestedRunWith(language);
    m_cmd->setPlaceholderText(suggestion.isEmpty() ? tr("e.g. python3") : tr("e.g. %1").arg(suggestion));
    if (c.runWith.isEmpty() && !suggestion.isEmpty()) m_cmd->setText(suggestion);
    m_cmd->setMinimumWidth(520);
    lay->addWidget(m_cmd);
    auto *help = new QLabel(tr(kPlaceholderHelp));
    help->setWordWrap(true);
    lay->addWidget(help);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    QPushButton *remove = buttons->addButton(tr("Remove"), QDialogButtonBox::DestructiveRole);
    remove->setEnabled(!c.runWith.isEmpty());
    connect(remove, &QPushButton::clicked, this, [this] { m_removed = true; accept(); });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    lay->addWidget(buttons);
}

QString RunWithDialog::command() const { return m_cmd->text().trimmed(); }

// ---- Automate as... -----------------------------------------------------------

AutomateDialog::AutomateDialog(const QString &language, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Automate as... (%1)").arg(language));
    const Run::Config c = Run::load(language);
    auto *lay = new QVBoxLayout(this);
    auto *intro = new QLabel(tr("Commands F5 runs for <b>%1</b> files, one per line, top to bottom: "
                                "compile, make, copy, anything your system can do. Lines starting with "
                                "# are notes. The Info window shows each result and whether it all "
                                "succeeded.").arg(language.toHtmlEscaped()));
    intro->setWordWrap(true);
    lay->addWidget(intro);
    m_text = new QPlainTextEdit(c.automate.join('\n'));
    m_text->setFont(Prefs::editorFont());
    m_text->setPlaceholderText(QStringLiteral(
        "# for example:\n"
        "gcc -Wall %current_filename% -o %current_name%\n"
        "./%current_name%\n"
        "cp %current_name% ~/bin/"));
    m_text->setMinimumSize(560, 200);
    lay->addWidget(m_text, 1);
    auto *help = new QLabel(tr(kPlaceholderHelp));
    help->setWordWrap(true);
    lay->addWidget(help);
    m_stop = new QCheckBox(tr("Stop at the first command that fails"));
    m_stop->setChecked(c.stopOnError);
    lay->addWidget(m_stop);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    QPushButton *remove = buttons->addButton(tr("Remove"), QDialogButtonBox::DestructiveRole);
    remove->setEnabled(!c.automate.isEmpty());
    connect(remove, &QPushButton::clicked, this, [this] { m_removed = true; accept(); });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    lay->addWidget(buttons);
}

QStringList AutomateDialog::lines() const
{
    QStringList out = m_text->toPlainText().split('\n');
    while (!out.isEmpty() && out.last().trimmed().isEmpty()) out.removeLast();
    return out;
}

bool AutomateDialog::stopOnError() const { return m_stop->isChecked(); }

// ---- The Error Screen / Info window ---------------------------------------------

RunWindow::RunWindow(Kind kind, const QString &fileName, const QString &workDir,
                     const QStringList &commands, bool stopOnError, QWidget *parent)
    : QDialog(parent), m_kind(kind), m_workDir(workDir), m_commands(commands), m_stopOnError(stopOnError)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(kind == ErrorScreen ? tr("Error Screen - %1").arg(fileName) : tr("Info - %1").arg(fileName));
    setModal(false);
    resize(760, 460);
    auto *lay = new QVBoxLayout(this);
    m_status = new QLabel;
    m_status->setWordWrap(true);
    QFont f = m_status->font(); f.setBold(true); f.setPointSizeF(f.pointSizeF() * 1.15);
    m_status->setFont(f);
    lay->addWidget(m_status);
    m_out = new QPlainTextEdit;
    m_out->setReadOnly(true);
    m_out->setFont(Prefs::editorFont());
    m_out->setLineWrapMode(QPlainTextEdit::NoWrap);
    lay->addWidget(m_out, 1);
    auto *row = new QHBoxLayout;
    m_stopBtn = new QPushButton(tr("&Stop"));
    auto *close = new QPushButton(tr("&Close"));
    row->addWidget(m_stopBtn);
    row->addStretch(1);
    row->addWidget(close);
    lay->addLayout(row);
    connect(m_stopBtn, &QPushButton::clicked, this, [this] {
        if (!m_proc) return;
        m_stopped = true;
        m_proc->terminate();
        QPointer<QProcess> p = m_proc;
        QTimer::singleShot(2000, this, [p] { if (p && p->state() != QProcess::NotRunning) p->kill(); });
    });
    connect(close, &QPushButton::clicked, this, &QDialog::close);
    start();
}

RunWindow::~RunWindow()
{
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        m_proc->disconnect(this);
        m_proc->kill();
        m_proc->waitForFinished(1000);
    }
}

void RunWindow::closeEvent(QCloseEvent *e)
{
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        m_proc->disconnect(this);
        m_proc->kill();
        m_proc->waitForFinished(1000);
    }
    QDialog::closeEvent(e);
}

QString RunWindow::outputText() const { return m_out->toPlainText(); }

void RunWindow::appendText(const QString &text, const QString &colour, bool bold)
{
    QString html = text.toHtmlEscaped();
    html.replace('\n', "<br>");
    if (bold) html = "<b>" + html + "</b>";
    if (!colour.isEmpty()) html = QStringLiteral("<span style=\"color:%1\">%2</span>").arg(colour, html);
    m_out->moveCursor(QTextCursor::End);
    m_out->textCursor().insertHtml(html);
    m_out->moveCursor(QTextCursor::End);
}

void RunWindow::start()
{
    m_out->clear();
    m_step = 0; m_failures = 0; m_done = false; m_ok = false; m_stopped = false;
    m_stopBtn->setEnabled(true);
    m_status->setStyleSheet(QString());
    m_status->setText(tr("Running..."));
    startStep();
}

void RunWindow::startStep()
{
    const QString cmd = m_commands.value(m_step);
    if (m_kind == Info)
        appendText(tr("[%1/%2] $ %3\n").arg(m_step + 1).arg(m_commands.size()).arg(cmd), QString(), true);
    else
        appendText(QStringLiteral("$ %1\n").arg(cmd), QString(), true);
    m_status->setText(m_kind == Info ? tr("Running command %1 of %2...").arg(m_step + 1).arg(m_commands.size())
                                     : tr("Running..."));
    delete m_proc;
    m_proc = new QProcess(this);
    m_proc->setWorkingDirectory(m_workDir);
    connect(m_proc, &QProcess::readyReadStandardOutput, this, [this] {
        appendText(QString::fromLocal8Bit(m_proc->readAllStandardOutput()));
    });
    connect(m_proc, &QProcess::readyReadStandardError, this, [this] {
        appendText(QString::fromLocal8Bit(m_proc->readAllStandardError()), "#c62828");
    });
    connect(m_proc, &QProcess::finished, this, &RunWindow::onFinished);
    connect(m_proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) {
            appendText(tr("Couldn't start /bin/sh.\n"), "#c62828", true);
            finish(false, tr("FAILED: couldn't start the command"));
        }
    });
    m_proc->start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), cmd});
    m_proc->closeWriteChannel();   // nothing to type into; programs see end of input
}

void RunWindow::onFinished(int code, QProcess::ExitStatus status)
{
    const bool ok = status == QProcess::NormalExit && code == 0 && !m_stopped;
    QString how = m_stopped ? tr("stopped") : status == QProcess::CrashExit ? tr("crashed")
                                                                            : tr("exit code %1").arg(code);
    appendText(ok ? tr("-> OK (%1)\n\n").arg(how) : tr("-> FAILED (%1)\n\n").arg(how),
               ok ? "#2e7d32" : "#c62828", true);
    if (!ok) ++m_failures;
    const int total = m_commands.size();
    if (m_stopped || (!ok && (m_kind == ErrorScreen || m_stopOnError)) || m_step + 1 >= total) {
        if (m_kind == ErrorScreen)
            finish(ok, ok ? tr("SUCCESS: finished with exit code 0")
                          : tr("FAILED: %1. See the errors below.").arg(how));
        else if (m_failures == 0)
            finish(true, tr("SUCCESS: all %n command(s) succeeded", nullptr, total));
        else if (m_stopped || m_stopOnError)
            finish(false, tr("FAILED at command %1 of %2 (%3)").arg(m_step + 1).arg(total).arg(how));
        else
            finish(false, tr("FAILED: %1 of %2 command(s) failed").arg(m_failures).arg(total));
        return;
    }
    ++m_step;
    startStep();
}

void RunWindow::finish(bool ok, const QString &summary)
{
    if (m_done) return;
    m_done = true;
    m_ok = ok;
    m_status->setText(summary);
    m_status->setStyleSheet(ok ? "color:#2e7d32" : "color:#c62828");
    m_stopBtn->setEnabled(false);
    if (!ok) { show(); raise(); activateWindow(); }
}
