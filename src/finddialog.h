#pragma once

#include "search.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QRadioButton;

// Notepad++-style Find / Replace window. It does not search by itself:
// it emits requests that MainWindow carries out on the right editor(s).
class FindDialog : public QDialog {
    Q_OBJECT
public:
    explicit FindDialog(QWidget *parent = nullptr);

    void showFind(const QString &seed);
    void showReplace(const QString &seed);
    SearchOptions options() const;
    void setMessage(const QString &text, bool error = false);
    void remember(); // add current find/replace text to the history drop-downs

signals:
    void findNext(bool forward);
    void count();
    void replaceOne();
    void replaceAll();
    void replaceAllInTabs();
    void findAllInTabs();

private:
    void setReplaceVisible(bool on);
    void seed(const QString &text);

    QComboBox *m_find;
    QComboBox *m_replace;
    QLabel *m_replaceLabel;
    QRadioButton *m_normal;
    QRadioButton *m_extended;
    QRadioButton *m_regex;
    QCheckBox *m_case;
    QCheckBox *m_word;
    QCheckBox *m_wrap;
    QPushButton *m_replaceBtn;
    QPushButton *m_replaceAllBtn;
    QPushButton *m_replaceTabsBtn;
    QLabel *m_message;
};
