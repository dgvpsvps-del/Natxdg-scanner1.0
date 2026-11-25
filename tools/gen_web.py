#!/usr/bin/env python3
"""Buat src/web_page.h (dari web/index.html + logo) dan src/donate_png.h (dari docs/donate-qris.png).
Jalankan setiap habis mengubah web/index.html atau QR:  python3 tools/gen_web.py
"""
import pathlib
root = pathlib.Path(__file__).resolve().parent.parent

html = (root / "web" / "index.html").read_text(encoding="utf-8")
logo = (root / "web" / "logo.b64").read_text().strip()
html = html.replace("__LOGO__", logo)
assert ')NXDG"' not in html
out = ("// Dibuat otomatis oleh tools/gen_web.py dari web/index.html - jangan diedit manual\n"
       "#pragma once\n#include <Arduino.h>\n\n"
       'const char INDEX_HTML[] PROGMEM = R"NXDG(' + html + ')NXDG";\n')
(root / "src" / "web_page.h").write_text(out, encoding="utf-8")
print("src/web_page.h", len(out), "bytes")

png = (root / "docs" / "donate-qris.png").read_bytes()
rows = []
for i in range(0, len(png), 16):
    rows.append("  " + ", ".join("0x%02X" % b for b in png[i:i + 16]))
out = ("// Dibuat otomatis oleh tools/gen_web.py dari docs/donate-qris.png\n"
       "#pragma once\n#include <Arduino.h>\n\n"
       "const uint8_t DONATE_PNG[] PROGMEM = {\n" + ",\n".join(rows) + "\n};\n"
       "const size_t DONATE_PNG_LEN = %d;\n" % len(png))
(root / "src" / "donate_png.h").write_text(out, encoding="utf-8")
print("src/donate_png.h", len(png), "bytes png")
