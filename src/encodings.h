#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

// Character encodings BlitzPad can open and save: Unicode, DOS / terminal code
// pages (CP437 and friends), Windows and ISO-8859 code pages, and East Asian
// multi-byte encodings where the system's Qt has them (through ICU).
//
// The editor works in UTF-8; these convert file bytes to and from it.
namespace Encodings {

struct Info {
    QString id;      // stored name, e.g. "CP437"
    QString display; // menu text, e.g. "CP437 - DOS United States (OEM-US)"
    QString group;   // menu group, e.g. "DOS / Terminal"
};

const QList<Info> &all();
QStringList groups();          // in menu order
const Info *find(const QString &id); // case-insensitive; also accepts aliases like "IBM437"

// Picks the encoding of file bytes: a byte-order mark wins, then valid UTF-8,
// otherwise `fallback`.
QString detect(const QByteArray &data, const QString &fallback);

// File bytes in `id` -> UTF-8 for the editor (a byte-order mark is dropped).
QByteArray toUtf8(const QByteArray &data, const QString &id);

// UTF-8 from the editor -> file bytes in `id` (a byte-order mark is added for
// "UTF-8 BOM" and UTF-16). Characters the encoding can't hold become '?', and
// *lost counts them.
QByteArray fromUtf8(const QByteArray &utf8, const QString &id, int *lost = nullptr);

}
