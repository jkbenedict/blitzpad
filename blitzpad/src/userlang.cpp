#include "userlang.h"
#include "prefs.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>

#include <Qsci/qsciscintilla.h>

#include <cctype>
#include <cstring>

// ---- definition files ------------------------------------------------------

static const char *const kStyleKeys[] = {"default", "line-comment", "block-comment", "keyword",
                                         "keyword2", "string", "number", "operator"};

QString UserLexer::styleName(int style)
{
    switch (style) {
    case Default:      return QObject::tr("Default text");
    case LineComment:  return QObject::tr("Line comment");
    case BlockComment: return QObject::tr("Block comment");
    case Keyword:      return QObject::tr("Keyword");
    case Keyword2:     return QObject::tr("Keyword 2 (types, built-ins)");
    case String:       return QObject::tr("String");
    case Number:       return QObject::tr("Number");
    case Operator:     return QObject::tr("Operator / symbol");
    }
    return {};
}

QColor UserLexer::builtinColour(int style)
{
    switch (style) {
    case LineComment:
    case BlockComment: return QColor("#008000");
    case Keyword:      return QColor("#00007f");
    case Keyword2:     return QColor("#7f007f");
    case String:       return QColor("#a31515");
    case Number:       return QColor("#007f7f");
    case Operator:     return QColor("#000000");
    default:           return QColor("#000000");
    }
}

UserLanguage UserLanguage::defaults()
{
    UserLanguage u;
    for (int s = 0; s < UserLexer::StyleCount; ++s) {
        u.colours << UserLexer::builtinColour(s);
        u.bold << (s == UserLexer::Keyword);
    }
    return u;
}

QString UserLanguage::directory()
{
    const QString dir = QDir::homePath() + "/.blitzpad/languages";
    QDir().mkpath(dir);
    return dir;
}

static QStringList words(const QString &s)
{
    return s.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
}

UserLanguage UserLanguage::load(const QString &path, bool *ok)
{
    UserLanguage u = defaults();
    QSettings s(path, QSettings::IniFormat);
    u.filePath = path;
    u.name = s.value("language/name").toString().trimmed();
    u.extensions = words(s.value("language/extensions").toString().toLower().remove('.'));
    u.caseSensitive = s.value("language/case-sensitive", true).toBool();
    u.lineComment = s.value("syntax/line-comment").toString();
    u.blockCommentStart = s.value("syntax/block-comment-start").toString();
    u.blockCommentEnd = s.value("syntax/block-comment-end").toString();
    u.stringDelimiters = s.value("syntax/string-delimiters", u.stringDelimiters).toString();
    u.escapeChar = s.value("syntax/escape-char", u.escapeChar).toString().left(1);
    u.keywords = words(s.value("syntax/keywords").toString());
    u.keywords2 = words(s.value("syntax/keywords2").toString());
    u.operators = s.value("syntax/operators", u.operators).toString();
    u.highlightNumbers = s.value("syntax/numbers", true).toBool();
    u.foldOpen = words(s.value("flow/fold-open", "{").toString());
    u.foldClose = words(s.value("flow/fold-close", "}").toString());
    u.foldByIndentation = s.value("flow/fold-by-indentation", false).toBool();
    for (int i = 0; i < UserLexer::StyleCount; ++i) {
        const QColor c(s.value(QString("colours/") + kStyleKeys[i]).toString());
        if (c.isValid()) u.colours[i] = c;
        u.bold[i] = s.value(QString("bold/") + kStyleKeys[i], u.bold[i]).toBool();
    }
    *ok = !u.name.isEmpty() && s.status() == QSettings::NoError;
    return u;
}

static QString fileSlug(const QString &name)
{
    QString s = name.toLower();
    s.replace("c++", "cpp");
    s.replace(QRegularExpression("[^a-z0-9]+"), "-");
    s.remove(QRegularExpression("^-+|-+$"));
    return s.isEmpty() ? QStringLiteral("format") : s;
}

