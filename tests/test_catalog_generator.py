"""Network-free tests for CLDR inheritance, fallback and repeatability."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("catalog_generator", ROOT / "tools/update_emoji_catalog.py")
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)


class CatalogGeneration(unittest.TestCase):
    def test_inheritance_fallback_and_repeatability(self):
        with tempfile.TemporaryDirectory() as directory:
            cache = Path(directory)
            files = {
                "supplemental/supplementalData.xml": '<supplementalData><parentLocales><parentLocale parent="no" locales="nb nn"/></parentLocales></supplementalData>',
                "annotationsDerived/en.xml": '<ldml><annotations><annotation cp="🚀" type="tts">derived rocket</annotation></annotations></ldml>',
                "annotations/en.xml": '<ldml><annotations><annotation cp="🚀" type="tts">rocket</annotation><annotation cp="🚀">space | launch</annotation><annotation cp="☕" type="tts">hot beverage</annotation><annotation cp="☕">coffee</annotation></annotations></ldml>',
                "annotationsDerived/no.xml": '<ldml><annotations><annotation cp="🚀" type="tts">rakett</annotation></annotations></ldml>',
                "annotations/no.xml": '<ldml><annotations><annotation cp="🚀">verdensrommet</annotation></annotations></ldml>',
                "annotations/nb.xml": '<ldml><annotations><annotation cp="🚀" type="tts">↑↑↑</annotation></annotations></ldml>',
                "annotationsDerived/nb.missing": "404",
            }
            for name, text in files.items():
                path = cache / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(text, encoding="utf-8")
            original = "🚀\told rocket\tship\n☕\tcoffee\tbreak\n"
            sources = generator.Sources(cache)
            result = generator.generate(original, sources)
            rows = [line.split("\t") for line in result.splitlines()]
            self.assertEqual(rows[0][1], "rocket")
            self.assertEqual(rows[0][3], "rakett")
            self.assertIn("verdensrommet", rows[0][4])
            self.assertIn("ship", rows[0][2])
            self.assertEqual(rows[1][3], "hot beverage")
            self.assertIn("coffee", rows[1][4])
            self.assertEqual(generator.generate(result, generator.Sources(cache)), result)
            self.assertIsNone(sources.hashes["annotationsDerived/nb.xml"])
            self.assertEqual(len(sources.hashes["annotations/en.xml"]), 64)
            (cache / "annotations/no.xml").unlink()
            with self.assertRaises(FileNotFoundError):
                generator.generate(original, generator.Sources(cache))

    def test_curated_intents_are_separate_valid_records(self):
        glyphs = {line.split("\t")[0] for line in (ROOT / "emojis.txt").read_text(encoding="utf-8").splitlines()}
        phrases = set()
        for line in (ROOT / "intent_phrases.tsv").read_text(encoding="utf-8").splitlines():
            if not line or line.startswith("#"):
                continue
            phrase, glyph = line.split("\t")
            self.assertIn(glyph, glyphs)
            self.assertNotIn(phrase, phrases)
            phrases.add(phrase)
        self.assertGreaterEqual(len(phrases), 40)


if __name__ == "__main__":
    unittest.main()
