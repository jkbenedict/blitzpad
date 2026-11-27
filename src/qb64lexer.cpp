#include "qb64lexer.h"
#include "prefs.h"

#include <QSet>
#include <Qsci/qsciscintilla.h>
#include <cctype>

namespace {

// QBasic / QuickBASIC statements, functions and types (QB64 keeps them all).
const char *kCore =
    "abs absolute access alias and any append as asc atn base beep binary bload bsave byval call "
    "calls case cdbl cdecl chain chdir chr$ cint circle clear clng close cls color com command$ common "
    "const cos csng csrlin cvd cvdmbf cvi cvl cvs cvsmbf data date$ declare def defdbl defint deflng "
    "defsng defstr dim do double draw dynamic else elseif end environ environ$ eof eqv erase erdev "
    "erdev$ erl err error exit exp field fileattr files fix for fre freefile function get gosub goto "
    "hex$ if imp inkey$ inp input input$ instr int integer ioctl ioctl$ is key kill lbound lcase$ "
    "left$ len let line list loc locate lock lof log long loop lpos lprint lset ltrim$ mid$ mkd$ "
    "mkdir mkdmbf$ mki$ mkl$ mks$ mksmbf$ mod name next not oct$ off on open option or out output "
    "paint palette pcopy peek pen play pmap point poke pos preset print pset put random randomize "
    "read redim rem reset restore resume return right$ rmdir rnd rset rtrim$ run sadd screen seek "
    "seg select setmem sgn shared shell signal sin single sleep sound space$ spc sqr static step "
    "stick stop str$ strig string string$ sub swap system tab tan then time$ timer to troff tron "
    "type ubound ucase$ uevent unlock until using val varptr varptr$ varseg view wait wend while "
    "width window write xor";

// QB64 additions, with their leading underscore.
const char *kQB64 =
    "_acceptfiledrop _acos _acosh _allowfullscreen _alpha _alpha32 _arccot _arccsc _arcsec _asin _asinh "
    "_assert _atan2 _atanh _autodisplay _axis _backgroundcolor _bin$ _bit _blend _blink _blue _blue32 "
    "_button _buttonchange _byte _capslock _ceil _cinp _clearcolor _clip _clipboard$ _clipboardimage "
    "_color _commandcount _connected _connectionaddress _connectionaddress$ _console _consoleinput "
    "_consoletitle _continue _controlchr _copyimage _copypalette _cos _cosh _cot _coth _csc _csch _cv "
    "_cwd$ _d2g _d2r _debug _declarelibrary _default _defaultcolor _define _deflate$ _delay _depth "
    "_desktopheight _desktopwidth _dest _device$ _deviceinput _devices _dir$ _direxists _display "
    "_displayorder _dontblend _dontwait _droppedfile _droppedfile$ _echo _environcount _errorline "
    "_errormessage$ _exit _explicit _explicitarray _fileexists _filelist$ _finishdrop _float _font "
    "_fontheight _fontwidth _freefont _freeimage _freetimer _fullpath$ _fullscreen _g2d _g2r _glrender "
    "_green _green32 _height _hide _hypot _icon _inclerrorfile$ _inclerrorline _inflate$ _inputbox$ "
    "_instrrev _integer64 _keyclear _keydown _keyhit _lastaxis _lastbutton _lastwheel _limit "
    "_loadfont _loadimage _maptriangle _mapunicode _md5$ _mem _memcopy _memelement _memexists "
    "_memfill _memfree _memget _memimage _memnew _memput _memsound _messagebox _middle _mk$ "
    "_mousebutton _mousehide _mouseinput _mousemove _mousemovementx _mousemovementy _mousepipeopen "
    "_mouseshow _mousewheel _mousex _mousey _newimage _newsound _numlock _offset _openclient "
    "_openconnection _openfiledialog$ _openhost _os$ _palettecolor _pi _pixelsize _preserve "
    "_printimage _printmode _printstring _printwidth _putimage _r2d _r2g _readbit _red _red32 "
    "_readfile$ _resetbit _resize _resizeheight _resizewidth _rgb _rgb32 _rgba _rgba32 _rol _ror "
    "_round _savefiledialog$ _saveimage _scaledheight _scaledwidth _screenclick _screenexists "
    "_screenhide _screenicon _screenimage _screenmove _screenprint _screenshow _screenx _screeny "
    "_scrolllock _sec _sech _selectfolderdialog$ _setalpha _setbit _shellhide _shl _shr _sinh "
    "_sndbal _sndclose _sndcopy _sndgetpos _sndlen _sndlimit _sndloop _sndnew _sndopen _sndopenraw "
    "_sndpause _sndpaused _sndplay _sndplaycopy _sndplayfile _sndplaying _sndrate _sndraw "
    "_sndrawdone _sndrawlen _sndsetpos _sndstop _sndvol _source _startdir$ _status _strcmp _stricmp "
    "_tanh _title _title$ _togglebit _totaldroppedfiles _trim$ _uchar _unsigned _width "
    "_windowhandle _windowhasfocus _writefile";

const QSet<QByteArray> &wordSet(const char *list)
{
    static QHash<const char *, QSet<QByteArray>> cache;
    auto it = cache.find(list);
    if (it == cache.end()) {
        QSet<QByteArray> s;
        for (const QByteArray &w : QByteArray(list).split(' '))
            if (!w.isEmpty()) s.insert(w);
        it = cache.insert(list, s);
    }
    return *it;
}

bool isWordStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; }
bool isWordChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }
bool isDigit(char c) { return std::isdigit(static_cast<unsigned char>(c)); }

