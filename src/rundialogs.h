#pragma once

#include "runner.h"

#include <QDialog>
#include <QProcess>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

// Options > Run with...: one command the file is passed to.
class RunWithDialog : public QDialog {
    Q_OBJECT
public:
    RunWithDialog(const QString &language, QWidget *parent = nullptr);
    bool removed() const { return m_removed; }
    QString command() const;

private:
    QLineEdit *m_cmd;
    bool m_removed = false;
};

// Options > Automate as...: any number of commands, run in order.
class AutomateDialog : public QDialog {
    Q_OBJECT
public:
    AutomateDialog(const QString &language, QWidget *parent = nullptr);
    bool removed() const { return m_removed; }
    QStringList lines() const;
    bool stopOnError() const;

private:
    QPlainTextEdit *m_text;
    QCheckBox *m_stop;
    bool m_removed = false;
};

// The window a run reports to: the Error Screen for "Run with", the Info
// window for "Automate as". Runs in the background, shows output as it
// arrives, and can be stopped. Not modal, so you can keep editing.
class RunWindow : public QDialog {
    Q_OBJECT
public:
    enum Kind { ErrorScreen, Info };
    RunWindow(Kind kind, const QString &fileName, const QString &workDir,
              const QStringList &commands, bool stopOnError, QWidget *parent = nullptr);
    ~RunWindow() override;

    // For tests: true once every command has finished (or failed).
    bool finished() const { return m_done; }
    bool succeeded() const { return m_ok; }
    QString outputText() const;

protected:
    void closeEvent(QCloseEvent *e) override;

private:
    void start();
    void startStep();
    void onFinished(int code, QProcess::ExitStatus status);
    void finish(bool ok, const QString &summary);
    void appendText(const QString &text, const QString &colour = QString(), bool bold = false);

    Kind m_kind;
    QString m_workDir;
    QStringList m_commands;
    bool m_stopOnError;
    int m_step = 0;
    int m_failures = 0;
    bool m_done = false;
    bool m_ok = false;
    bool m_stopped = false;
    QProcess *m_proc = nullptr;
    QLabel *m_status;
    QPlainTextEdit *m_out;
    QPushButton *m_stopBtn;
};
