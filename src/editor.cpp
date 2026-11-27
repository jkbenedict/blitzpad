#include "editor.h"
#include "encodings.h"
#include "languages.h"
#include "prefs.h"
#include "theme.h"

#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QSaveFile>
#include <QStringDecoder>

#include <Qsci/qscilexer.h>

Editor::Editor(QWidget *parent) : QsciScintilla(parent)
{
    applyCommonSettings();
    Theme::apply(this, m_language);
    connect(this, &QsciScintilla::linesChanged, this, &Editor::updateMarginWidth);
}

void Editor::applyCommonSettings()
{
    setUtf8(true);
    const QFont mono = Prefs::editorFont();
    setFont(mono);
    setMarginsFont(mono);

    // Line numbers, plus a fold margin. Every option comes from [editor] in
    // preferences.ini; the values here are only the first-run defaults.
    setMarginType(0, QsciScintilla::NumberMargin);
    const bool numbers = Prefs::get("editor/line-numbers", true).toBool();
    setMarginLineNumbers(0, numbers);
    m_lastLineDigits = 0;
    if (numbers) updateMarginWidth(); else setMarginWidth(0, 0);
    setFolding(Prefs::get("editor/folding", true).toBool() ? QsciScintilla::BoxedTreeFoldStyle
                                                            : QsciScintilla::NoFoldStyle, 2);

    setCaretLineVisible(Prefs::get("editor/highlight-current-line", true).toBool());
    setBraceMatching(Prefs::get("editor/brace-matching", true).toBool() ? QsciScintilla::SloppyBraceMatch
                                                                         : QsciScintilla::NoBraceMatch);
    setAutoIndent(Prefs::get("editor/auto-indent", true).toBool());
    setIndentationGuides(Prefs::get("editor/indent-guides", true).toBool());
    setTabWidth(Prefs::get("editor/tab-width", 4).toInt());
    setIndentationsUseTabs(Prefs::get("editor/use-tabs", false).toBool());
    const int edge = Prefs::get("editor/long-line-marker-column", 0).toInt();
    setEdgeMode(edge > 0 ? QsciScintilla::EdgeLine : QsciScintilla::EdgeNone);
    if (edge > 0) setEdgeColumn(edge);
    SendScintilla(SCI_SETCARETWIDTH, Prefs::get("editor/caret-width", 1).toInt());
    setScrollWidthTracking(true);
    setScrollWidth(1);

    // Multiple selections and rectangular (Alt+drag) selection, like Notepad++.
    SendScintilla(SCI_SETMULTIPLESELECTION, 1);
    SendScintilla(SCI_SETADDITIONALSELECTIONTYPING, 1);
    SendScintilla(SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH);

    // Keep styling work proportional to what is on screen, so large
    // files open fast. Scintilla styles the rest lazily while idle.
    SendScintilla(SCI_SETIDLESTYLING, SC_IDLESTYLING_ALL);
    SendScintilla(SCI_SETLAYOUTCACHE, SC_CACHE_PAGE);
}

void Editor::updateMarginWidth()
{
    if (!marginLineNumbers(0))
        return;
    int digits = QString::number(qMax(1, lines())).size();
    if (digits == m_lastLineDigits)
        return;
    m_lastLineDigits = digits;
    setMarginWidth(0, QString(digits + 1, QLatin1Char('9')));
}

QString Editor::displayName() const
{
    return m_path.isEmpty() ? tr("new") : QFileInfo(m_path).fileName();
}

QString Editor::eolName() const
{
    switch (eolMode()) {
    case EolWindows: return "CRLF";
    case EolMac:     return "CR";
    default:         return "LF";
    }
}

void Editor::setLanguage(const QString &name)
{
    QsciLexer *old = lexer();
    QsciLexer *lx = Languages::createLexer(name, this);
    setLexer(lx);
    delete old;
    m_language = lx ? name : QStringLiteral("Plain Text");
    if (!lx) {
        // setLexer(nullptr) resets fonts; put the editor font back.
        setFont(Prefs::editorFont());
    }
    setMarginsFont(Prefs::editorFont());
    Theme::apply(this, m_language);
    m_lastLineDigits = 0;
    updateMarginWidth();
}