// Length of a type suffix at b[i] ($ % %% & && ! # ## ` `n ~% ~%% ~& ~&& ~`), or 0.
int suffixLength(const char *b, int i, int n)
{
    int j = i;
    if (j < n && b[j] == '~') ++j;                       // unsigned
    if (j >= n) return 0;
    const char c = b[j];
    if (c == '%' || c == '&' || c == '#') {
        ++j;
        if (j < n && b[j] == c) ++j;                     // %% && ##
        return j - i;
    }
    if (c == '`') {
        ++j;
        while (j < n && isDigit(b[j])) ++j;              // _BIT * n written as `n
        return j - i;
    }
    if (b[i] == '~') return 0;                           // a lone ~ isn't a suffix
    if (c == '$' || c == '!') {
        ++j;
        return j - i;
    }
    return 0;
}

} // namespace

int QB64Lexer::wordStyle(const QByteArray &w)
{
    if (wordSet(kQB64).contains(w)) return QB64Keyword;
    if (wordSet(kCore).contains(w)) return Keyword;
    return Default;
}

QString QB64Lexer::description(int style) const
{
    switch (style) {
    case Default:     return tr("Default");
    case Comment:     return tr("Comment");
    case Metacommand: return tr("Metacommand");
    case Keyword:     return tr("Keyword");
    case QB64Keyword: return tr("QB64 keyword");
    case String:      return tr("String");
    case Number:      return tr("Number");
    case Operator:    return tr("Operator");
    case Label:       return tr("Line number / label");
    case TypeSuffix:  return tr("Type suffix");
    default:          return QString();
    }
}

QColor QB64Lexer::defaultColor(int style) const
{
    switch (style) {
    case Comment:     return QColor("#008000");
    case Metacommand: return QColor("#7f7f00");
    case Keyword:     return QColor("#00007f");
    case QB64Keyword: return QColor("#7f007f");
    case String:      return QColor("#a31515");
    case Number:      return QColor("#007f7f");
    case Label:       return QColor("#b05a00");
    case TypeSuffix:  return QColor("#555555");
    default:          return QColor("#000000");
    }
}

QFont QB64Lexer::defaultFont(int style) const
{
    QFont f = Prefs::editorFont();
    f.setBold(style == Keyword || style == QB64Keyword || style == Metacommand);
    f.setItalic(style == Comment);
    return f;
}

