#pragma once

#include <QColor>
#include <QList>
#include <QString>
#include <QStringList>

#include <Qsci/qscilexercustom.h>

// A file format you define yourself (Options > Create File Format...).
// Stored as one INI file per format in ~/.blitzpad/languages/.
struct UserLanguage {
    QString name;
    QStringList extensions;        // lower case, no dot
    bool caseSensitive = true;

    // Grammar and symbols
    QString lineComment;           // e.g. "//" or "#"
    QString blockCommentStart;     // e.g. "/*"
    QString blockCommentEnd;       // e.g. "*/"
    QString stringDelimiters = "\"'";
    QString escapeChar = "\\";
    QStringList keywords;
    QStringList keywords2;         // types, built-ins, ...
    QString operators = "+-*/%=<>!&|^~?:;,.()[]{}";
    bool highlightNumbers = true;

    // Flow (folding)
    QStringList foldOpen = {"{"};
    QStringList foldClose = {"}"};
    bool foldByIndentation = false;

    // Colours, one per style (see UserLexer::Style)
    QList<QColor> colours;
    QList<bool> bold;

    QString filePath;              // where it was loaded from / saved to

    static UserLanguage defaults();
    static UserLanguage load(const QString &path, bool *ok);
    bool save(QString *error);     // writes to ~/.blitzpad/languages/<name>.ini
    static QString directory();    // ~/.blitzpad/languages
    static QList<UserLanguage> loadAll();
};

// The lexer that colours text and computes folding for a UserLanguage.
class UserLexer : public QsciLexerCustom {
    Q_OBJECT
public:
    enum Style { Default, LineComment, BlockComment, Keyword, Keyword2, String, Number, Operator, StyleCount };

    UserLexer(const UserLanguage &def, QObject *parent = nullptr);
    const char *language() const override { return m_nameUtf8.constData(); }
    QString description(int style) const override;
    QColor defaultColor(int style) const override;
    QFont defaultFont(int style) const override;
    void styleText(int start, int end) override;

    static QString styleName(int style);
    static QColor builtinColour(int style);

private:
    void computeFolding(int startLine, int endLine, const QList<QPair<int, int>> &foldDeltas);
    void computeIndentFolding(int startLine, int endLine);

    UserLanguage m_def;
    QByteArray m_nameUtf8;
};