bool Editor::loadFile(const QString &path, QString *error, const QString &encoding)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        *error = f.errorString();
        return false;
    }
    QByteArray data = f.readAll();

    // Scintilla stores UTF-8 bytes. UTF-8 files go in as-is (no conversion);
    // other encodings are converted, and converted back when saving.
    // .nfo, .diz and .ans files come from DOS and use CP437 box drawing.
    static const QStringList dosArt = {"nfo", "diz", "ans"};
    const QString fallback = dosArt.contains(QFileInfo(path).suffix().toLower())
        ? QStringLiteral("CP437") : Prefs::get("editor/fallback-encoding", "ISO-8859-1").toString();
    m_encoding = encoding.isEmpty()
        ? Encodings::detect(data, fallback)
        : (Encodings::find(encoding) ? Encodings::find(encoding)->id : QStringLiteral("UTF-8"));
    m_encodingChanged = false;
    if (m_encoding != "UTF-8")
        data = Encodings::toUtf8(data, m_encoding);

    // Pick line-ending mode from the first line break.
    const int nl = data.indexOf('\n');
    if (nl > 0 && data.at(nl - 1) == '\r')
        setEolMode(EolWindows);
    else if (nl < 0 && data.contains('\r'))
        setEolMode(EolMac);
    else
        setEolMode(EolUnix);

    const QByteArray firstLine = data.left(nl < 0 ? 200 : qMin(nl, 200));
    m_path = QFileInfo(path).absoluteFilePath();
    const QString lang = Languages::detect(m_path, firstLine);
    m_unknownExt = lang.isEmpty() ? QFileInfo(m_path).suffix().toLower() : QString();
    setLanguage(lang.isEmpty() ? QStringLiteral("Plain Text") : lang);

    SendScintilla(SCI_ALLOCATE, (unsigned long)data.size() + 1);
    SendScintilla(SCI_SETTEXT, (unsigned long)0, data.constData());
    SendScintilla(SCI_EMPTYUNDOBUFFER);
    SendScintilla(SCI_SETSAVEPOINT);
    SendScintilla(SCI_GOTOPOS, 0);
    return true;
}

QByteArray Editor::contentBytes(int *lost)
{
    const long len = SendScintilla(SCI_GETLENGTH);
    const char *ptr = reinterpret_cast<const char *>(SendScintilla(SCI_GETCHARACTERPOINTER));
    return Encodings::fromUtf8(QByteArray(ptr, len), m_encoding, lost);
}

void Editor::setEncoding(const QString &id)
{
    const Encodings::Info *info = Encodings::find(id);
    if (!info || info->id == m_encoding) return;
    m_encoding = info->id;
    m_encodingChanged = true;
}

bool Editor::saveFile(const QString &path, QString *error)
{
    const long len = SendScintilla(SCI_GETLENGTH);
    const char *ptr = reinterpret_cast<const char *>(SendScintilla(SCI_GETCHARACTERPOINTER));
    const QByteArray out = contentBytes();

    QSaveFile f(path); // atomic: writes a temp file, then renames
    if (!f.open(QIODevice::WriteOnly)) {
        *error = f.errorString();
        return false;
    }
    f.write(out);
    if (!f.commit()) {
        *error = f.errorString();
        return false;
    }
    const QString old = m_path;
    m_path = QFileInfo(path).absoluteFilePath();
    if (old != m_path)
    {
        // Saved under a new name: switch language if the new name says so.
        const QString lang = Languages::detect(m_path, QByteArray(ptr, qMin(len, 200L)));
        if (!lang.isEmpty() && lang != m_language) setLanguage(lang);
    }
    m_encodingChanged = false;
    SendScintilla(SCI_SETSAVEPOINT);
    return true;
}
