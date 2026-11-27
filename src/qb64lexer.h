#pragma once

#include <Qsci/qscilexercustom.h>

// QB64 (and QBasic / QuickBASIC) syntax colouring. Case-insensitive.
// Covers QBasic keywords, QB64's _-prefixed statements and functions,
// $metacommands (and '$INCLUDE style ones in comments), ' and REM comments,
// strings, numbers (&H, &O, &B too), line numbers and labels, and the type
// suffixes $ % %% & && ! # ## ` ~% ~& and friends.
class QB64Lexer : public QsciLexerCustom {
    Q_OBJECT
public:
    enum Style { Default, Comment, Metacommand, Keyword, QB64Keyword, String, Number,
                 Operator, Label, TypeSuffix, StyleCount };

    explicit QB64Lexer(QObject *parent = nullptr) : QsciLexerCustom(parent) {}
    const char *language() const override { return "QB64"; }
    QString description(int style) const override;
    QColor defaultColor(int style) const override;
    QFont defaultFont(int style) const override;
    void styleText(int start, int end) override;

    // Exposed for tests: the style a QBasic/QB64 word gets (lower case, suffix included).
    static int wordStyle(const QByteArray &lowerWord);
};