bool UserLanguage::save(QString *error)
{
    const QString path = directory() + "/" + fileSlug(name) + ".ini";
    if (!filePath.isEmpty() && QFileInfo(filePath) != QFileInfo(path))
        QFile::remove(filePath); // renamed: drop the old file
    QFile::remove(path);
    {
        QSettings s(path, QSettings::IniFormat);
        s.setValue("language/name", name);
        s.setValue("language/extensions", extensions.join(' '));
        s.setValue("language/case-sensitive", caseSensitive);
        s.setValue("syntax/line-comment", lineComment);
        s.setValue("syntax/block-comment-start", blockCommentStart);
        s.setValue("syntax/block-comment-end", blockCommentEnd);
        s.setValue("syntax/string-delimiters", stringDelimiters);
        s.setValue("syntax/escape-char", escapeChar);
        s.setValue("syntax/keywords", keywords.join(' '));
        s.setValue("syntax/keywords2", keywords2.join(' '));
        s.setValue("syntax/operators", operators);
        s.setValue("syntax/numbers", highlightNumbers);
        s.setValue("flow/fold-open", foldOpen.join(' '));
        s.setValue("flow/fold-close", foldClose.join(' '));
        s.setValue("flow/fold-by-indentation", foldByIndentation);
        for (int i = 0; i < UserLexer::StyleCount; ++i) {
            s.setValue(QString("colours/") + kStyleKeys[i], colours.value(i, UserLexer::builtinColour(i)).name());
            s.setValue(QString("bold/") + kStyleKeys[i], bold.value(i, false));
        }
        s.sync();
        if (s.status() != QSettings::NoError) {
            *error = QObject::tr("Could not write %1").arg(path);
            return false;
        }
    }
    filePath = path;
    return true;
}

QList<UserLanguage> UserLanguage::loadAll()
{
    QList<UserLanguage> list;
    QDir d(directory());
    for (const QString &f : d.entryList({"*.ini"}, QDir::Files, QDir::Name)) {
        bool ok = false;
        UserLanguage u = load(d.filePath(f), &ok);
        if (ok) list << u;
    }
    return list;
}

// ---- the lexer -------------------------------------------------------------

static bool isWordChar(char c)
{
    const unsigned char u = static_cast<unsigned char>(c);
    return std::isalnum(u) || u == '_' || u >= 0x80;
}

static bool isWordToken(const QString &t)
{
    if (t.isEmpty()) return false;
    for (QChar c : t)
        if (!(c.isLetterOrNumber() || c == '_')) return false;
    return true;
}

namespace {
struct Compiled {
    QSet<QString> kw, kw2, openWords, closeWords;
    QList<QByteArray> openSyms, closeSyms;
};
}

static QHash<const UserLexer *, Compiled> &compiled()
{
    static QHash<const UserLexer *, Compiled> h;
    return h;
}

UserLexer::UserLexer(const UserLanguage &def, QObject *parent)
    : QsciLexerCustom(parent), m_def(def), m_nameUtf8(def.name.toUtf8())
{
    Compiled c;
    auto norm = [&](const QString &w) { return m_def.caseSensitive ? w : w.toLower(); };
    for (const QString &w : m_def.keywords) c.kw.insert(norm(w));
    for (const QString &w : m_def.keywords2) c.kw2.insert(norm(w));
    for (const QString &t : m_def.foldOpen) {
        if (isWordToken(t)) c.openWords.insert(norm(t)); else c.openSyms << t.toUtf8();
    }
    for (const QString &t : m_def.foldClose) {
        if (isWordToken(t)) c.closeWords.insert(norm(t)); else c.closeSyms << t.toUtf8();
    }
    compiled().insert(this, c);
    connect(this, &QObject::destroyed, [p = this] { compiled().remove(p); });
}

QString UserLexer::description(int style) const
{
    return style >= 0 && style < StyleCount ? styleName(style) : QString();
}

QColor UserLexer::defaultColor(int style) const
{
    if (style >= 0 && style < StyleCount && style < m_def.colours.size() && m_def.colours.at(style).isValid())
        return m_def.colours.at(style);
    return builtinColour(style);
}

QFont UserLexer::defaultFont(int style) const
{
    QFont f = Prefs::editorFont();
    f.setBold(style >= 0 && style < m_def.bold.size() && m_def.bold.at(style));
    return f;
}

