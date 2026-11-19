#include "languages.h"
#include "prefs.h"
#include "userlang.h"

#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>

#include <Qsci/qscilexerbash.h>
#include <Qsci/qscilexercmake.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qscilexercss.h>
#include <Qsci/qscilexerdiff.h>
#include <Qsci/qscilexerhtml.h>
#include <Qsci/qscilexerjava.h>
#include <Qsci/qscilexerjavascript.h>
#include <Qsci/qscilexerjson.h>
#include <Qsci/qscilexermakefile.h>
#include <Qsci/qscilexermarkdown.h>
#include <Qsci/qscilexerproperties.h>
#include <Qsci/qscilexerpython.h>
#include <Qsci/qscilexersql.h>
#include <Qsci/qscilexerxml.h>
#include <Qsci/qscilexeryaml.h>

namespace Languages {

static QList<Language> builtins()
{
    return {
        {"Plain Text", {"txt", "log", "text", "out", "nfo"}, {"LICENSE", "COPYING", "AUTHORS", "TODO", "NEWS"}, {}},
        {"C", {"c", "h"}, {}, "//"},
        {"C++", {"cpp", "cc", "cxx", "c++", "hpp", "hh", "hxx", "h++", "ino"}, {}, "//"},
        {"Bash", {"sh", "bash", "zsh", "ksh"}, {".bashrc", ".bash_profile", ".profile", ".zshrc", "PKGBUILD"}, "#"},
        {"JavaScript", {"js", "mjs", "cjs", "jsx", "ts", "tsx"}, {}, "//"},
        {"JSON", {"json"}, {}, {}},
        {"Python", {"py", "pyw"}, {}, "#"},
        {"HTML", {"html", "htm", "xhtml"}, {}, {}},
        {"CSS", {"css"}, {}, {}},
        {"XML", {"xml", "svg", "ui", "xsd", "xsl"}, {}, {}},
        {"Markdown", {"md", "markdown"}, {"README"}, {}},
        {"Makefile", {"mk", "mak"}, {"Makefile", "makefile", "GNUmakefile"}, "#"},
        {"CMake", {"cmake"}, {"CMakeLists.txt"}, "#"},
        {"YAML", {"yml", "yaml"}, {}, "#"},
        {"INI / Config", {"ini", "conf", "cfg", "desktop", "properties", "service"}, {}, ";"},
        {"Diff", {"diff", "patch"}, {}, {}},
        {"SQL", {"sql"}, {}, "--"},
        {"Java", {"java"}, {}, "//"},
    };
}

static bool builtinsHave(const QString &name)
{
    for (const Language &l : builtins())
        if (l.name == name) return true;
    return false;
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
        langs = builtins();
        for (const UserLanguage &u : userDefs()) {
            if (builtinsHave(u.name)) continue; // a built-in name wins
            langs.append({u.name, u.extensions, {}, u.lineComment, true});
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

QString detect(const QString &filePath, const QByteArray &firstLine)
{
    const QFileInfo fi(filePath);
    const QString base = fi.fileName();
    const QString ext = fi.suffix().toLower();

    for (const Language &l : all())
        if (l.filenames.contains(base))
            return l.name;
    if (!ext.isEmpty()) {
        // Your own formats first, so they can claim an extension.
        for (const Language &l : all())
            if (l.userDefined && l.extensions.contains(ext))
                return l.name;
        for (const Language &l : all())
            if (l.extensions.contains(ext))
                return l.name;
    }

    // Look at the content.
    const QByteArray head = firstLine.trimmed();
    if (head.startsWith("#!")) {
        if (head.contains("python")) return "Python";
        if (head.contains("node")) return "JavaScript";
        if (head.contains("sh")) return "Bash"; // sh, bash, zsh, dash...
    }
    if (head.startsWith("<?xml")) return "XML";
    if (head.startsWith("<!DOCTYPE html") || head.startsWith("<!doctype html") || head.startsWith("<html")) return "HTML";
    if (head.startsWith("diff --git") || head.startsWith("--- ")) return "Diff";

    // Files with no extension (and no clue inside) are plain text. An
    // extension nobody knows is "unknown", so the caller can ask.
    return ext.isEmpty() ? QStringLiteral("Plain Text") : QString();
}

QsciLexer *createLexer(const QString &name, QObject *parent)
{
    if (const UserLanguage *u = userDef(name); u && !builtinsHave(name))
        return new UserLexer(*u, parent); // keeps its own fonts (bold keywords)
    QsciLexer *lx = nullptr;
    if (name == "C" || name == "C++")      lx = new QsciLexerCPP(parent);
    else if (name == "Bash")               lx = new QsciLexerBash(parent);
    else if (name == "JavaScript")         lx = new QsciLexerJavaScript(parent);
    else if (name == "JSON")               lx = new QsciLexerJSON(parent);
    else if (name == "Python")             lx = new QsciLexerPython(parent);
    else if (name == "HTML")               lx = new QsciLexerHTML(parent);
    else if (name == "CSS")                lx = new QsciLexerCSS(parent);
    else if (name == "XML")                lx = new QsciLexerXML(parent);
    else if (name == "Markdown")           lx = new QsciLexerMarkdown(parent);
    else if (name == "Makefile")           lx = new QsciLexerMakefile(parent);
    else if (name == "CMake")              lx = new QsciLexerCMake(parent);
    else if (name == "YAML")               lx = new QsciLexerYAML(parent);
    else if (name == "INI / Config")       lx = new QsciLexerProperties(parent);
    else if (name == "Diff")               lx = new QsciLexerDiff(parent);
    else if (name == "SQL")                lx = new QsciLexerSQL(parent);
    else if (name == "Java")               lx = new QsciLexerJava(parent);
    if (!lx)
        return nullptr;

    // Same monospace font for every style, so bold keywords don't change width.
    QFont mono = Prefs::editorFont();
    lx->setDefaultFont(mono);
    lx->setFont(mono, -1);
    if (auto *cpp = qobject_cast<QsciLexerCPP *>(lx))
        cpp->setFoldPreprocessor(true);
    return lx;
}

}
