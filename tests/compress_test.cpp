// Tests for Save Compressed: every format round-trips, and passwords work.
#include "../src/compress.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryDir>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++failures; } } while (0)

// Extracts the archive with 7-Zip and returns the inner file's bytes.
static QByteArray extract(const QString &archive, const QString &inner, const QString &pw, bool *ok)
{
    QTemporaryDir out;
    QProcess p;
    QStringList args = {"x", "-y", "-bd", "-p" + pw, "-o" + out.path(), archive};
    p.start(Compress::findSevenZip(), args);
    p.closeWriteChannel();
    p.waitForFinished(60000);
    *ok = p.exitCode() == 0;
    QFile f(out.filePath(inner));
    if (!f.open(QIODevice::ReadOnly)) {
        // gzip/bzip2 name the output after the archive, minus the extension
        QFile g(out.filePath(QFileInfo(archive).completeBaseName()));
        if (!g.open(QIODevice::ReadOnly)) { *ok = false; return {}; }
        return g.readAll();
    }
    return f.readAll();
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    if (Compress::findSevenZip().isEmpty()) {
        std::printf("7-Zip not installed; skipping compress tests\n");
        return 0;
    }
    QTemporaryDir dir;
    const QByteArray text = "hello blitzpad\nline 2 \xc3\xa9\n";
    QString err;

    for (const auto &fi : Compress::formats()) {
        const QString path = dir.filePath("doc.txt" + fi.extension);
        CHECK(Compress::save(fi.format, path, "doc.txt", text, {}, &err));
        bool ok = false;
        CHECK(extract(path, "doc.txt", "", &ok) == text);
        CHECK(ok);
        // Saving again replaces the archive rather than adding to it.
        CHECK(Compress::save(fi.format, path, "doc.txt", "v2", {}, &err));
        CHECK(extract(path, "doc.txt", "", &ok) == "v2");
        if (!fi.supportsPassword)
            CHECK(!Compress::save(fi.format, path, "doc.txt", text, "pw", &err));
    }

    for (auto f : {Compress::Format::Zip, Compress::Format::SevenZip}) {
        const QString path = dir.filePath(QString("secret") + Compress::info(f).extension);
        CHECK(Compress::save(f, path, "doc.txt", text, "s3cret pass", &err));
        bool ok = false;
        CHECK(extract(path, "doc.txt", "s3cret pass", &ok) == text && ok);
        extract(path, "doc.txt", "wrong", &ok);
        CHECK(!ok);
    }

    std::printf(failures ? "%d failure(s)\n" : "all compress tests passed\n", failures);
    return failures ? 1 : 0;
}
