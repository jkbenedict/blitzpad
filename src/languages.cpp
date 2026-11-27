#include "languages.h"
#include "qb64lexer.h"
#include "prefs.h"
#include "userlang.h"

#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>

#include <functional>

#include <Qsci/qscilexeravs.h>
#include <Qsci/qscilexerbash.h>
#include <Qsci/qscilexerbatch.h>
#include <Qsci/qscilexercmake.h>
#include <Qsci/qscilexercoffeescript.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qscilexercsharp.h>
#include <Qsci/qscilexercss.h>
#include <Qsci/qscilexerd.h>
#include <Qsci/qscilexerdiff.h>
#include <Qsci/qscilexeredifact.h>
#include <Qsci/qscilexerfortran.h>
#include <Qsci/qscilexerfortran77.h>
#include <Qsci/qscilexerhtml.h>
#include <Qsci/qscilexeridl.h>
#include <Qsci/qscilexerintelhex.h>
#include <Qsci/qscilexerjava.h>
#include <Qsci/qscilexerjavascript.h>
#include <Qsci/qscilexerjson.h>
#include <Qsci/qscilexerlua.h>
#include <Qsci/qscilexermakefile.h>
#include <Qsci/qscilexermarkdown.h>
#include <Qsci/qscilexermasm.h>
#include <Qsci/qscilexermatlab.h>
#include <Qsci/qscilexernasm.h>
#include <Qsci/qscilexeroctave.h>
#include <Qsci/qscilexerpascal.h>
#include <Qsci/qscilexerperl.h>
#include <Qsci/qscilexerpo.h>
#include <Qsci/qscilexerpostscript.h>
#include <Qsci/qscilexerpov.h>
#include <Qsci/qscilexerproperties.h>
#include <Qsci/qscilexerpython.h>
#include <Qsci/qscilexerruby.h>
#include <Qsci/qscilexerspice.h>
#include <Qsci/qscilexersql.h>
#include <Qsci/qscilexersrec.h>
#include <Qsci/qscilexertcl.h>
#include <Qsci/qscilexertekhex.h>
#include <Qsci/qscilexertex.h>
#include <Qsci/qscilexerverilog.h>
#include <Qsci/qscilexervhdl.h>
#include <Qsci/qscilexerxml.h>
#include <Qsci/qscilexeryaml.h>

namespace Languages {

namespace {

// C-like languages QScintilla has no lexer of its own for (Go, Rust, Kotlin...).
// Scintilla's C/C++ lexer handles their comments, strings and numbers; this
// gives it the right keywords and string rules.
class CFamilyLexer : public QsciLexerCPP
{
public:
    CFamilyLexer(QObject *parent, const char *name, const char *keywords, const char *types,
                 bool backquoted = false, bool tripleQuoted = false)
        : QsciLexerCPP(parent), m_name(name), m_kw(keywords), m_types(types),
          m_backquoted(backquoted), m_triple(tripleQuoted) {}

