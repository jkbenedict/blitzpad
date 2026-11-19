#pragma once

#include <Qsci/qsciscintilla.h>

// One open document. Wraps QScintilla and keeps track of its file,
// language and line endings.
class Editor : public QsciScintilla {
    Q_OBJECT
public:
    explicit Editor(QWidget *parent = nullptr);

    bool loadFile(const QString &path, QString *error);
    bool saveFile(const QString &path, QString *error);
    // The document as it would be written to disk (original encoding).
    QByteArray contentBytes();

    QString filePath() const { return m_path; }
    QString displayName() const;
    QString language() const { return m_language; }
    // Set after loadFile() when the file's extension wasn't recognised.
    QString unknownExtension() const { return m_unknownExt; }
    void clearUnknownExtension() { m_unknownExt.clear(); }
    void setLanguage(const QString &name);
    QString encodingName() const { return m_wasLatin1 ? "ISO-8859-1" : "UTF-8"; }
    QString eolName() const;
    // Re-reads [editor] options and colours from preferences.ini.
    void reloadPreferences() { applyCommonSettings(); setLanguage(m_language); }

private:
    void applyCommonSettings();
    void updateMarginWidth();

    QString m_path;
    QString m_language = "Plain Text";
    bool m_wasLatin1 = false;
    QString m_unknownExt;
    int m_lastLineDigits = 0;
};
