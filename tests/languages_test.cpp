// Tests for language detection and that every built-in language has a lexer.
#include "../src/languages.h"

#include <QApplication>
#include <QTemporaryDir>
#include <Qsci/qscilexer.h>
#include <cstdio>

static int failures = 0;
static void expect(const char *path, const char *firstLine, const char *want)
{
    const QString got = Languages::detect(path, firstLine);
    if (got != QLatin1String(want)) {
        std::printf("FAIL detect(%s, \"%s\"): got \"%s\", want \"%s\"\n", path, firstLine,
                    qPrintable(got), want);
        ++failures;
    }
}

int main(int argc, char **argv)
{
    QTemporaryDir home; // keep ~/.blitzpad out of the real home folder
    qputenv("HOME", home.path().toUtf8());
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    // By extension
    expect("/x/index.php", "", "PHP");
    expect("/x/view.phtml", "", "PHP");
    expect("/x/a.c", "", "C");
    expect("/x/a.HPP", "", "C++");
    expect("/x/a.cs", "", "C#");
    expect("/x/main.go", "", "Go");
    expect("/x/lib.rs", "", "Rust");
    expect("/x/app.ts", "", "TypeScript");
    expect("/x/app.js", "", "JavaScript");
    expect("/x/a.rb", "", "Ruby");
    expect("/x/a.pl", "", "Perl");
    expect("/x/a.lua", "", "Lua");
    expect("/x/a.tex", "", "TeX / LaTeX");
    expect("/x/a.bat", "", "Batch");
    expect("/x/a.vhd", "", "VHDL");
    expect("/x/a.sv", "", "Verilog");
    expect("/x/a.asm", "", "Assembly (NASM)");
    expect("/x/a.toml", "", "TOML");
    expect("/x/a.f90", "", "Fortran");
    expect("/x/a.m", "% comment", "MATLAB");
    expect("/x/a.m", "#import <Foundation/Foundation.h>", "Objective-C");
    // By file name
    expect("/x/Dockerfile", "", "Dockerfile");
    expect("/x/Gemfile", "", "Ruby");
    expect("/x/CMakeLists.txt", "", "CMake");
    // By content
    expect("/x/script", "<?php", "PHP");
    expect("/x/script", "#!/usr/bin/php", "PHP");
    expect("/x/script", "#!/usr/bin/env php", "PHP");
    expect("/x/script", "#!/usr/bin/env python3", "Python");
    expect("/x/script", "#!/usr/bin/perl -w", "Perl");
    expect("/x/script", "#!/usr/bin/env ruby", "Ruby");
    expect("/x/script", "#!/usr/bin/env lua5.4", "Lua");
    expect("/x/script", "#!/bin/bash", "Bash");
    expect("/x/script", "#!/usr/bin/env -S node --no-warnings", "JavaScript");
    expect("/x/script", "#!/usr/bin/tclsh", "Tcl");
    expect("/x/script", "", "Plain Text");
    expect("/x/thing.qqq", "", ""); // unknown extension: the caller asks

    // Every built-in language except Plain Text has a lexer and a category.
    int count = 0;
    for (const Language &l : Languages::all()) {
        if (l.userDefined) continue;
        ++count;
        QsciLexer *lx = Languages::createLexer(l.name, nullptr);
        if (l.name == "Plain Text") { if (lx) { std::printf("FAIL Plain Text has a lexer\n"); ++failures; } continue; }
        if (!lx) { std::printf("FAIL no lexer for %s\n", qPrintable(l.name)); ++failures; }
        if (!Languages::categories().contains(l.category)) { std::printf("FAIL no category for %s\n", qPrintable(l.name)); ++failures; }
        delete lx;
    }
    std::printf("%d built-in languages checked\n", count);
    if (failures) std::printf("%d failure(s)\n", failures);
    else std::printf("All language tests passed.\n");
    return failures ? 1 : 0;
}
