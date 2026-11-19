#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

// "Save Compressed": writes one document into an archive using the 7-Zip
// command-line tool (7z / 7zz / 7za), which handles every format below.
namespace Compress {

enum class Format { Zip, SevenZip, GZip, BZip2, Tar };

struct FormatInfo {
    Format format;
    QString label;      // shown in the file dialog
    QString extension;  // with dot, e.g. ".tar"
    QString sevenZipType;
    bool supportsPassword;
};

const QList<FormatInfo> &formats();
const FormatInfo &info(Format f);

// The 7-Zip executable to use, or empty if none is installed.
QString findSevenZip();

// Writes `data` as a file called `innerName` inside a new archive at
// `archivePath` (replacing any existing file). An empty password means none.
// The password goes to 7-Zip on stdin, never on the command line.
bool save(Format f, const QString &archivePath, const QString &innerName,
          const QByteArray &data, const QString &password, QString *error);

}
