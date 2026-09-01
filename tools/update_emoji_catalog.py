#!/usr/bin/env python3
"""Enrich emojis.txt with pinned Unicode CLDR English names and keywords."""

from __future__ import annotations

import argparse
from collections import defaultdict
from pathlib import Path
from urllib.request import urlopen
import xml.etree.ElementTree as ET


CLDR_VERSION = "48.2"
CLDR_TAG = "release-48-2"
CLDR_BASE = f"https://raw.githubusercontent.com/unicode-org/cldr/{CLDR_TAG}/common"
CLDR_FILES = (
    f"{CLDR_BASE}/annotationsDerived/en.xml",
    f"{CLDR_BASE}/annotations/en.xml",
)

# CLDR deliberately stays formal. These few high-value aliases cover how people
# commonly search for expressive faces without turning the catalog into a thesaurus.
ALIASES = {
    "😂": ("funny", "haha", "hilarious", "lmao", "lol", "rofl"),
    "🤣": ("funny", "haha", "hilarious", "lmao", "lol", "rofl"),
    "😆": ("haha", "laugh", "lol"),
    "😹": ("haha", "laugh", "lol"),
    "🤭": ("giggle", "laugh"),
}


def normalized_glyph(value: str) -> str:
    return value.replace("\ufe0f", "")


def load_cldr() -> tuple[dict[str, str], dict[str, set[str]]]:
    names: dict[str, str] = {}
    keywords: dict[str, set[str]] = defaultdict(set)

    # Derived annotations establish the full catalog; hand-authored annotations
    # are read second so their preferred short names win.
    for url in CLDR_FILES:
        with urlopen(url) as response:
            root = ET.fromstring(response.read())
        for annotation in root.findall(".//annotation"):
            glyph = normalized_glyph(annotation.attrib.get("cp", ""))
            text = (annotation.text or "").strip()
            if not glyph or not text:
                continue
            if annotation.attrib.get("type") == "tts":
                names[glyph] = text
            else:
                keywords[glyph].update(part.strip() for part in text.split("|") if part.strip())
    return names, keywords


def parse_catalog_line(line: str) -> tuple[str, str]:
    if "\t" in line:
        fields = line.split("\t", 2)
        return fields[0], fields[1]
    return tuple(line.split(maxsplit=1))  # type: ignore[return-value]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("catalog", nargs="?", type=Path, default=Path("emojis.txt"))
    args = parser.parse_args()

    names, cldr_keywords = load_cldr()
    output: list[str] = []
    matched = 0
    for raw_line in args.catalog.read_text(encoding="utf-8-sig").splitlines():
        if not raw_line.strip():
            continue
        glyph, old_name = parse_catalog_line(raw_line)
        key = normalized_glyph(glyph)
        name = names.get(key, old_name)
        if key in names:
            matched += 1
        keywords = set(cldr_keywords.get(key, ()))
        keywords.update((name, old_name))
        keywords.update(ALIASES.get(key, ()))
        searchable = " | ".join(sorted(keywords, key=str.casefold))
        output.append(f"{glyph}\t{name}\t{searchable}")

    args.catalog.write_text("\n".join(output) + "\n", encoding="utf-8", newline="\n")
    print(f"Updated {len(output)} entries ({matched} matched CLDR {CLDR_VERSION}).")


if __name__ == "__main__":
    main()
