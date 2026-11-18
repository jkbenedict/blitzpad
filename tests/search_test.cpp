// Tests for the search engine. Run: ctest --test-dir build
#include "../src/search.h"

#include <QApplication>
#include <Qsci/qsciscintilla.h>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++failures; } } while (0)

static QByteArray doc(QsciScintilla &ed)
{
    const long len = ed.SendScintilla(QsciScintillaBase::SCI_GETLENGTH);
    return QByteArray(reinterpret_cast<const char *>(ed.SendScintilla(QsciScintillaBase::SCI_GETCHARACTERPOINTER)), len);
}

static void setDoc(QsciScintilla &ed, const QByteArray &t)
{
    ed.SendScintilla(QsciScintillaBase::SCI_SETTEXT, (unsigned long)0, t.constData());
    ed.SendScintilla(QsciScintillaBase::SCI_GOTOPOS, 0);
}

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    QsciScintilla ed;
    ed.setUtf8(true);
    SearchOptions o;

    // Normal: case and whole word.
    setDoc(ed, "Foo foo food FOO");
    o.find = "foo";
    CHECK(Search::count(&ed, o) == 4);
    o.matchCase = true;
    CHECK(Search::count(&ed, o) == 2);
    o.matchCase = false; o.wholeWord = true;
    CHECK(Search::count(&ed, o) == 3);
    o.wholeWord = false;

    // Find next wraps; find previous goes backwards.
    setDoc(ed, "a1 a2 a3");
    o.find = "a";
    bool wrapped = false;
    CHECK(Search::findNext(&ed, o, true, &wrapped) == Search::Status::Found);
    CHECK(ed.SendScintilla(QsciScintillaBase::SCI_GETSELECTIONSTART) == 0);
    Search::findNext(&ed, o, true); Search::findNext(&ed, o, true);
    CHECK(ed.SendScintilla(QsciScintillaBase::SCI_GETSELECTIONSTART) == 6);
    CHECK(Search::findNext(&ed, o, true, &wrapped) == Search::Status::Found && wrapped);
    CHECK(ed.SendScintilla(QsciScintillaBase::SCI_GETSELECTIONSTART) == 0);
    CHECK(Search::findNext(&ed, o, false, &wrapped) == Search::Status::Found && wrapped);
    CHECK(ed.SendScintilla(QsciScintillaBase::SCI_GETSELECTIONSTART) == 6);
    o.wrap = false;
    CHECK(Search::findNext(&ed, o, true) == Search::Status::NotFound);
    o.wrap = true;

    // Extended mode: \t and \n escapes in find and replace.
    setDoc(ed, "a\tb\nc\td");
    o.mode = SearchMode::Extended; o.find = "\\t"; o.replace = "\\n";
    CHECK(Search::replaceAll(&ed, o) == 2);
    CHECK(doc(ed) == "a\nb\nc\nd");
    CHECK(Search::unescapeExtended("\\x41\\u00e9\\\\") == QByteArray("A\xc3\xa9\\"));

    // Regex: groups in the replacement, one undo step.
    setDoc(ed, "john smith\njane doe\n");
    o.mode = SearchMode::Regex; o.find = "(\\w+) (\\w+)"; o.replace = "\\2, \\1";
    CHECK(Search::replaceAll(&ed, o) == 2);
    CHECK(doc(ed) == "smith, john\ndoe, jane\n");
    ed.undo();
    CHECK(doc(ed) == "john smith\njane doe\n");

    // Regex anchors (zero-length matches) don't loop forever.
    setDoc(ed, "x\ny\nz");
    o.find = "^"; o.replace = "> ";
    CHECK(Search::replaceAll(&ed, o) == 3);
    CHECK(doc(ed) == "> x\n> y\n> z");

    // Regex with case-insensitivity, \d, and replace-one.
    setDoc(ed, "ID 12, id 345");
    o.find = "id (\\d+)"; o.replace = "#\\1"; o.matchCase = false;
    CHECK(Search::count(&ed, o) == 2);
    Search::findNext(&ed, o, true);
    Search::replaceOne(&ed, o);
    CHECK(doc(ed) == "#12, id 345");
    Search::replaceOne(&ed, o);
    CHECK(doc(ed) == "#12, #345");

    // Bad regex is reported, not treated as "no match".
    o.find = "(unclosed";
    CHECK(Search::count(&ed, o) == -1);
    CHECK(Search::findNext(&ed, o, true) == Search::Status::BadPattern);

    // UTF-8 text.
    setDoc(ed, "café CAFÉ");
    o.mode = SearchMode::Normal; o.find = QString::fromUtf8("café");
    CHECK(Search::count(&ed, o) >= 1);

    std::printf(failures ? "%d failure(s)\n" : "all search tests passed\n", failures);
    return failures ? 1 : 0;
}
