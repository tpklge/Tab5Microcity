#!/usr/bin/env python3
"""Copy build artifacts and license notices. Never flashes or opens a device."""
import hashlib, json, os, re, shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def package():
    build=ROOT/'build';out=ROOT/'dist';usb=out/'usb';usb.mkdir(parents=True,exist_ok=True)
    args=json.loads((build/'flasher_args.json').read_text());inventory=[]
    for offset,relative in args['flash_files'].items():
        src=build/relative;dst=usb/relative;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst);inventory.append(dict(offset=offset,path=relative,size=dst.stat().st_size,sha256=hashlib.sha256(dst.read_bytes()).hexdigest()))
    for name in ['flash_args','flasher_args.json']:shutil.copy2(build/name,usb/name)
    shutil.copy2(build/'microcity.bin',out/'microcity-app-ota.bin')
    (usb/'README.txt').write_text('Pacote USB ESP32-P4 / flash 16 MB. Consulte README.md do projeto e flash_args.\nGerado sem gravar o dispositivo. Flash exige ação explícita do usuário.\nAssets são instalados separadamente no microSD.\n')
    notices=out/'licenses';notices.mkdir(exist_ok=True);shutil.copy2(ROOT/'LICENSE',notices/'MicroCity-MIT.txt');shutil.copy2(ROOT/'docs/TERCEIROS.md',notices/'TERCEIROS.md')
    shutil.copy2(ROOT/'assets/fonts/OFL.txt',notices/'NotoSans-OFL.txt')
    for component in list((ROOT/'managed_components').iterdir())+list((ROOT/'components').iterdir()):
        if not component.is_dir():continue
        for path in component.rglob('*'):
            if path.is_file() and path.name.upper().startswith(('LICENSE','COPYING','NOTICE')):
                dst=notices/'components'/component.name/path.relative_to(component);dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,dst)
    sdk=Path(os.environ.get('IDF_PATH','/Users/paludo/esp/esp-idf'))
    for folder in [sdk]:
        for path in folder.rglob('*'):
            if path.is_file() and path.name.upper().startswith(('LICENSE','COPYING','NOTICE')) and '.git' not in path.parts:
                dst=notices/'esp-idf'/path.relative_to(sdk);dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,dst)
    shutil.copy2(ROOT/'assets/package/microcity.mcap',out/'microcity-assets.mcap');shutil.copy2(ROOT/'assets/package/manifest.json',out/'assets-manifest.json')
    (out/'build-inventory.json').write_text(json.dumps(dict(engine=1,firmware=re.search(r'set\(PROJECT_VER \"([^\"]+)\"\)',(ROOT/'CMakeLists.txt').read_text()).group(1),usb=inventory,application_sha256=hashlib.sha256((out/'microcity-app-ota.bin').read_bytes()).hexdigest()),indent=2)+'\n')
    launcher=out/'launcher/microcity';launcher.mkdir(parents=True,exist_ok=True);shutil.copy2(out/'microcity-app-ota.bin',launcher/'microcity.bin')
    print(json.dumps(inventory,indent=2))
if __name__=='__main__':package()
