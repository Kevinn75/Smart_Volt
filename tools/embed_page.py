# Copia docs/index.html dentro del firmware (src/web_page.h) antes de compilar.
# Así hay UNA sola página: la misma que sirve el ESP32 y la que se publica en GitHub Pages.
Import("env")
import os

root = env.subst("$PROJECT_DIR")
src = os.path.join(root, "docs", "index.html")
dst = os.path.join(root, "src", "web_page.h")

with open(src, encoding="utf-8") as f:
    html = f.read()
if ')rawliteral"' in html:
    raise Exception("docs/index.html no puede contener la secuencia )rawliteral\"")

out = (
    "// GENERADO AUTOMÁTICAMENTE desde docs/index.html por tools/embed_page.py.\n"
    "// No editar este archivo: los cambios se pierden al compilar.\n"
    "#ifndef WEB_PAGE_H\n#define WEB_PAGE_H\n\n#include <Arduino.h>\n\n"
    'static const char WEB_PAGE[] PROGMEM = R"rawliteral(' + html + ')rawliteral";\n\n'
    "#endif // WEB_PAGE_H\n"
)

old = None
if os.path.exists(dst):
    with open(dst, encoding="utf-8") as f:
        old = f.read()
if old != out:
    with open(dst, "w", encoding="utf-8", newline="\n") as f:
        f.write(out)
    print("embed_page: src/web_page.h actualizado desde docs/index.html")
