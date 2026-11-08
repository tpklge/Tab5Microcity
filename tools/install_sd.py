#!/usr/bin/env python3
"""Manual installer uses identical ready records and slots as the Tab5."""
import argparse, os, shutil, struct, zlib
from pathlib import Path
from validate_assets import validate

def sequence(path):
    try:
        data=path.read_bytes();magic,seq,crc=struct.unpack('<3I',data)
        return seq if magic==0x59444152 and zlib.crc32(data[:8])==crc else 0
    except (OSError,ValueError,struct.error):return 0

def install(package,card,fail_after=None):
    info=validate(package);root=Path(card)/'microcity';root.mkdir(parents=True,exist_ok=True)
    sequences=[sequence(root/f'slot-{s}.ready') for s in 'ab'];valid=[]
    for s,seq in zip('ab',sequences):
        try:
            validate(root/f'slot-{s}.mcap')
            if seq:valid.append((seq,s))
        except (OSError,ValueError,struct.error):pass
    active=max(valid,default=(0,'b'))[1];slot='b' if active=='a' else 'a';target=root/f'slot-{slot}.mcap';ready=root/f'slot-{slot}.ready'
    ready.unlink(missing_ok=True);target.unlink(missing_ok=True)
    if shutil.disk_usage(root).free<info['bytes']+65536:raise OSError('insufficient free space')
    with open(package,'rb') as src,target.open('wb') as dst:
        copied=0
        while block:=src.read(4096):
            dst.write(block);copied+=len(block)
            if fail_after is not None and copied>=fail_after:raise OSError('injected interruption')
        dst.flush();os.fsync(dst.fileno())
    validate(target)
    seq=max(sequences)+1
    if seq>0xffffffff:raise ValueError('sequence exhausted')
    record=struct.pack('<2I',0x59444152,seq);record+=struct.pack('<I',zlib.crc32(record))
    with ready.open('wb') as f:f.write(record);f.flush();os.fsync(f.fileno())
    return dict(info,slot=slot,sequence=seq)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('card',type=Path);p.add_argument('--package',type=Path,default=Path(__file__).resolve().parents[1]/'assets/package/microcity.mcap');a=p.parse_args();print(install(a.package,a.card))
