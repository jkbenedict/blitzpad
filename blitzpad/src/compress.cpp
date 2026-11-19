#include "compress.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>

namespace Compress {

const QList<FormatInfo> &formats()
{
    static const QList<FormatInfo> list = {
        {Format::Zip,      "ZIP archive",           ".zip", "zip",   true},
        {Format::SevenZip, "7-Zip archive",         ".7z",  "7z",    true},
        {Format::GZip,     "GZip compressed file",  ".gz",  "gzip",  false},
        {Format::BZip2,    "BZip2 compressed file", ".bz2", "bzip2", false},
        {Format::Tar,      "TAR archive",           ".tar", "tar",   false},
    };
    return list;
}

const FormatInfo &info(Format f)
{
    for (const FormatInfo &i : formats())
        if (i.format == f)
            return i;
    return formats().first();
}

QString findSevenZip()
{
    for (const char *name : {"7z", "7zz", "7za"}) {
        const QString p = QStandardPaths::findExecutable(QString::fromLatin1(name));
        if (!p.isEmpty())
            return p;
    }
    return {};
}

bool save(Format f, const QString &archivePath, const QString &innerName,
          const QByteArray &data, const QString &password, QString *error)
{
    const QString exe = findSevenZip();
    if (exe.isEmpty()) {
        *error = QObject::tr("7-Zip is not installed. Install it with your package manager "
                             "(p7zip-full on Zorin/Ubuntu/Debian, p7zip on Fedora, 7zip on Arch) and try again.");
        return false;
    }
    const FormatInfo &fi = info(f);
    if (!password.isEmpty() && !fi.supportsPassword) {
        *error = QObject::tr("%1 does not support passwords.").arg(fi.label);
        return false;
    }

    QTemporaryDir tmp;
    if (!tmp.isValid()) {
        *error = QObject::tr("Cannot create a temporary folder.");
        return false;
    }
    // Work in a private temp folder, then copy the finished archive into
    // place, so a failure never leaves a half-written file at the target
    // and 7-Zip never appends to an existing archive.
    const QString name = innerName.isEmpty() ? QStringLiteral("document.txt") : innerName;
    QFile in(tmp.filePath(name));
    if (!in.open(QIODevice::WriteOnly) || in.write(data) != data.size()) {
        *error = in.errorString();
        return false;
    }
    in.close();

    const QString out = tmp.filePath("archive" + fi.extension);
    QStringList args = {"a", "-t" + fi.sevenZipType, "-y", "-bd"};
    if (!password.isEmpty()) {
        args << "-p"; // read from stdin
        if (f == Format::Zip)
            args << "-mem=AES256";
        else if (f == Format::SevenZip)
            args << "-mhe=on"; // also hide file names
    }
    args << out << name;

    QProcess p;
    p.setWorkingDirectory(tmp.path());
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(exe, args);
    if (!p.waitForStarted(10000)) {
        *error = QObject::tr("Could not start %1.").arg(exe);
        return false;
    }
    if (!password.isEmpty()) {
        const QByteArray pw = password.toUtf8();
        p.write(pw + "\n" + pw + "\n"); // some 7-Zip versions ask to confirm
    }
    p.closeWriteChannel();
    if (!p.waitForFinished(5 * 60 * 1000) || p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        *error = QObject::tr("7-Zip failed:\n%1").arg(QString::fromLocal8Bit(p.readAll()).trimmed().right(800));
        return false;
    }

    if (QFileInfo::exists(archivePath) && !QFile::remove(archivePath)) {
        *error = QObject::tr("Cannot replace %1.").arg(archivePath);
        return false;
    }
    if (!QFile::copy(out, archivePath)) {
        *error = QObject::tr("Cannot write %1.").arg(archivePath);
        return false;
    }
    return true;
}

}
