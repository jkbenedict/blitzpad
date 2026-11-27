// Tests for QB64 / QBasic colouring. Run: ctest --test-dir build
#include "../src/qb64lexer.h"

#include <QApplication>
#include <QTemporaryDir>
#include <Qsci/qsciscintilla.h>
#include <cstdio>

static int failures = 0;
using Sci = QsciScintillaBase;
using S = QB64Lexer;

// Style of the first character of `needle` (its occurrence number `nth`) in the document.
static int styleOf(QsciScintilla &ed, const QByteArray &text, const char *needle, int nth = 0)
{
    int p = -1;
    for (int k = 0; k <= nth; ++k) p = text.indexOf(needle, p + 1);
    if (p < 0) return -1;
    return int(ed.SendScintilla(Sci::SCI_GETSTYLEAT, (unsigned long)p));
}
static void expect(QsciScintilla &ed, const QByteArray &text, const char *needle, int want, int nth = 0)
{
    const int got = styleOf(ed, text, needle, nth);
    if (got != want) {
        std::printf("FAIL \"%s\" (#%d): style %d, want %d\n", needle, nth, got, want);
        ++failures;
    }
}

int main(int argc, char **argv)
{
    QTemporaryDir home;
    qputenv("HOME", home.path().toUtf8());
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    QsciScintilla ed;
    ed.setUtf8(true);
    QB64Lexer lexer;
    ed.setLexer(&lexer);

    const QByteArray text =
        "$CONSOLE:ONLY\n"
        "'$DYNAMIC\n"
        "OPTION _EXPLICIT\n"
        "DIM title$, count%, big&&, ratio#, flag`, u~%\n"
        "' a comment\n"
        "10 PRINT \"Hello, QB64\": REM greeting\n"
        "start:\n"
        "x = &HFF + 3.5E+2 + 42%\n"
        "s$ = LEFT$(title$, 3) + _TRIM$(\" hi \")\n"
        "_PrintString (10, 20), s$\n"
        "rem lower-case rem works too\n"
        "remark = 1\n"
        "GOTO start\n";
    ed.SendScintilla(Sci::SCI_SETTEXT, (unsigned long)0, text.constData());
    ed.SendScintilla(Sci::SCI_COLOURISE, 0, -1);

    expect(ed, text, "$CONSOLE", S::Metacommand);
    expect(ed, text, "'$DYNAMIC", S::Metacommand);
    expect(ed, text, "OPTION", S::Keyword);
    expect(ed, text, "_EXPLICIT", S::QB64Keyword);
    expect(ed, text, "DIM", S::Keyword);
    expect(ed, text, "title$", S::Default);
    expect(ed, text, "$, count", S::TypeSuffix);
    expect(ed, text, "%, big", S::TypeSuffix);
    expect(ed, text, "&&, ratio", S::TypeSuffix);
    expect(ed, text, "#, flag", S::TypeSuffix);
    expect(ed, text, "`, u", S::TypeSuffix);
    expect(ed, text, "~%", S::TypeSuffix);
    expect(ed, text, "' a comment", S::Comment);
    expect(ed, text, "10 PRINT", S::Label);
    expect(ed, text, "PRINT", S::Keyword);
    expect(ed, text, "\"Hello", S::String);
    expect(ed, text, "REM greeting", S::Comment);
    expect(ed, text, "start:", S::Label);
    expect(ed, text, "&HFF", S::Number);
    expect(ed, text, "3.5E+2", S::Number);
    expect(ed, text, "42%", S::Number);
    expect(ed, text, "%\n", S::Number, 1);          // the suffix stays part of the number
    expect(ed, text, "LEFT$", S::Keyword);
    expect(ed, text, "$(title", S::Keyword);        // LEFT$ as a whole
    expect(ed, text, "_TRIM$", S::QB64Keyword);
    expect(ed, text, "_PrintString", S::QB64Keyword); // case-insensitive
    expect(ed, text, "rem lower", S::Comment);
    expect(ed, text, "remark", S::Default);          // not REM
    expect(ed, text, "GOTO", S::Keyword);
    expect(ed, text, "start\n", S::Default, 0);      // a use of the label, not a definition
    expect(ed, text, "+ 3.5", S::Operator);

    // Words on their own.
    if (QB64Lexer::wordStyle("print") != S::Keyword) { std::printf("FAIL wordStyle(print)\n"); ++failures; }
    if (QB64Lexer::wordStyle("_newimage") != S::QB64Keyword) { std::printf("FAIL wordStyle(_newimage)\n"); ++failures; }
    // Ordinary names that happen to match a QB64 word without its _ stay plain.
    if (QB64Lexer::wordStyle("title") != S::Default) { std::printf("FAIL wordStyle(title)\n"); ++failures; }
    if (QB64Lexer::wordStyle("banana") != S::Default) { std::printf("FAIL wordStyle(banana)\n"); ++failures; }

    if (failures == 0) std::printf("qb64: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
