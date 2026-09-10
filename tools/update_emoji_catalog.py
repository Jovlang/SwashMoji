#!/usr/bin/env python3
"""Generate the multilingual catalog from pinned CLDR. Runtime stays offline.

First run: --download --cldr-dir build/cldr. Subsequent runs need only the cache.
Missing locale files are accepted only when CLDR returns HTTP 404.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
from urllib.error import HTTPError
from urllib.request import urlopen
import xml.etree.ElementTree as ET

CLDR_TAG = "release-48-2"
CLDR_BASE = f"https://raw.githubusercontent.com/unicode-org/cldr/{CLDR_TAG}/common"
EXTRA_LOCALES = ("de", "it")


def normalized_glyph(value: str) -> str:
    return value.replace("\ufe0f", "")


class Sources:
    def __init__(self, directory: Path, download: bool = False):
        self.directory, self.download = directory, download
        self.hashes: dict[str, str | None] = {}

    def read(self, path: str, optional: bool = False):
        local = self.directory / path
        missing = local.with_suffix(".missing")
        if not local.exists() and not missing.exists() and self.download:
            local.parent.mkdir(parents=True, exist_ok=True)
            try:
                with urlopen(f"{CLDR_BASE}/{path}", timeout=60) as response:
                    local.write_bytes(response.read())
            except HTTPError as error:
                if error.code != 404 or not optional:
                    raise
                missing.write_text("404\n", encoding="ascii")
        if optional and missing.exists() and not local.exists():
            self.hashes[path] = None
            return None
        data = local.read_bytes()  # An incomplete offline cache is an error.
        self.hashes[path] = hashlib.sha256(data).hexdigest()
        return ET.fromstring(data)


def parent_chain(locale: str, supplemental) -> list[str]:
    parents = {}
    for group in supplemental.findall(".//parentLocales"):
        if group.get("component"):
            continue
        for item in group.findall("parentLocale"):
            for child in item.get("locales", "").split():
                parents[child] = item.get("parent", "root")
    chain = []
    while locale != "root":
        if locale in chain:
            raise ValueError("CLDR locale inheritance cycle")
        chain.append(locale)
        locale = parents.get(locale, locale.rsplit("_", 1)[0] if "_" in locale else "root")
    return list(reversed(chain))


def load_locale(sources: Sources, locale: str, supplemental):
    names, keywords = {}, {}
    for inherited in parent_chain(locale, supplemental):
        for folder in ("annotationsDerived", "annotations"):
            root = sources.read(f"{folder}/{inherited}.xml", optional=inherited != "en")
            if root is None:
                continue
            for annotation in root.findall(".//annotation"):
                glyph = normalized_glyph(annotation.get("cp", ""))
                text = (annotation.text or "").strip()
                if not glyph or not text or text == "↑↑↑":
                    continue
                if annotation.get("type") == "tts":
                    names[glyph] = text
                else:
                    keywords[glyph] = {part.strip() for part in text.split("|") if part.strip()}
    return names, keywords


def generate(catalog: str, sources: Sources) -> str:
    supplemental = sources.read("supplemental/supplementalData.xml")
    en_names, en_keywords = load_locale(sources, "en", supplemental)
    nb_names, nb_keywords = load_locale(sources, "nb", supplemental)
    translations = {locale: load_locale(sources, locale, supplemental) for locale in EXTRA_LOCALES}
    output = []
    for line in catalog.splitlines():
        if not line.strip():
            continue
        fields = line.split("\t") if "\t" in line else line.split(maxsplit=1)
        glyph, old_name = fields[:2]
        key = normalized_glyph(glyph)
        name = en_names.get(key, old_name)
        words = set(en_keywords.get(key, ())) | {name, old_name}
        # Preserve existing English search vocabulary, including informal aliases.
        if len(fields) >= 3:
            words.update(part.strip() for part in fields[2].split("|") if part.strip())
        nb_name = nb_names.get(key, name)
        nb_words = set(nb_keywords.get(key, en_keywords.get(key, ()))) | {nb_name}
        join = lambda values: " | ".join(sorted(values, key=lambda value: (value.casefold(), value)))
        row = [glyph, name, join(words), nb_name, join(nb_words)]
        # Keep the legacy columns stable; further languages are explicit locale/name/keywords triples.
        # Missing translations stay missing, so display fallback does not masquerade as translation.
        for locale, (names, keywords) in translations.items():
            local_name = names.get(key, "")
            local_words = set(keywords.get(key, ()))
            if local_name:
                local_words.add(local_name)
            if local_name or local_words:
                row.extend((locale, local_name, join(local_words)))
        output.append("\t".join(row))
    return "\n".join(output) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("catalog", nargs="?", type=Path, default=Path("emojis.txt"))
    parser.add_argument("--cldr-dir", type=Path, default=Path("build/cldr"))
    parser.add_argument("--download", action="store_true")
    parser.add_argument("--manifest", type=Path, default=Path("data/catalog_sources.json"))
    args = parser.parse_args()
    sources = Sources(args.cldr_dir, args.download)
    result = generate(args.catalog.read_text(encoding="utf-8-sig"), sources)
    args.catalog.write_text(result, encoding="utf-8", newline="\n")
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    args.manifest.write_text(json.dumps({"cldr_tag": CLDR_TAG, "base_url": CLDR_BASE,
        "locales": ["en", "nb", *EXTRA_LOCALES], "fallback": "en", "files_sha256": sources.hashes},
        indent=2, sort_keys=True) + "\n", encoding="utf-8", newline="\n")
    print(f"Generated {len(result.splitlines())} multilingual entries from {CLDR_TAG}.")


if __name__ == "__main__":
    main()
