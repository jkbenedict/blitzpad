#include "search.h"

#include <Qsci/qsciscintilla.h>

#include <regex>

namespace Search {

QByteArray unescapeExtended(const QString &s)
{
    QString out;
    out.reserve(s.size());
    for (int i = 0; i < s.size(); ++i) {
        const QChar c = s.at(i);
        if (c != '\\' || i + 1 >= s.size()) {
            out += c;
            continue;
        }
        const QChar n = s.at(++i);
        switch (n.unicode()) {
        case 'n': out += '\n'; break;
        case 'r': out += '\r'; break;
        case 't': out += '\t'; break;
        case '0': out += QChar(0); break;
        case '\\': out += '\\'; break;
        case 'x':
        case 'u': {
            const int len = n == 'x' ? 2 : 4;
            bool ok = false;
            const uint v = s.mid(i + 1, len).toUInt(&ok, 16);
            if (ok && s.mid(i + 1, len).size() == len) {
                out += QChar(char16_t(v));
                i += len;
            } else {
                out += '\\';
                out += n;
            }
            break;
        }
        default:
            out += '\\';
            out += n;
        }
    }
    return out.toUtf8();
}

static QByteArray patternBytes(const SearchOptions &o)
{
    return o.mode == SearchMode::Extended ? unescapeExtended(o.find) : o.find.toUtf8();
}

static QByteArray replaceBytes(const SearchOptions &o)
{
    return o.mode == SearchMode::Extended ? unescapeExtended(o.replace) : o.replace.toUtf8();
}

static int flags(const SearchOptions &o)
{
    int f = 0;
    if (o.matchCase) f |= QsciScintillaBase::SCFIND_MATCHCASE;
    if (o.wholeWord) f |= QsciScintillaBase::SCFIND_WHOLEWORD;
    if (o.mode == SearchMode::Regex)
        f |= QsciScintillaBase::SCFIND_REGEXP | QsciScintillaBase::SCFIND_CXX11REGEX;
    return f;
}

// Searches [from, to); from > to searches backwards.
static Status searchRange(QsciScintilla *ed, const QByteArray &pat, int fl, long from, long to, Match *out)
{
    if (pat.isEmpty())
        return Status::NotFound;
    ed->SendScintilla(QsciScintillaBase::SCI_SETSEARCHFLAGS, fl);
    ed->SendScintilla(QsciScintillaBase::SCI_SETTARGETRANGE, from, to);
    const long r = ed->SendScintilla(QsciScintillaBase::SCI_SEARCHINTARGET, pat.size(), pat.constData());
    if (r == -2)
        return Status::BadPattern;
    if (r < 0)
        return Status::NotFound;
    out->start = r;
    out->end = ed->SendScintilla(QsciScintillaBase::SCI_GETTARGETEND);
    return Status::Found;
}

// Check the regex up front so a typo gives a clear "invalid regex" message
// instead of looking like "no match".
static bool regexIsValid(const SearchOptions &o)
{
    if (o.mode != SearchMode::Regex)
        return true;
    try {
        std::regex re(o.find.toStdString(), std::regex::ECMAScript);
        (void)re;
        return true;
    } catch (const std::regex_error &) {
        return false;
    }
}

Status find(QsciScintilla *ed, const SearchOptions &o, long from, long to, Match *out)
{
    if (!regexIsValid(o))
        return Status::BadPattern;
    return searchRange(ed, patternBytes(o), flags(o), from, to, out);
}

static void selectMatch(QsciScintilla *ed, const Match &m)
{
    ed->SendScintilla(QsciScintillaBase::SCI_SETSEL, m.start, m.end);
    ed->SendScintilla(QsciScintillaBase::SCI_SCROLLRANGE, m.end, m.start);
}

Status findNext(QsciScintilla *ed, const SearchOptions &o, bool forward, bool *wrapped)
{
    if (wrapped) *wrapped = false;
    if (!regexIsValid(o))
        return Status::BadPattern;
    const QByteArray pat = patternBytes(o);
    const int fl = flags(o);
    const long len = ed->SendScintilla(QsciScintillaBase::SCI_GETLENGTH);
    const long selStart = ed->SendScintilla(QsciScintillaBase::SCI_GETSELECTIONSTART);
    const long selEnd = ed->SendScintilla(QsciScintillaBase::SCI_GETSELECTIONEND);

    Match m;
    Status st;
    if (forward) {
        // Step past an empty selection that is itself a zero-length match.
        long from = selEnd;
        st = searchRange(ed, pat, fl, from, len, &m);
        if (st == Status::Found && m.start == m.end && m.start == selStart && selStart == selEnd && from < len)
            st = searchRange(ed, pat, fl, from + 1, len, &m);
        if (st == Status::NotFound && o.wrap) {
            st = searchRange(ed, pat, fl, 0, len, &m);
            if (st == Status::Found && wrapped) *wrapped = true;
        }
    } else {
        st = searchRange(ed, pat, fl, selStart, 0, &m);
        if (st == Status::NotFound && o.wrap) {
            st = searchRange(ed, pat, fl, len, 0, &m);
            if (st == Status::Found && wrapped) *wrapped = true;
        }
    }
    if (st == Status::Found)
        selectMatch(ed, m);
    return st;
}

static void replaceTarget(QsciScintilla *ed, const SearchOptions &o, const QByteArray &rep)
{
    const int msg = o.mode == SearchMode::Regex ? QsciScintillaBase::SCI_REPLACETARGETRE
                                                : QsciScintillaBase::SCI_REPLACETARGET;
    ed->SendScintilla(msg, rep.size(), rep.constData());
}

Status replaceOne(QsciScintilla *ed, const SearchOptions &o)
{
    if (!regexIsValid(o))
        return Status::BadPattern;
    const QByteArray pat = patternBytes(o);
    const long selStart = ed->SendScintilla(QsciScintillaBase::SCI_GETSELECTIONSTART);
    const long selEnd = ed->SendScintilla(QsciScintillaBase::SCI_GETSELECTIONEND);

    // Only replace if the selection is exactly a match.
    Match m;
    if (searchRange(ed, pat, flags(o), selStart, selEnd, &m) == Status::Found
        && m.start == selStart && m.end == selEnd) {
        replaceTarget(ed, o, replaceBytes(o));
        const long newEnd = ed->SendScintilla(QsciScintillaBase::SCI_GETTARGETEND);
        ed->SendScintilla(QsciScintillaBase::SCI_SETSEL, newEnd, newEnd);
    }
    return findNext(ed, o, true);
}

// Walks every match in the document, optionally replacing it.
static int walk(QsciScintilla *ed, const SearchOptions &o, bool doReplace)
{
    if (!regexIsValid(o))
        return -1;
    const QByteArray pat = patternBytes(o);
    if (pat.isEmpty())
        return 0;
    const QByteArray rep = replaceBytes(o);
    const int fl = flags(o);

    int n = 0;
    long pos = 0;
    if (doReplace)
        ed->SendScintilla(QsciScintillaBase::SCI_BEGINUNDOACTION);
    for (;;) {
        const long len = ed->SendScintilla(QsciScintillaBase::SCI_GETLENGTH);
        if (pos > len)
            break;
        Match m;
        if (searchRange(ed, pat, fl, pos, len, &m) != Status::Found)
            break;
        ++n;
        long end = m.end;
        if (doReplace) {
            replaceTarget(ed, o, rep);
            end = ed->SendScintilla(QsciScintillaBase::SCI_GETTARGETEND);
        }
        // Zero-length match (e.g. "^" or "$"): step forward one character.
        if (m.start == m.end) {
            const long after = ed->SendScintilla(QsciScintillaBase::SCI_POSITIONAFTER, end);
            end = after > end ? after : end + 1;
        }
        pos = end;
    }
    if (doReplace)
        ed->SendScintilla(QsciScintillaBase::SCI_ENDUNDOACTION);
    return n;
}

int replaceAll(QsciScintilla *ed, const SearchOptions &o) { return walk(ed, o, true); }
int count(QsciScintilla *ed, const SearchOptions &o) { return walk(ed, o, false); }

}