    const char *language() const override { return m_name; }
    const char *keywords(int set) const override
    {
        if (set == 1) return m_kw;
        if (set == 2) return m_types;                   // "secondary keywords" style
        if (set == 3) return QsciLexerCPP::keywords(3); // doc-comment keywords
        return nullptr;
    }
    void refreshProperties() override
    {
        QsciLexerCPP::refreshProperties();
        emit propertyChanged("lexer.cpp.backquoted.strings", m_backquoted ? "1" : "0");
        emit propertyChanged("lexer.cpp.triplequoted.strings", m_triple ? "1" : "0");
    }

private:
    const char *m_name, *m_kw, *m_types;
    bool m_backquoted, m_triple;
};

const char kGo[] = "break case chan const continue default defer else fallthrough for func go goto if import "
                   "interface map package range return select struct switch type var true false nil iota";
const char kGoT[] = "any bool byte comparable complex64 complex128 error float32 float64 int int8 int16 int32 "
                    "int64 rune string uint uint8 uint16 uint32 uint64 uintptr append cap clear close complex "
                    "copy delete imag len make max min new panic print println real recover";
const char kRust[] = "as async await break const continue crate dyn else enum extern false fn for if impl in let "
                     "loop match mod move mut pub ref return self Self static struct super trait true type union "
                     "unsafe use where while yield macro_rules";
const char kRustT[] = "bool char str i8 i16 i32 i64 i128 isize u8 u16 u32 u64 u128 usize f32 f64 String Vec "
                      "Option Result Box Rc Arc Some None Ok Err";
const char kKotlin[] = "abstract actual annotation as break by catch class companion const constructor continue "
                       "crossinline data do else enum expect external false final finally for fun get if import "
                       "in infix init inline inner interface internal is lateinit noinline null object open "
                       "operator out override package private protected public reified return sealed set super "
                       "suspend tailrec this throw true try typealias val var vararg when where while";
const char kKotlinT[] = "Any Unit Nothing Int Long Short Byte Float Double Boolean Char String Array List "
                        "MutableList Map MutableMap Set MutableSet";
const char kSwift[] = "actor any as associatedtype async await break case catch class continue convenience "
                      "default defer deinit do dynamic else enum extension fallthrough false fileprivate final "
                      "for func guard if import in indirect init inout internal is lazy let mutating nil "
                      "nonmutating open operator optional override private protocol public repeat required "
                      "rethrows return self Self some static struct subscript super switch throw throws true "
                      "try typealias unowned var weak where while";
const char kSwiftT[] = "Int Int8 Int16 Int32 Int64 UInt UInt8 UInt16 UInt32 UInt64 Float Double Bool String "
                       "Character Array Dictionary Set Optional Void Any AnyObject";
const char kDart[] = "abstract as assert async await base break case catch class const continue covariant "
                     "default deferred do dynamic else enum export extends extension external factory false "
                     "final finally for get hide if implements import in interface is late library mixin new "
                     "null of on operator part required rethrow return sealed set show static super switch sync "
                     "this throw true try typedef var void when while with yield";
const char kDartT[] = "int double num bool String List Map Set Future Stream Object Function Iterable Never Null Record";
const char kScala[] = "abstract case catch class def do else enum export extends false final finally for forSome "
                      "given if implicit import lazy match new null object override package private protected "
                      "return sealed super then this throw trait true try type using val var while with yield";
const char kScalaT[] = "Int Long Short Byte Float Double Boolean Char String Unit Any AnyRef Nothing List Map Set "
                       "Option Some None Seq Array";
const char kGroovy[] = "abstract as assert break case catch class const continue def default do else enum "
                       "extends false final finally for goto if implements import in instanceof interface native "
                       "new null package private protected public return static strictfp super switch "
                       "synchronized this threadsafe throw throws trait transient true try var void volatile while";
const char kGroovyT[] = "boolean byte char double float int long short String List Map Object Integer";
const char kObjC[] = "auto break case char const continue default do double else enum extern float for goto if "
                     "inline int long register restrict return short signed sizeof static struct switch typedef "
                     "union unsigned void volatile while interface implementation end protocol property "
                     "synthesize dynamic class selector encode try catch finally throw autoreleasepool optional "
                     "required public private protected import self super nil Nil YES NO";
const char kObjCT[] = "id BOOL SEL IMP instancetype NSObject NSString NSArray NSDictionary NSInteger NSUInteger "
                      "CGFloat nonatomic atomic strong weak copy assign readonly readwrite retain";
const char kTS[] = "abstract accessor any as asserts async await bigint boolean break case catch class const "
                   "constructor continue debugger declare default delete do else enum export extends false "
                   "finally for from function get if implements import in infer instanceof interface is keyof "
                   "let module namespace never new null number object of override package private protected "
                   "public readonly require return satisfies set static string super switch symbol this throw "
                   "true try type typeof undefined unique unknown var void while with yield";
const char kTST[] = "Array Promise Record Partial Required Readonly Pick Omit Map Set Date RegExp Error JSON Math console";
const char kProto[] = "syntax edition package import option message enum service rpc returns stream oneof map "
                      "reserved extensions extend optional required repeated public weak true false to max";
const char kProtoT[] = "double float int32 int64 uint32 uint64 sint32 sint64 fixed32 fixed64 sfixed32 sfixed64 "
                       "bool string bytes";

using Factory = std::function<QsciLexer *(QObject *)>;
template <class T> Factory make() { return [](QObject *p) -> QsciLexer * { return new T(p); }; }
Factory cfamily(const char *name, const char *kw, const char *types, bool bq = false, bool triple = false)
{
    return [=](QObject *p) -> QsciLexer * { return new CFamilyLexer(p, name, kw, types, bq, triple); };
}

struct BuiltIn {
    Language lang;
    Factory lexer; // empty for Plain Text
};

const QString P = QStringLiteral("Programming");
const QString S = QStringLiteral("Scripting");
const QString M = QStringLiteral("Markup & Data");
const QString B = QStringLiteral("Build & Config");
const QString H = QStringLiteral("Hardware & Low-level");

// Every built-in language: name, extensions, exact file names, line comment,
// menu category, and the lexer that colours it.
const QList<BuiltIn> &builtinTable()
{
    static const QList<BuiltIn> t = {
        {{"Plain Text", {"txt", "log", "text", "out", "nfo"}, {"LICENSE", "COPYING", "AUTHORS", "TODO", "NEWS"}, {}, false, {}}, {}},

        // Programming
        {{"C", {"c", "h"}, {}, "//", false, P}, make<QsciLexerCPP>()},
        {{"C++", {"cpp", "cc", "cxx", "c++", "hpp", "hh", "hxx", "h++", "ino", "tpp", "ipp"}, {}, "//", false, P}, make<QsciLexerCPP>()},
        {{"C#", {"cs", "csx"}, {}, "//", false, P}, make<QsciLexerCSharp>()},
        {{"D", {"d", "di"}, {}, "//", false, P}, make<QsciLexerD>()},
        {{"Dart", {"dart"}, {}, "//", false, P}, cfamily("Dart", kDart, kDartT, false, true)},
        {{"Fortran", {"f90", "f95", "f03", "f08"}, {}, "!", false, P}, make<QsciLexerFortran>()},
        {{"Fortran 77", {"f", "for", "f77", "ftn"}, {}, "!", false, P}, make<QsciLexerFortran77>()},
        {{"Go", {"go"}, {}, "//", false, P}, cfamily("Go", kGo, kGoT, true)},
        {{"Java", {"java"}, {}, "//", false, P}, make<QsciLexerJava>()},
        {{"Kotlin", {"kt", "kts"}, {}, "//", false, P}, cfamily("Kotlin", kKotlin, kKotlinT, false, true)},
        {{"MATLAB", {"m"}, {}, "%", false, P}, make<QsciLexerMatlab>()},
        {{"Objective-C", {"mm"}, {}, "//", false, P}, cfamily("Objective-C", kObjC, kObjCT)},
        {{"Octave", {}, {}, "%", false, P}, make<QsciLexerOctave>()},
        {{"Pascal", {"pas", "pp", "dpr", "lpr", "dpk"}, {}, "//", false, P}, make<QsciLexerPascal>()},
        {{"PostScript", {"ps", "eps"}, {}, "%", false, P}, make<QsciLexerPostScript>()},
        {{"POV-Ray", {"pov"}, {}, "//", false, P}, make<QsciLexerPOV>()},
        {{"Rust", {"rs"}, {}, "//", false, P}, cfamily("Rust", kRust, kRustT)},
        {{"Scala", {"scala", "sc"}, {}, "//", false, P}, cfamily("Scala", kScala, kScalaT, false, true)},
        {{"SQL", {"sql"}, {}, "--", false, P}, make<QsciLexerSQL>()},
        {{"QB64 / QBasic", {"bas", "bi", "bm", "qb64"}, {}, "'", false, P}, make<QB64Lexer>()},
        {{"Swift", {"swift"}, {}, "//", false, P}, cfamily("Swift", kSwift, kSwiftT, false, true)},

        // Scripting
        {{"AviSynth", {"avs", "avsi"}, {}, "#", false, S}, make<QsciLexerAVS>()},
        {{"Bash", {"sh", "bash", "zsh", "ksh"}, {".bashrc", ".bash_profile", ".bash_aliases", ".profile", ".zshrc", "PKGBUILD"}, "#", false, S}, make<QsciLexerBash>()},
        {{"Batch", {"bat", "cmd", "nt"}, {}, "REM", false, S}, make<QsciLexerBatch>()},
        {{"CoffeeScript", {"coffee", "litcoffee"}, {"Cakefile"}, "#", false, S}, make<QsciLexerCoffeeScript>()},
        {{"Groovy", {"groovy", "gvy", "gradle"}, {"Jenkinsfile"}, "//", false, S}, cfamily("Groovy", kGroovy, kGroovyT, false, true)},
        {{"JavaScript", {"js", "mjs", "cjs", "jsx"}, {}, "//", false, S}, make<QsciLexerJavaScript>()},
        {{"Lua", {"lua", "rockspec"}, {}, "--", false, S}, make<QsciLexerLua>()},
        {{"Perl", {"pl", "pm", "pod", "perl"}, {}, "#", false, S}, make<QsciLexerPerl>()},
        {{"PHP", {"php", "phtml", "php3", "php4", "php5", "php7", "php8", "phps"}, {}, "//", false, S}, make<QsciLexerHTML>()},
        {{"Python", {"py", "pyw", "pyi"}, {"SConstruct", "SConscript"}, "#", false, S}, make<QsciLexerPython>()},
        {{"Ruby", {"rb", "rbw", "rake", "gemspec", "ru"}, {"Rakefile", "Gemfile", "Vagrantfile", "Podfile", "Guardfile"}, "#", false, S}, make<QsciLexerRuby>()},
        {{"Tcl", {"tcl", "tk", "itcl", "exp"}, {}, "#", false, S}, make<QsciLexerTCL>()},
        {{"TypeScript", {"ts", "tsx", "mts", "cts"}, {}, "//", false, S}, cfamily("TypeScript", kTS, kTST)},

        // Markup & data
        {{"CSS", {"css", "scss", "less"}, {}, {}, false, M}, make<QsciLexerCSS>()},
        {{"EDIFACT", {"edi", "edifact"}, {}, {}, false, M}, make<QsciLexerEDIFACT>()},
        {{"Gettext PO", {"po", "pot"}, {}, "#", false, M}, make<QsciLexerPO>()},
        {{"HTML", {"html", "htm", "xhtml", "shtml", "vue", "svelte", "asp", "aspx", "jsp"}, {}, {}, false, M}, make<QsciLexerHTML>()},
        {{"IDL", {"idl", "odl"}, {}, "//", false, M}, make<QsciLexerIDL>()},
        {{"JSON", {"json", "jsonc", "geojson", "webmanifest"}, {".prettierrc", ".eslintrc"}, {}, false, M}, make<QsciLexerJSON>()},
        {{"Markdown", {"md", "markdown", "mkd"}, {"README"}, {}, false, M}, make<QsciLexerMarkdown>()},
        {{"Protocol Buffers", {"proto"}, {}, "//", false, M}, cfamily("Protocol Buffers", kProto, kProtoT)},
        {{"TeX / LaTeX", {"tex", "ltx", "sty", "cls", "dtx", "bib"}, {}, "%", false, M}, make<QsciLexerTeX>()},
        {{"XML", {"xml", "svg", "ui", "xsd", "xsl", "xslt", "plist", "qrc", "xaml", "csproj", "rss", "atom"}, {}, {}, false, M}, make<QsciLexerXML>()},
        {{"YAML", {"yml", "yaml"}, {".clang-format", ".clang-tidy"}, "#", false, M}, make<QsciLexerYAML>()},

        // Build & config
        {{"CMake", {"cmake"}, {"CMakeLists.txt"}, "#", false, B}, make<QsciLexerCMake>()},
        {{"Diff", {"diff", "patch", "rej"}, {}, {}, false, B}, make<QsciLexerDiff>()},
        {{"Dockerfile", {"dockerfile"}, {"Dockerfile", "Containerfile"}, "#", false, B}, make<QsciLexerBash>()},
        {{"INI / Config", {"ini", "conf", "cfg", "desktop", "properties", "service", "env", "editorconfig"}, {".env", ".gitconfig", ".editorconfig"}, ";", false, B}, make<QsciLexerProperties>()},
        {{"Makefile", {"mk", "mak"}, {"Makefile", "makefile", "GNUmakefile"}, "#", false, B}, make<QsciLexerMakefile>()},
        {{"TOML", {"toml"}, {"Cargo.lock", "Pipfile"}, "#", false, B}, make<QsciLexerProperties>()},

        // Hardware & low-level
        {{"Assembly (MASM)", {"masm"}, {}, ";", false, H}, make<QsciLexerMASM>()},
        {{"Assembly (NASM)", {"asm", "nasm", "s"}, {}, ";", false, H}, make<QsciLexerNASM>()},
        {{"Intel HEX", {"hex", "ihex", "ihx"}, {}, {}, false, H}, make<QsciLexerIntelHex>()},
        {{"Motorola S-Record", {"srec", "s19", "s28", "s37", "mot"}, {}, {}, false, H}, make<QsciLexerSRec>()},
        {{"SPICE", {"cir", "sp", "spice", "ckt"}, {}, "*", false, H}, make<QsciLexerSpice>()},
        {{"Tektronix HEX", {"tek"}, {}, {}, false, H}, make<QsciLexerTekHex>()},
        {{"Verilog", {"v", "vh", "sv", "svh"}, {}, "//", false, H}, make<QsciLexerVerilog>()},
        {{"VHDL", {"vhd", "vhdl"}, {}, "--", false, H}, make<QsciLexerVHDL>()},
    };
    return t;
}

} // namespace

QStringList categories() { return {P, S, M, B, H}; }

static const BuiltIn *builtin(const QString &name)
{
    for (const BuiltIn &b : builtinTable())
        if (b.lang.name == name) return &b;
    return nullptr;
}

static QList<UserLanguage> &userDefs()
{
    static QList<UserLanguage> defs = UserLanguage::loadAll();
    return defs;
}

static QList<Language> &table()
{
    static QList<Language> langs;
    if (langs.isEmpty()) {
        for (const BuiltIn &b : builtinTable())
            langs.append(b.lang);
        for (const UserLanguage &u : userDefs()) {
            if (builtin(u.name)) continue; // a built-in name wins
            langs.append({u.name, u.extensions, {}, u.lineComment, true, QStringLiteral("Your formats")});
        }
    }
    return langs;
}

const QList<Language> &all() { return table(); }

const Language *find(const QString &name)
{
    for (const Language &l : table())
        if (l.name == name) return &l;
    return nullptr;
}

void reloadUserLanguages()
{
    userDefs() = UserLanguage::loadAll();
    table().clear();
    (void)table();
}

static const UserLanguage *userDef(const QString &name)
{
    for (const UserLanguage &u : userDefs())
        if (u.name == name) return &u;
    return nullptr;
}

// Language named by a #! line, e.g. "#!/usr/bin/env python3".
static QString fromShebang(const QByteArray &head)
{
    // The program is the last path part of the first word, or the word after "env".
    QList<QByteArray> words = head.mid(2).trimmed().split(' ');
    words.removeAll(QByteArray());
    if (words.isEmpty()) return {};
    QByteArray prog = words.first().mid(words.first().lastIndexOf('/') + 1);
    if (prog == "env") {
        prog.clear();
        for (int i = 1; i < words.size(); ++i)
            if (!words[i].startsWith('-')) { prog = words[i]; break; }
    }
    static const struct { const char *prefix; const char *lang; } map[] = {
        {"python", "Python"}, {"pypy", "Python"}, {"node", "JavaScript"}, {"bun", "JavaScript"},
        {"deno", "TypeScript"}, {"ts-node", "TypeScript"}, {"php", "PHP"},
        {"perl", "Perl"}, {"ruby", "Ruby"}, {"lua", "Lua"}, {"tclsh", "Tcl"},
        {"wish", "Tcl"}, {"expect", "Tcl"}, {"octave", "Octave"}, {"coffee", "CoffeeScript"},
        {"groovy", "Groovy"}, {"kotlin", "Kotlin"}, {"swift", "Swift"}, {"dart", "Dart"},
        {"scala", "Scala"}, {"make", "Makefile"},
        {"bash", "Bash"}, {"sh", "Bash"}, {"zsh", "Bash"}, {"ksh", "Bash"}, {"dash", "Bash"}, {"ash", "Bash"},
    };
    for (const auto &m : map)
        if (prog.startsWith(m.prefix)) return QString::fromLatin1(m.lang);
    if (prog.endsWith("sh")) return QStringLiteral("Bash"); // some other shell
    return {};
}

QString detect(const QString &filePath, const QByteArray &firstLine)
{
    const QFileInfo fi(filePath);
    const QString base = fi.fileName();
    const QString ext = fi.suffix().toLower();
    const QByteArray head = firstLine.trimmed();
    const QByteArray lower = head.toLower();

    for (const Language &l : all())
        if (l.filenames.contains(base))
            return l.name;
    if (base.startsWith("Dockerfile.") || base.startsWith("Containerfile."))
        return QStringLiteral("Dockerfile");
    if (!ext.isEmpty()) {
        // Your own formats first, so they can claim an extension.
        for (const Language &l : all())
            if (l.userDefined && l.extensions.contains(ext))
                return l.name;
        // .m is MATLAB, unless it looks like Objective-C.
        if (ext == "m" && (head.startsWith("#import") || head.startsWith("#include") || head.startsWith("@")))
            return QStringLiteral("Objective-C");
        for (const Language &l : all())
            if (l.extensions.contains(ext))
                return l.name;
    }

    // Look at the content.
    if (head.startsWith("#!")) {
        const QString lang = fromShebang(head);
        if (!lang.isEmpty()) return lang;
    }
    if (head.startsWith("<?php") || head.startsWith("<?=")) return QStringLiteral("PHP");
    if (head.startsWith("<?xml")) return QStringLiteral("XML");
    if (lower.startsWith("<!doctype html") || lower.startsWith("<html")) return QStringLiteral("HTML");
    if (lower.contains("<?php")) return QStringLiteral("PHP");
    if (head.startsWith("diff --git") || head.startsWith("--- ")) return QStringLiteral("Diff");
    if (head.startsWith("%!PS")) return QStringLiteral("PostScript");
    if (head.startsWith("\\documentclass")) return QStringLiteral("TeX / LaTeX");
    if (lower.startsWith("@echo off")) return QStringLiteral("Batch");

    // Files with no extension (and no clue inside) are plain text. An
    // extension nobody knows is "unknown", so the caller can ask.
    return ext.isEmpty() ? QStringLiteral("Plain Text") : QString();
}

QsciLexer *createLexer(const QString &name, QObject *parent)
{
    const BuiltIn *b = builtin(name);
    if (!b) {
        if (const UserLanguage *u = userDef(name))
            return new UserLexer(*u, parent); // keeps its own fonts (bold keywords)
        return nullptr;
    }
    if (!b->lexer)
        return nullptr;
    QsciLexer *lx = b->lexer(parent);

    // Same monospace font for every style, so bold keywords don't change width.
    QFont mono = Prefs::editorFont();
    lx->setDefaultFont(mono);
    lx->setFont(mono, -1);
    if (auto *cpp = qobject_cast<QsciLexerCPP *>(lx))
        cpp->setFoldPreprocessor(true);
    return lx;
}

}