void UserLexer::styleText(int start, int end)
{
    QsciScintilla *ed = editor();
    if (!ed || start >= end)
        return;
    const Compiled &cp = compiled()[this];

    QByteArray buf(end - start + 1, '\0');
    ed->SendScintilla(QsciScintillaBase::SCI_GETTEXTRANGE, start, end, buf.data());
    buf.resize(end - start);
    const int n = buf.size();
    const char *b = buf.constData();

    const QByteArray lc = m_def.lineComment.toUtf8();
    const QByteArray bs = m_def.blockCommentStart.toUtf8();
    const QByteArray be = m_def.blockCommentEnd.toUtf8();
    const QByteArray quotes = m_def.stringDelimiters.toUtf8();
    const char esc = m_def.escapeChar.isEmpty() ? '\0' : m_def.escapeChar.at(0).toLatin1();
    const QByteArray ops = m_def.operators.toUtf8();

    auto at = [&](int p, const QByteArray &t) {
        return !t.isEmpty() && p + t.size() <= n && std::memcmp(b + p, t.constData(), size_t(t.size())) == 0;
    };
    auto isSpace = [](char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; };

    // Still inside a block comment from the line above?
    bool inBlock = start > 0 && !bs.isEmpty()
                   && ed->SendScintilla(QsciScintillaBase::SCI_GETSTYLEAT, start - 1) == BlockComment;

    QList<QPair<int, int>> folds; // (absolute position, +1 open / -1 close)
    startStyling(start);
    int i = 0;
    while (i < n) {
        if (inBlock) {
            const int j = be.isEmpty() ? -1 : buf.indexOf(be, i);
            const int stop = j < 0 ? n : j + be.size();
            setStyling(stop - i, BlockComment);
            i = stop;
            if (j >= 0) inBlock = false;
            continue;
        }
        const char c = b[i];
        if (at(i, lc)) {
            const int j = buf.indexOf('\n', i);
            const int stop = j < 0 ? n : j;
            setStyling(stop - i, LineComment);
            i = stop;
            continue;
        }
        if (at(i, bs)) {
            setStyling(bs.size(), BlockComment);
            i += bs.size();
            inBlock = true;
            continue;
        }
        if (c != '\0' && quotes.contains(c)) {
            int j = i + 1;
            while (j < n && b[j] != c && b[j] != '\n')
                j += (esc && b[j] == esc && j + 1 < n) ? 2 : 1;
            if (j < n && b[j] == c) ++j;
            setStyling(j - i, String);
            i = j;
            continue;
        }
        if (m_def.highlightNumbers && (i == 0 || !isWordChar(b[i - 1]))
            && (std::isdigit(static_cast<unsigned char>(c))
                || (c == '.' && i + 1 < n && std::isdigit(static_cast<unsigned char>(b[i + 1]))))) {
            int j = i + 1;
            while (j < n && (std::isalnum(static_cast<unsigned char>(b[j])) || b[j] == '.' || b[j] == '_')) ++j;
            setStyling(j - i, Number);
            i = j;
            continue;
        }
        if (isWordChar(c)) {
            int j = i + 1;
            while (j < n && isWordChar(b[j])) ++j;
            QString w = QString::fromUtf8(b + i, j - i);
            if (!m_def.caseSensitive) w = w.toLower();
            const int st = cp.kw.contains(w) ? Keyword : cp.kw2.contains(w) ? Keyword2 : Default;
            if (cp.openWords.contains(w)) folds.append({start + i, +1});
            else if (cp.closeWords.contains(w)) folds.append({start + i, -1});
            setStyling(j - i, st);
            i = j;
            continue;
        }
        bool handled = false;
        for (const QByteArray &t : cp.openSyms) {
            if (at(i, t)) { folds.append({start + i, +1}); setStyling(t.size(), ops.contains(c) ? Operator : Default); i += t.size(); handled = true; break; }
        }
        if (!handled) {
            for (const QByteArray &t : cp.closeSyms) {
                if (at(i, t)) { folds.append({start + i, -1}); setStyling(t.size(), ops.contains(c) ? Operator : Default); i += t.size(); handled = true; break; }
            }
        }
        if (handled) continue;
        if (!isSpace(c) && ops.contains(c)) {
            setStyling(1, Operator);
            ++i;
            continue;
        }
        int j = i + 1;
        if (isSpace(c))
            while (j < n && isSpace(b[j])) ++j;
        setStyling(j - i, Default);
        i = j;
    }

    const int startLine = int(ed->SendScintilla(QsciScintillaBase::SCI_LINEFROMPOSITION, start));
    const int endLine = int(ed->SendScintilla(QsciScintillaBase::SCI_LINEFROMPOSITION, end));
    if (m_def.foldByIndentation)
        computeIndentFolding(qMax(0, startLine - 1), endLine);
    else
        computeFolding(startLine, endLine, folds);
}

