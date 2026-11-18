#include "languages.h"
#include "prefs.h"

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

const QList<Language> &all()
{
    static const QList<Language> langs = {
        {"Plain Text", {"txt", "log", "text"}, {}},
        {"C", {"c", "h"}, {}},
        {"C++", {"cpp", "cc", "cxx", "c++", "hpp", "hh", "hxx", "h++", "ino"}, {}},
        {"Bash", {"sh", "bash", "zsh", "ksh"}, {".bashrc", ".bash_profile", ".profile", ".zshrc", "PKGBUILD"}},
        {"JavaScript", {"js", "mjs", "cjs", "jsx", "ts", "tsx"}, {}},
        {"JSON", {"json"}, {}},
        {"Python", {"py", "pyw"}, {}},
        {"HTML", {"html", "htm", "xhtml"}, {}},
        {"CSS", {"css"}, {}},
        {"XML", {"xml", "svg", "ui", "xsd", "xsl"}, {}},
        {"Markdown", {"md", "markdown"}, {"README"}},
        {"Makefile", {"mk", "mak"}, {"Makefile", "makefile", "GNUmakefile"}},
        {"CMake", {"cmake"}, {"CMakeLists.txt"}},
        {"YAML", {"yml", "yaml"}, {}},
        {"INI / Config", {"ini", "conf", "cfg", "desktop", "properties", "service"}, {}},
        {"Diff", {"diff", "patch"}, {}},
        {"SQL", {"sql"}, {}},
        {"Java", {"java"}, {}},
    };
    return langs;
}

QString detect(const QString &filePath, const QByteArray &firstLine)
{
    const QFileInfo fi(filePath);
    const QString base = fi.fileName();
    const QString ext = fi.suffix().toLower();

    for (const Language &l : all())
        if (l.filenames.contains(base))
            return l.name;
    if (!ext.isEmpty())
        for (const Language &l : all())
            if (l.extensions.contains(ext))
                return l.name;

    if (firstLine.startsWith("#!")) {
        if (firstLine.contains("sh")) // sh, bash, zsh, dash...
            return firstLine.contains("python") ? "Python" : "Bash";
        if (firstLine.contains("python"))
            return "Python";
        if (firstLine.contains("node"))
            return "JavaScript";
    }
    return "Plain Text";
}

QsciLexer *createLexer(const QString &name, QObject *parent)
{
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
