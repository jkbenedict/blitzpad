#include "smartindent.h"

#include <Qsci/qsciscintilla.h>

namespace {
using Sci = QsciScintillaBase;

QByteArray lineText(QsciScintilla *ed, int line)
{
    const long start = ed->SendScintilla(Sci::SCI_POSITIONFROMLINE, line);
    const long end = ed->SendScintilla(Sci::SCI_GETLINEENDPOSITION, line);
    QByteArray out;
    for (long p = start; p < end; ++p)
        out.append(char(ed->SendScintilla(Sci::SCI_GETCHARAT, p)));
    return out;
}

bool singleCaretNoSelection(QsciScintilla *ed)
{
    return ed->SendScintilla(Sci::SCI_GETSELECTIONS) == 1 && !ed->hasSelectedText();
}

bool allBlank(const QByteArray &t)
{
    for (char c : t)
        if (c != ' ' && c != '\t') return false;
    return true;
}
} // namespace

bool SmartIndent::beforeEnter(QsciScintilla *ed)
{
    if (!ed->autoIndent() || m_line < 0 || !singleCaretNoSelection(ed)) { reset(); return false; }
    const long pos = ed->SendScintilla(Sci::SCI_GETCURRENTPOS);
    const int line = int(ed->SendScintilla(Sci::SCI_LINEFROMPOSITION, pos));
    const long end = ed->SendScintilla(Sci::SCI_GETLINEENDPOSITION, line);
    const QByteArray text = lineText(ed, line);
    // Only a raw Enter on the line Enter just indented, with nothing typed since.
    if (line != m_line || text != m_text || text.isEmpty() || !allBlank(text) || pos != end) {
        reset();
        return false;
    }
    reset();
    const long start = ed->SendScintilla(Sci::SCI_POSITIONFROMLINE, line);
    ed->SendScintilla(Sci::SCI_BEGINUNDOACTION);
    ed->SendScintilla(Sci::SCI_DELETERANGE, start, end - start);   // the blank line loses its indent
    ed->SendScintilla(Sci::SCI_SETEMPTYSELECTION, start);
    const bool ai = ed->autoIndent();
    ed->setAutoIndent(false);
    ed->SendScintilla(Sci::SCI_NEWLINE);                          // next line starts at column 0
    ed->setAutoIndent(ai);
    ed->SendScintilla(Sci::SCI_ENDUNDOACTION);
    return true;
}

void SmartIndent::afterEnter(QsciScintilla *ed, int fromLine)
{
    reset();
    if (!ed->autoIndent() || !singleCaretNoSelection(ed)) return;
    const long pos = ed->SendScintilla(Sci::SCI_GETCURRENTPOS);
    const int line = int(ed->SendScintilla(Sci::SCI_LINEFROMPOSITION, pos));
    if (line != fromLine + 1) return;
    const int width = int(ed->SendScintilla(Sci::SCI_GETLINEINDENTATION, line));
    if (width <= 0) return;

    // The user indented with tabs: keep tabs on the new line.
    const QByteArray prev = lineText(ed, fromLine);
    if (!prev.isEmpty() && prev.at(0) == '\t' && !ed->indentationsUseTabs()) {
        const int tw = qMax(1, ed->tabWidth());
        const QByteArray indent = QByteArray(width / tw, '\t') + QByteArray(width % tw, ' ');
        const long start = ed->SendScintilla(Sci::SCI_POSITIONFROMLINE, line);
        const long indEnd = ed->SendScintilla(Sci::SCI_GETLINEINDENTPOSITION, line);
        const long caretOffset = pos - indEnd;
        ed->SendScintilla(Sci::SCI_SETTARGETRANGE, start, indEnd);
        ed->SendScintilla(Sci::SCI_REPLACETARGET, (unsigned long)indent.size(), indent.constData());
        ed->SendScintilla(Sci::SCI_GOTOPOS, (unsigned long)(start + indent.size() + qMax(0L, caretOffset)));
    }
    m_line = line;
    m_text = lineText(ed, line);
}
