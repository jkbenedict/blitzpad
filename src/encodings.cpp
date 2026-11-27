#include "encodings.h"

#include <QHash>
#include <QStringDecoder>
#include <QStringEncoder>

#include <array>

namespace Encodings {

namespace {

struct Table {
    const char *id;
    const char *display;
    const char *group;
    std::array<char16_t, 256> map; // byte -> Unicode
};

// Single-byte code pages are built in, so they work on every system.
const Table kTables[] = {
#include "codepage_tables.inc"
};

const QString kUnicode = QStringLiteral("Unicode");
const QString kAsian = QStringLiteral("East Asian");

// Multi-byte encodings come from Qt's ICU support; shown only if available.
const struct { const char *id; const char *display; } kAsianList[] = {
    {"Shift_JIS", "Shift_JIS - Japanese"},
    {"EUC-JP", "EUC-JP - Japanese (Unix)"},
    {"GBK", "GBK - Simplified Chinese"},
    {"GB18030", "GB18030 - Chinese (all of Unicode)"},
    {"Big5", "Big5 - Traditional Chinese"},
    {"EUC-KR", "EUC-KR - Korean"},
};

const Table *table(const QString &id)
{
    for (const Table &t : kTables)
        if (id.compare(QLatin1String(t.id), Qt::CaseInsensitive) == 0) return &t;
    return nullptr;
}

const QHash<char16_t, uchar> &reverse(const Table *t)
{
    static QHash<const Table *, QHash<char16_t, uchar>> cache;
    auto it = cache.find(t);
    if (it == cache.end()) {
        QHash<char16_t, uchar> r;
        for (int b = 255; b >= 0; --b) // lowest byte wins for duplicates
            if (t->map[b] != 0xFFFD) r.insert(t->map[b], uchar(b));
        it = cache.insert(t, r);
    }
    return *it;
}

bool startsWith(const QByteArray &d, std::initializer_list<uchar> bom)
{
    if (d.size() < int(bom.size())) return false;
    int i = 0;
    for (uchar c : bom) if (uchar(d[i++]) != c) return false;
    return true;
}

} // namespace

const QList<Info> &all()
{
    static const QList<Info> list = [] {
        QList<Info> l = {
            {"UTF-8", "UTF-8", kUnicode},
            {"UTF-8 BOM", "UTF-8 with BOM", kUnicode},
            {"UTF-16 LE", "UTF-16 Little Endian (with BOM)", kUnicode},
            {"UTF-16 BE", "UTF-16 Big Endian (with BOM)", kUnicode},
        };
        for (const Table &t : kTables)
            l.append({QString::fromLatin1(t.id), QString::fromLatin1(t.display), QString::fromLatin1(t.group)});
        for (const auto &a : kAsianList)
            if (QStringDecoder(a.id).isValid())
                l.append({QString::fromLatin1(a.id), QString::fromLatin1(a.display), kAsian});
        return l;
    }();
    return list;
}

QStringList groups()
{
    return {kUnicode, "DOS / Terminal", "Windows", "ISO 8859", kAsian, "Other"};
}

const Info *find(const QString &idIn)
{
    QString id = idIn.trimmed();
    // Common other spellings.
    static const QHash<QString, QString> aliases = {
        {"ibm437", "CP437"}, {"oem-us", "CP437"}, {"dos", "CP437"}, {"ibm850", "CP850"},
        {"ibm866", "CP866"}, {"latin1", "ISO-8859-1"}, {"latin-1", "ISO-8859-1"},
        {"iso8859-1", "ISO-8859-1"}, {"cp1252", "Windows-1252"}, {"cp1251", "Windows-1251"},
        {"cp1250", "Windows-1250"}, {"utf8", "UTF-8"}, {"macintosh", "Mac-Roman"},
        {"sjis", "Shift_JIS"},
    };
    id = aliases.value(id.toLower(), id);
    for (const Info &i : all())
        if (i.id.compare(id, Qt::CaseInsensitive) == 0) return &i;
    return nullptr;
}

QString detect(const QByteArray &data, const QString &fallback)
{
    if (startsWith(data, {0xEF, 0xBB, 0xBF})) return QStringLiteral("UTF-8 BOM");
    if (startsWith(data, {0xFF, 0xFE})) return QStringLiteral("UTF-16 LE");
    if (startsWith(data, {0xFE, 0xFF})) return QStringLiteral("UTF-16 BE");
    QStringDecoder check(QStringDecoder::Utf8, QStringDecoder::Flag::Stateless);
    const QString decoded = check.decode(data); // the conversion runs here
    Q_UNUSED(decoded);
    if (!check.hasError()) return QStringLiteral("UTF-8");
    const Info *f = find(fallback);
    return f ? f->id : QStringLiteral("ISO-8859-1");
}

QByteArray toUtf8(const QByteArray &data, const QString &idIn)
{
    const Info *info = find(idIn);
    const QString id = info ? info->id : QStringLiteral("UTF-8");
    if (id == "UTF-8")
        return data; // Scintilla shows any bad bytes as-is
    if (id == "UTF-8 BOM")
        return startsWith(data, {0xEF, 0xBB, 0xBF}) ? data.mid(3) : data;
    if (id == "UTF-16 LE" || id == "UTF-16 BE") {
        QStringDecoder d(id == "UTF-16 LE" ? QStringDecoder::Utf16LE : QStringDecoder::Utf16BE,
                         QStringDecoder::Flag::Stateless); // no flag: keep the BOM out ourselves
        QByteArray body = data;
        if (startsWith(body, {0xFF, 0xFE}) || startsWith(body, {0xFE, 0xFF})) body = body.mid(2);
        return QString(d.decode(body)).toUtf8();
    }
    if (const Table *t = table(id)) {
        QString s(data.size(), Qt::Uninitialized);
        QChar *out = s.data();
        for (int i = 0; i < data.size(); ++i)
            out[i] = QChar(t->map[uchar(data[i])]);
        return s.toUtf8();
    }
    QStringDecoder d(id.toLatin1().constData());
    if (!d.isValid()) return QString::fromLatin1(data).toUtf8();
    return QString(d.decode(data)).toUtf8();
}

QByteArray fromUtf8(const QByteArray &utf8, const QString &idIn, int *lost)
{
    if (lost) *lost = 0;
    const Info *info = find(idIn);
    const QString id = info ? info->id : QStringLiteral("UTF-8");
    if (id == "UTF-8")
        return utf8;
    if (id == "UTF-8 BOM")
        return QByteArray("\xEF\xBB\xBF", 3) + utf8;
    const QString text = QString::fromUtf8(utf8);
    if (id == "UTF-16 LE" || id == "UTF-16 BE") {
        QStringEncoder e(id == "UTF-16 LE" ? QStringEncoder::Utf16LE : QStringEncoder::Utf16BE,
                         QStringEncoder::Flag::WriteBom);
        return e.encode(text);
    }
    if (const Table *t = table(id)) {
        const QHash<char16_t, uchar> &r = reverse(t);
        QByteArray out;
        out.reserve(text.size());
        for (int i = 0; i < text.size(); ++i) {
            const char16_t c = text[i].unicode();
            auto it = r.constFind(c);
            if (it != r.constEnd()) {
                out.append(char(*it));
            } else {
                out.append('?');
                if (lost) ++*lost;
                if (QChar::isHighSurrogate(c) && i + 1 < text.size()) ++i; // one '?' per character
            }
        }
        return out;
    }
    QStringEncoder e(id.toLatin1().constData());
    if (!e.isValid()) return text.toLatin1();
    QByteArray out = e.encode(text);
    if (lost) {
        // ICU swaps unmappable characters silently; count what doesn't come back.
        const QString back = QStringDecoder(id.toLatin1().constData()).decode(out);
        if (back != text) {
            const QList<uint> a = text.toUcs4(), b = back.toUcs4();
            if (a.size() == b.size()) { for (int i = 0; i < a.size(); ++i) if (a[i] != b[i]) ++*lost; }
            else *lost = qMax(1, int(qAbs(a.size() - b.size())));
        }
    }
    return out;
}

}
