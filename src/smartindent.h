#pragma once

#include <QByteArray>

class QsciScintilla;

// Smart Enter for auto-indent (BlitzPad 0.21):
//  * Tabs the user typed are kept: when Enter carries a tab-indented line's
//    indentation onto the next line, it is written with tabs, whatever the
//    editor/use-tabs preference says.
//  * When Enter has just generated indentation on a new line and the user
//    presses Enter again straight away (a "raw" Enter, nothing typed), that
//    generated indentation is dropped: the blank line is left empty and the
//    next line starts at column 0, so the indent stops carrying forward.
class SmartIndent {
public:
    // Call before the editor handles Enter. Returns true when it handled the
    // key itself (the raw-Enter case), so the editor must not handle it again.
    bool beforeEnter(QsciScintilla *ed);
    // Call after the editor has handled Enter normally.
    void afterEnter(QsciScintilla *ed, int fromLine);
    // Forget the generated indentation (e.g. after a new document is loaded).
    void reset() { m_line = -1; m_text.clear(); }

private:
    int m_line = -1;      // line whose indentation Enter generated
    QByteArray m_text;    // that line's text right after it was generated
};
