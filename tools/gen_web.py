#!/usr/bin/env python3
"""Ubah web/index.html (+ logo) menjadi src/web_page.h (string PROGMEM).
Jalankan setiap habis mengubah web/index.html:  python3 tools/gen_web.py
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
