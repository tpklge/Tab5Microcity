import hashlib,json,struct,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class VisualUpdate(unittest.TestCase):
 def test_only_service_art_changes_with_stable_ids_and_geometry(self):
  old=json.loads((ROOT/'tests/fixtures/assets-v1-hashes.json').read_text());raw=(ROOT/'assets/package/microcity.mcap').read_bytes();count=struct.unpack_from('<I',raw,20)[0];current={};changed=set();hashes={}
  self.assertEqual(struct.unpack_from('<I',raw,16)[0],10)
  for i in range(count):
   sid,w,h,ax,ay,fmt,offset,size=struct.unpack_from('<6H2I',raw,60+i*20);key=str(sid);current[key]=True;hashes[sid]=hashlib.sha256(raw[60+offset:60+offset+size]).hexdigest()
   if key in old:
    self.assertEqual([w,h,ax,ay,fmt],old[key]['shape'])
    if hashlib.sha256(raw[60+offset:60+offset+size]).hexdigest()!=old[key]['sha256']:changed.add(sid)
  self.assertTrue(set(old).issubset(current));self.assertEqual(changed,set(range(41,56))|set(range(100,106)))
  self.assertEqual(set(current)-set(old),set(map(str,[114,115,116,117,132,133,134,135,*range(140,192)])))
  self.assertEqual(len({hashes[i] for i in range(132,136)}),4)
  self.assertTrue(all(hashes[i]!=hashes[117] for i in range(132,136)))
if __name__=='__main__':unittest.main()
