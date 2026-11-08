import contextlib, io, hashlib, http.server, json, os, shutil, socketserver, struct, sys, tempfile, threading, unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from install_sd import install, sequence
from validate_assets import validate
from upload_assets import upload
PACKAGE=Path(__file__).resolve().parents[1]/'assets/package/microcity.mcap'
VERSION=validate(PACKAGE)['version']
class Installer(unittest.TestCase):
 def setUp(self):self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name)
 def tearDown(self):self.temp.cleanup()
 def test_interruption_and_space(self):
  install(PACKAGE,self.root)
  with self.assertRaises(OSError):install(PACKAGE,self.root,fail_after=8192)
  self.assertEqual(sequence(self.root/'microcity/slot-b.ready'),0)
  self.assertEqual(validate(self.root/'microcity/slot-a.mcap')['version'],VERSION)
  with patch('install_sd.shutil.disk_usage',return_value=shutil._ntuple_diskusage(100,100,0)):
   with self.assertRaises(OSError):install(PACKAGE,self.root)
  self.assertEqual(sequence(self.root/'microcity/slot-a.ready'),1)
 def test_version_switch_and_same_sprite_ids(self):
  install(PACKAGE,self.root);a=bytearray(PACKAGE.read_bytes());struct.pack_into('<I',a,16,VERSION+1)
  # Change one facade pixel and recompute the package digest: pure art update.
  count=struct.unpack_from('<I',a,20)[0]
  a[60+count*20+10000]^=31;a[28:60]=hashlib.sha256(a[60:]).digest();v2=self.root/'v2.mcap';v2.write_bytes(a)
  self.assertEqual(install(v2,self.root)['version'],VERSION+1)
  self.assertEqual(a[60:60+count*20],PACKAGE.read_bytes()[60:60+count*20])
  self.assertEqual(validate(self.root/'microcity/slot-a.mcap')['version'],VERSION)
 def test_incomplete_truncated_incompatible_duplicate(self):
  for offset in (8,20,60,70):
   a=bytearray(PACKAGE.read_bytes());a[offset]^=127;p=self.root/'bad';p.write_bytes(a)
   with self.assertRaises(ValueError):validate(p)
  p.write_bytes(PACKAGE.read_bytes()[:-1])
  with self.assertRaises(ValueError):validate(p)
 def test_uploader_stream(self):
  received=[]
  class Handler(http.server.BaseHTTPRequestHandler):
   def do_POST(s):
    received.append((s.path,s.headers.get('Authorization'),s.rfile.read(int(s.headers['Content-Length']))))
    s.send_response(200);s.end_headers();s.wfile.write(json.dumps(dict(status='installed',version=VERSION)).encode())
   def log_message(*args):pass
  with socketserver.TCPServer(('127.0.0.1',0),Handler) as server:
   t=threading.Thread(target=server.serve_forever);t.start()
   try:
    with contextlib.redirect_stdout(io.StringIO()):self.assertEqual(upload(f'http://127.0.0.1:{server.server_address[1]}/assets',PACKAGE,'fictional-token')['version'],VERSION)
   finally:server.shutdown();t.join()
  self.assertEqual(received[0],('/assets','Bearer fictional-token',PACKAGE.read_bytes()))
if __name__=='__main__':unittest.main()
