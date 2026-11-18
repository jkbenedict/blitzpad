#pragma once

#include "userlang.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QsciScintilla;
class ColourButton;

// Options > Create File Format...: define your own language (grammar,
// symbols, flow and colours). Saved to ~/.blitzpad/languages/<name>.ini.
class FileFormatDialog : public QDialog {
    Q_OBJECT
public:
    // `extension` pre-fills the Extensions field (when asked about an unknown file).
    explicit FileFormatDialog(const QString &extension = QString(), QWidget *parent = nullptr);
    QString savedName() const { return m_savedName; }

private:
    void loadInto(const UserLanguage &u);
    UserLanguage collect() const;
    void refreshPreview();
    void saveFormat();
    void deleteFormat();

    QComboBox *m_existing;
    QLineEdit *m_name, *m_exts, *m_lineComment, *m_blockStart, *m_blockEnd,
              *m_quotes, *m_escape, *m_operators, *m_foldOpen, *m_foldClose;
    QCheckBox *m_case, *m_numbers, *m_indentFold;
    QPlainTextEdit *m_keywords, *m_keywords2;
    QList<ColourButton *> m_colours;
    QList<QCheckBox *> m_bold;
    QsciScintilla *m_preview;
    QString m_originalPath;
    QString m_savedName;
};
