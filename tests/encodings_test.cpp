// Tests for code page conversion: CP437 and friends round-trip, detection, lost characters.
#include "../src/encodings.h"

#include <QStringDecoder>
#include <cstdio>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++failures; } } while (0)

int main()
{
    // CP437: box drawing, accents and symbols from a DOS screen.
    static const char dosBytes[] = "\xC9\xCD\xBB \x82t\x82 \x9C" "5 \xE1\xF8 \xB0\xB1\xB2\xDB";
    const QByteArray dos(dosBytes, sizeof(dosBytes) - 1);
    const QByteArray utf8 = Encodings::toUtf8(dos, "CP437");
    CHECK(QString::fromUtf8(utf8) == QString::fromUtf8("╔═╗ été £5 ß° ░▒▓█"));
    int lost = -1;
    CHECK(Encodings::fromUtf8(utf8, "CP437", &lost) == dos);
    CHECK(lost == 0);
    // A character CP437 doesn't have becomes '?', and is counted.
    CHECK(Encodings::fromUtf8(QString::fromUtf8("a€b").toUtf8(), "CP437", &lost) == "a?b");
    CHECK(lost == 1);
    // Every single-byte code page round-trips all 256 bytes.
    QByteArray all256;
    for (int b = 0; b < 256; ++b) all256.append(char(b));
    for (const Encodings::Info &i : Encodings::all()) {
        if (i.group == "Unicode" || i.group == "East Asian") continue;
        const QByteArray back = Encodings::fromUtf8(Encodings::toUtf8(all256, i.id), i.id, &lost);
        if (back != all256) {
            int bad = 0;
            for (int b = 0; b < 256; ++b) if (back[b] != all256[b]) ++bad;
            std::printf("FAIL %s: %d bytes don't round-trip\n", qPrintable(i.id), bad);
            ++failures;
        }
    }
    // Specific code pages.
    CHECK(QString::fromUtf8(Encodings::toUtf8("\x80\x81", "CP866")) == QString::fromUtf8("АБ"));
    CHECK(QString::fromUtf8(Encodings::toUtf8("\x80", "Windows-1252")) == QString::fromUtf8("€"));
    CHECK(QString::fromUtf8(Encodings::toUtf8("\xC1", "KOI8-R")) == QString::fromUtf8("а"));
    CHECK(QString::fromUtf8(Encodings::toUtf8("\xC1\xC2", "CP037")) == "AB"); // EBCDIC
    // Unicode with byte-order marks.
    CHECK(Encodings::detect("\xEF\xBB\xBFhi", "CP437") == "UTF-8 BOM");
    CHECK(Encodings::toUtf8("\xEF\xBB\xBFhi", "UTF-8 BOM") == "hi");
    CHECK(Encodings::fromUtf8("hi", "UTF-8 BOM") == "\xEF\xBB\xBFhi");
    const QByteArray u16 = Encodings::fromUtf8(QString::fromUtf8("é").toUtf8(), "UTF-16 LE");
    CHECK(u16 == QByteArray("\xFF\xFE\xE9\x00", 4));
    CHECK(Encodings::detect(u16, "CP437") == "UTF-16 LE");
    CHECK(QString::fromUtf8(Encodings::toUtf8(u16, "UTF-16 LE")) == QString::fromUtf8("é"));
    // Detection: valid UTF-8 stays UTF-8; anything else uses the fallback.
    CHECK(Encodings::detect(QString::fromUtf8("ünïcode").toUtf8(), "CP437") == "UTF-8");
    CHECK(Encodings::detect(dos, "CP437") == "CP437");
    CHECK(Encodings::detect(dos, "ibm437") == "CP437"); // alias
    CHECK(Encodings::detect(dos, "nonsense") == "ISO-8859-1");
    // East Asian encodings, when this Qt has them.
    if (Encodings::find("Shift_JIS")) {
        const QByteArray sjis = Encodings::fromUtf8(QString::fromUtf8("日本").toUtf8(), "Shift_JIS", &lost);
        CHECK(sjis == QByteArray("\x93\xFA\x96\x7B", 4));
        CHECK(lost == 0);
        CHECK(QString::fromUtf8(Encodings::toUtf8(sjis, "Shift_JIS")) == QString::fromUtf8("日本"));
    }
    std::printf("%d encodings available\n", int(Encodings::all().size()));
    if (failures) std::printf("%d failure(s)\n", failures);
    else std::printf("All encoding tests passed.\n");
    return failures ? 1 : 0;
}
