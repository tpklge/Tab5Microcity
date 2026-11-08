import json,re,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class UIFont(unittest.TestCase):
 def test_interface_character_coverage(self):
  coverage=json.loads((ROOT/'assets/fonts/coverage.json').read_text());supported=set(coverage['codepoints'])
  # All visible non-ASCII literals in firmware are covered, not merely example words.
  used=set()
  for folder in ['main/ui','main/core']:
   for p in (ROOT/folder).glob('*.cpp'):
    for literal in re.findall(r'"((?:[^"\\]|\\.)*)"',p.read_text()):
     used.update(ord(ch) for ch in literal if ord(ch)>127)
  self.assertEqual(used-supported,set())
  self.assertEqual(set(map(int,coverage['glyph_ids'])),supported)
  self.assertTrue(all(i>0 for i in coverage['glyph_ids'].values()))
  self.assertTrue(set(map(ord,'açãoÁÉÍÓÚçãõâêô−×•←↑→↓'))<=supported)
 def test_generated_cmap_and_glyphs(self):
  source=(ROOT/'main/ui/fonts/mc_font_ui.c').read_text();coverage=json.loads((ROOT/'assets/fonts/coverage.json').read_text())
  numbers=re.search(r'unicodes\[\]=\{([^}]+)',source).group(1)
  self.assertEqual([int(n)+32 for n in numbers.split(',') if n.strip()],coverage['codepoints'])
  glyphs=re.findall(r'\.bitmap_index=(\d+),\.adv_w=(\d+),\.box_w=(\d+),\.box_h=(\d+)',source)
  for ch in 'áéíóúãõçÁÉÍÓÚ':
   glyph=glyphs[coverage['codepoints'].index(ord(ch))];self.assertGreater(int(glyph[2])*int(glyph[3]),0)
if __name__=='__main__':unittest.main()