// Token folding: each line remembers the nesting depth after it (line state),
// so styling can restart from any line.
void UserLexer::computeFolding(int startLine, int endLine, const QList<QPair<int, int>> &folds)
{
    QsciScintilla *ed = editor();
    int depth = startLine > 0 ? int(ed->SendScintilla(QsciScintillaBase::SCI_GETLINESTATE, startLine - 1)) : 0;
    int k = 0;
    for (int line = startLine; line <= endLine; ++line) {
        const long lineEnd = ed->SendScintilla(QsciScintillaBase::SCI_GETLINEENDPOSITION, line);
        int opens = 0, closes = 0;
        while (k < folds.size() && folds.at(k).first <= lineEnd) {
            if (folds.at(k).second > 0) ++opens; else ++closes;
            ++k;
        }
        const int next = qMax(0, depth + opens - closes);
        // The line's level is the depth it starts at, so a closing "}" stays
        // inside its fold. A line that ends deeper than it starts opens a fold.
        int level = QsciScintillaBase::SC_FOLDLEVELBASE + qMin(depth, 4000);
        if (next > depth)
            level |= QsciScintillaBase::SC_FOLDLEVELHEADERFLAG;
        ed->SendScintilla(QsciScintillaBase::SCI_SETFOLDLEVEL, line, level);
        ed->SendScintilla(QsciScintillaBase::SCI_SETLINESTATE, line, next);
        depth = next;
    }
}

// Indentation folding (Python / YAML style): a line is a fold header when the
// next non-blank line is indented further.
void UserLexer::computeIndentFolding(int startLine, int endLine)
{
    QsciScintilla *ed = editor();
    const int lineCount = int(ed->SendScintilla(QsciScintillaBase::SCI_GETLINECOUNT));
    auto blank = [&](int line) {
        const long s = ed->SendScintilla(QsciScintillaBase::SCI_POSITIONFROMLINE, line);
        const long e = ed->SendScintilla(QsciScintillaBase::SCI_GETLINEENDPOSITION, line);
        return ed->SendScintilla(QsciScintillaBase::SCI_GETLINEINDENTPOSITION, line) >= e || s == e;
    };
    auto indent = [&](int line) { return int(ed->SendScintilla(QsciScintillaBase::SCI_GETLINEINDENTATION, line)); };
    for (int line = startLine; line <= endLine && line < lineCount; ++line) {
        if (blank(line)) {
            // Blank lines belong to the block after them.
            int nxt = line + 1;
            while (nxt < lineCount && nxt < line + 1000 && blank(nxt)) ++nxt;
            const int lvl = nxt < lineCount ? indent(nxt) : 0;
            ed->SendScintilla(QsciScintillaBase::SCI_SETFOLDLEVEL, line,
                              (QsciScintillaBase::SC_FOLDLEVELBASE + qMin(lvl, 4000)) | QsciScintillaBase::SC_FOLDLEVELWHITEFLAG);
            continue;
        }
        const int cur = indent(line);
        int nxt = line + 1;
        while (nxt < lineCount && nxt < line + 1000 && blank(nxt)) ++nxt;
        int level = QsciScintillaBase::SC_FOLDLEVELBASE + qMin(cur, 4000);
        if (nxt < lineCount && indent(nxt) > cur)
            level |= QsciScintillaBase::SC_FOLDLEVELHEADERFLAG;
        ed->SendScintilla(QsciScintillaBase::SCI_SETFOLDLEVEL, line, level);
    }
}