void QB64Lexer::styleText(int start, int end)
{
    QsciScintilla *ed = editor();
    if (!ed || start >= end)
        return;
    // QB64 has nothing that spans lines, so restyle from the start of the line.
    start = int(ed->SendScintilla(QsciScintillaBase::SCI_POSITIONFROMLINE,
                                  ed->SendScintilla(QsciScintillaBase::SCI_LINEFROMPOSITION, start)));
    QByteArray buf(end - start + 1, '\0');
    ed->SendScintilla(QsciScintillaBase::SCI_GETTEXTRANGE, start, end, buf.data());
    buf.resize(end - start);
    const int n = buf.size();
    const char *b = buf.constData();

    startStyling(start);
    int i = 0;
    bool lineStart = true;   // at the start of a line (for line numbers, labels, metacommands)
    bool stmtStart = true;   // at the start of a statement (line start or after ':')
    auto toEol = [&](int p) { while (p < n && b[p] != '\n' && b[p] != '\r') ++p; return p; };

    while (i < n) {
        const char c = b[i];
        if (c == '\n' || c == '\r') { setStyling(1, Default); ++i; lineStart = stmtStart = true; continue; }
        if (c == ' ' || c == '\t') { setStyling(1, Default); ++i; continue; }

        // Line number at the start of a line: 10 PRINT "HI"
        if (lineStart && isDigit(c)) {
            int j = i; while (j < n && isDigit(b[j])) ++j;
            setStyling(j - i, Label); i = j; lineStart = false; continue;
        }
        // $Metacommand at the start of a line: $CONSOLE:ONLY, $IF WIN THEN, $INCLUDE:'x.bi'
        if (lineStart && c == '$' && i + 1 < n && isWordStart(b[i + 1])) {
            const int j = toEol(i);
            setStyling(j - i, Metacommand); i = j; continue;
        }
        // ' comment, or an old-style '$DYNAMIC / '$INCLUDE: metacommand
        if (c == '\'') {
            const int j = toEol(i);
            setStyling(j - i, (i + 1 < n && b[i + 1] == '$') ? Metacommand : Comment);
            i = j; continue;
        }
        if (c == '"') {
            int j = i + 1;
            while (j < n && b[j] != '"' && b[j] != '\n' && b[j] != '\r') ++j;
            if (j < n && b[j] == '"') ++j;
            setStyling(j - i, String); i = j; lineStart = stmtStart = false; continue;
        }
        // &H1F, &O17, &B1010 (with optional suffix)
        if (c == '&' && i + 1 < n && std::strchr("hHoObB", b[i + 1]) && b[i + 1] != '\0') {
            int j = i + 2;
            while (j < n && std::isxdigit(static_cast<unsigned char>(b[j]))) ++j;
            j += suffixLength(b, j, n);
            setStyling(j - i, Number); i = j; lineStart = stmtStart = false; continue;
        }
        if (isDigit(c) || (c == '.' && i + 1 < n && isDigit(b[i + 1]))) {
            int j = i;
            while (j < n && (isDigit(b[j]) || b[j] == '.')) ++j;
            if (j < n && std::strchr("eEdD", b[j]) && b[j] != '\0'
                && j + 1 < n && (isDigit(b[j + 1]) || ((b[j + 1] == '+' || b[j + 1] == '-') && j + 2 < n && isDigit(b[j + 2])))) {
                j += 2;
                while (j < n && isDigit(b[j])) ++j;
            }
            j += suffixLength(b, j, n);
            setStyling(j - i, Number); i = j; lineStart = stmtStart = false; continue;
        }
        if (isWordStart(c)) {
            int j = i + 1;
            while (j < n && isWordChar(b[j])) ++j;
            const QByteArray base = QByteArray(b + i, j - i).toLower();
            // REM comment (only where a statement starts)
            if (stmtStart && base == "rem") {
                const int k = toEol(i);
                setStyling(k - i, Comment); i = k; continue;
            }
            // Label at the start of a line: myLabel:
            if (lineStart && j < n && b[j] == ':' && wordStyle(base) == Default) {
                setStyling(j + 1 - i, Label); i = j + 1; lineStart = false; stmtStart = true; continue;
            }
            const int sl = suffixLength(b, j, n);
            const QByteArray withSuffix = QByteArray(b + i, j + sl - i).toLower();
            int st = sl ? wordStyle(withSuffix) : Default;
            if (st != Default) {                       // LEFT$, _TRIM$ ... are keywords as a whole
                setStyling(j + sl - i, st);
            } else {
                setStyling(j - i, wordStyle(base));
                if (sl) setStyling(sl, TypeSuffix);
            }
            i = j + sl; lineStart = stmtStart = false; continue;
        }
        // Operators; ':' starts a new statement.
        setStyling(1, std::strchr("+-*/\\^=<>(),;:.?", c) && c != '\0' ? Operator : Default);
        stmtStart = (c == ':');
        lineStart = false;
        ++i;
    }
}
