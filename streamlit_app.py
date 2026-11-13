"""
NodeCraft on Streamlit.

Streamlit cannot serve the site's .js/.wasm files with the right content types,
so this app builds ONE self-contained HTML page and shows it with
components.html():

  web/index.html  +  web/style.css  +  web/nodecraft.single.js  +  web/script.js

web/nodecraft.single.js is the C++ engine with the .wasm embedded inside it
(build it with `make streamlit`). All data structure logic still runs in that
C++ code; this file only packages the page.

Run locally:   streamlit run streamlit_app.py
"""
from pathlib import Path

import streamlit as st
import streamlit.components.v1 as components

WEB = Path(__file__).parent / "web"

st.set_page_config(page_title="NodeCraft", page_icon="🧩", layout="wide",
                   initial_sidebar_state="collapsed")

# Let the embedded page use the whole browser window: hide Streamlit's own
# header, footer and padding, and stretch the iframe to the window height.
st.markdown(
    """
    <style>
      header[data-testid="stHeader"], footer, #MainMenu { display: none !important; }
      .block-container, [data-testid="stMainBlockContainer"] {
        padding: 0 !important; max-width: 100% !important;
      }
      [data-testid="stVerticalBlock"] { gap: 0 !important; }
      iframe { display: block; width: 100% !important; height: 100vh !important; border: 0; }
    </style>
    """,
    unsafe_allow_html=True,
)


def _replace_once(html: str, old: str, new: str) -> str:
    """Replace exactly one occurrence, so a changed index.html fails loudly."""
    if html.count(old) != 1:
        raise RuntimeError(f"index.html should contain {old!r} exactly once")
    return html.replace(old, new)


@st.cache_data(show_spinner=False)
def build_page() -> str:
    html = (WEB / "index.html").read_text(encoding="utf-8")
    css = (WEB / "style.css").read_text(encoding="utf-8")
    engine = (WEB / "nodecraft.single.js").read_text(encoding="utf-8")
    script = (WEB / "script.js").read_text(encoding="utf-8")

    for name, code in (("nodecraft.single.js", engine), ("script.js", script)):
        if "</script" in code.lower():
            raise RuntimeError(f"{name} contains '</script' and cannot be inlined")

    html = _replace_once(html, '<link rel="stylesheet" href="style.css">', f"<style>\n{css}\n</style>")
    html = _replace_once(
        html,
        '<script src="nodecraft.js"></script>',
        f"<script>window.NODECRAFT_SINGLE_FILE = true;</script>\n<script>\n{engine}\n</script>",
    )
    html = _replace_once(html, '<script src="script.js"></script>', f"<script>\n{script}\n</script>")
    return html


if not (WEB / "nodecraft.single.js").exists():
    st.error("web/nodecraft.single.js is missing. Build it with `make streamlit` and commit it.")
    st.stop()

# height is the fallback; the CSS above stretches the iframe to the window.
components.html(build_page(), height=900, scrolling=True)
