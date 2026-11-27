// Tests for smart Enter (auto-indent). Run: ctest --test-dir build
#include "../src/smartindent.h"

#include <QApplication>
#include <QKeyEvent>
#include <Qsci/qsciscintilla.h>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++failures; } } while (0)
using Sci = QsciScintillaBase;

static QByteArray doc(QsciScintilla &ed)
{
    const long len = ed.SendScintilla(Sci::SCI_GETLENGTH);
    return QByteArray(reinterpret_cast<const char *>(ed.SendScintilla(Sci::SCI_GETCHARACTERPOINTER)), len);
}
static void setDoc(QsciScintilla &ed, const QByteArray &t)
{
    ed.SendScintilla(Sci::SCI_SETTEXT, (unsigned long)0, t.constData());
    ed.SendScintilla(Sci::SCI_DOCUMENTEND);
}
static void typeText(QsciScintilla &ed, const char *t)
{
    ed.SendScintilla(Sci::SCI_REPLACESEL, (unsigned long)0, t);
}
// What Editor::keyPressEvent does for Enter.
static void enter(QsciScintilla &ed, SmartIndent &si)
{
    if (si.beforeEnter(&ed)) return;
    const int line = int(ed.SendScintilla(Sci::SCI_LINEFROMPOSITION, ed.SendScintilla(Sci::SCI_GETCURRENTPOS)));
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
    QApplication::sendEvent(&ed, &ev);
    si.afterEnter(&ed, line);
}

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    QsciScintilla ed;
    ed.setUtf8(true);
    ed.setEolMode(QsciScintilla::EolUnix);
    ed.setAutoIndent(true);
    ed.setTabWidth(4);
    ed.setIndentationsUseTabs(false);   // BlitzPad's default
    SmartIndent si;

    // 1. Tabs the user typed are carried onto the next line as tabs.
    setDoc(ed, "\tfoo");
    enter(ed, si);
    CHECK(doc(ed) == "\tfoo\n\t");
    typeText(ed, "bar");
    CHECK(doc(ed) == "\tfoo\n\tbar");

    // 2. Two tabs stay two tabs.
    setDoc(ed, "\t\tdeep");
    enter(ed, si);
    CHECK(doc(ed) == "\t\tdeep\n\t\t");

    // 3. A raw Enter on the generated indent drops it and stops the indent.
    setDoc(ed, "\tfoo");
    enter(ed, si);            // "\tfoo\n\t"
    enter(ed, si);            // raw Enter: blank line emptied, next line at column 0
    CHECK(doc(ed) == "\tfoo\n\n");
    typeText(ed, "x");
    CHECK(doc(ed) == "\tfoo\n\nx");

    // 4. Typing on the generated line keeps normal behaviour.
    setDoc(ed, "\tfoo");
    enter(ed, si);
    typeText(ed, "bar");
    enter(ed, si);
    CHECK(doc(ed) == "\tfoo\n\tbar\n\t");

    // 5. If the user types their own tab on the generated line, it is respected.
    setDoc(ed, "\tfoo");
    enter(ed, si);
    typeText(ed, "\t");
    enter(ed, si);
    CHECK(doc(ed) == "\tfoo\n\t\t\n\t\t");

    // 6. Space indentation is carried as spaces, and a raw Enter drops it too.
    setDoc(ed, "    foo");
    enter(ed, si);
    CHECK(doc(ed) == "    foo\n    ");
    enter(ed, si);
    CHECK(doc(ed) == "    foo\n\n");

    // 7. A raw Enter with no indentation is a plain new line.
    setDoc(ed, "foo");
    enter(ed, si);
    enter(ed, si);
    CHECK(doc(ed) == "foo\n\n");

    // 8. One undo takes back the whole raw Enter.
    setDoc(ed, "\tfoo");
    ed.SendScintilla(Sci::SCI_EMPTYUNDOBUFFER);
    enter(ed, si);
    enter(ed, si);
    ed.SendScintilla(Sci::SCI_UNDO);
    CHECK(doc(ed) == "\tfoo\n\t");

    // 9. With auto-indent off, nothing changes.
    ed.setAutoIndent(false);
    setDoc(ed, "\tfoo");
    enter(ed, si);
    CHECK(doc(ed) == "\tfoo\n");

    if (failures == 0) std::printf("smartindent: all tests passed\n");
    return failures == 0 ? 0 : 1;
}
