#pragma once

#include <Qsci/qsciscintilla.h>

// One open document. Wraps QScintilla and keeps track of its file,
// language and line endings.
class Editor : public QsciScintilla {
    Q_OBJECT
public:
    explicit Editor(QWidget *parent = nullptr);

    // `encoding` empty = detect it (byte-order mark, UTF-8, else the
    // [editor] fallback-encoding preference).
    bool loadFile(const QString &path, QString *error, const QString &encoding = QString());
    bool saveFile(const QString &path, QString *error);
    // The document as it would be written to disk (original encoding).
    // *lost counts characters the file's encoding can't hold (saved as '?').
    QByteArray contentBytes(int *lost = nullptr);

    QString filePath() const { return m_path; }
    QString displayName() const;
    QString language() const { return m_language; }
    // Set after loadFile() when the file's extension wasn't recognised.
    QString unknownExtension() const { return m_unknownExt; }
    void clearUnknownExtension() { m_unknownExt.clear(); }
    void setLanguage(const QString &name);
    QString encodingName() const { return m_encoding; }
    // Encoding > Convert to: the text stays, the next save uses `id`.
    void setEncoding(const QString &id);
    // Unsaved edits, or an encoding change not yet saved.
    bool needsSave() const { return isModified() || m_encodingChanged; }
    QString eolName() const;
    // Re-reads [editor] options and colours from preferences.ini.
    void reloadPreferences() { applyCommonSettings(); setLanguage(m_language); }

private:
    void applyCommonSettings();
    void updateMarginWidth();

    QString m_path;
    QString m_language = "Plain Text";
    QString m_encoding = QStringLiteral("UTF-8");
    bool m_encodingChanged = false;
    QString m_unknownExt;
    int m_lastLineDigits = 0;
};
