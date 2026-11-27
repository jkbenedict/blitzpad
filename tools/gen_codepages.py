import codecs, sys
# (id, python codec, display name, group)
CPS = [
 ("CP437","cp437","CP437 - DOS United States (OEM-US)","DOS / Terminal"),
 ("CP737","cp737","CP737 - DOS Greek","DOS / Terminal"),
 ("CP775","cp775","CP775 - DOS Baltic","DOS / Terminal"),
 ("CP850","cp850","CP850 - DOS Western Europe","DOS / Terminal"),
 ("CP852","cp852","CP852 - DOS Central Europe","DOS / Terminal"),
 ("CP855","cp855","CP855 - DOS Cyrillic","DOS / Terminal"),
 ("CP857","cp857","CP857 - DOS Turkish","DOS / Terminal"),
 ("CP858","cp858","CP858 - DOS Western Europe with Euro","DOS / Terminal"),
 ("CP860","cp860","CP860 - DOS Portuguese","DOS / Terminal"),
 ("CP861","cp861","CP861 - DOS Icelandic","DOS / Terminal"),
 ("CP862","cp862","CP862 - DOS Hebrew","DOS / Terminal"),
 ("CP863","cp863","CP863 - DOS French Canadian","DOS / Terminal"),
 ("CP864","cp864","CP864 - DOS Arabic","DOS / Terminal"),
 ("CP865","cp865","CP865 - DOS Nordic","DOS / Terminal"),
 ("CP866","cp866","CP866 - DOS Russian","DOS / Terminal"),
 ("CP869","cp869","CP869 - DOS Modern Greek","DOS / Terminal"),
 ("CP720","cp720","CP720 - DOS Arabic (Transparent ASMO)","DOS / Terminal"),
 ("CP1125","cp1125","CP1125 - DOS Ukrainian","DOS / Terminal"),
 ("KOI8-R","koi8_r","KOI8-R - Russian (Unix terminals)","DOS / Terminal"),
 ("KOI8-U","koi8_u","KOI8-U - Ukrainian (Unix terminals)","DOS / Terminal"),
 ("CP037","cp037","CP037 - EBCDIC US/Canada (IBM mainframe)","DOS / Terminal"),
 ("CP500","cp500","CP500 - EBCDIC International (IBM mainframe)","DOS / Terminal"),
 ("Windows-874","cp874","Windows-874 - Thai","Windows"),
 ("Windows-1250","cp1250","Windows-1250 - Central Europe","Windows"),
 ("Windows-1251","cp1251","Windows-1251 - Cyrillic","Windows"),
 ("Windows-1252","cp1252","Windows-1252 - Western Europe","Windows"),
 ("Windows-1253","cp1253","Windows-1253 - Greek","Windows"),
 ("Windows-1254","cp1254","Windows-1254 - Turkish","Windows"),
 ("Windows-1255","cp1255","Windows-1255 - Hebrew","Windows"),
 ("Windows-1256","cp1256","Windows-1256 - Arabic","Windows"),
 ("Windows-1257","cp1257","Windows-1257 - Baltic","Windows"),
 ("Windows-1258","cp1258","Windows-1258 - Vietnamese","Windows"),
] + [(f"ISO-8859-{n}", f"iso8859_{n}", f"ISO-8859-{n} - {d}", "ISO 8859") for n,d in [
 (1,"Latin-1 Western Europe"),(2,"Latin-2 Central Europe"),(3,"Latin-3 South Europe"),(4,"Latin-4 North Europe"),
 (5,"Cyrillic"),(6,"Arabic"),(7,"Greek"),(8,"Hebrew"),(9,"Latin-5 Turkish"),(10,"Latin-6 Nordic"),
 (11,"Thai"),(13,"Latin-7 Baltic Rim"),(14,"Latin-8 Celtic"),(15,"Latin-9 Western Europe with Euro"),(16,"Latin-10 South-East Europe")]] + [
 ("Mac-Roman","mac_roman","Mac Roman - Classic Mac OS Western","Other"),
 ("Mac-Cyrillic","mac_cyrillic","Mac Cyrillic - Classic Mac OS","Other"),
]
out = ["// Generated from Python's codec tables by tools/gen_codepages.py. Do not edit by hand.",
       "// Each table maps the 256 byte values to Unicode code points.",""]
for cid, codec, disp, grp in CPS:
    tbl=[]
    for b in range(256):
        try: tbl.append(ord(bytes([b]).decode(codec)))
        except UnicodeDecodeError: tbl.append(None)
    used=set(x for x in tbl if x is not None)
    # Undefined bytes map to U+00xx when free, else to private-use U+F700+xx,
    # so every byte survives opening and saving unchanged.
    tbl=[x if x is not None else (b if b not in used else 0xF700 + b) for b,x in enumerate(tbl)]
    rows=",".join(f"0x{x:04X}" for x in tbl)
    out.append(f'{{"{cid}", "{disp}", "{grp}", {{{rows}}}}},')
open(sys.argv[1],"w").write("\n".join(out)+"\n")
print(len(CPS))
