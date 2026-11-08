#!/usr/bin/env python3
"""Validate the same bounded MCAP v1 format used by firmware."""
import argparse, hashlib, json, struct
from pathlib import Path
MAX=32*1024*1024
REQUIRED=set([0,1,2,30,31,32,60,61,62,100,101,102,103,104,105,110,111,112,113,114,115,116,117,132,133,134,135,172])|set(range(10,26))|set(range(40,56))|set(range(70,76))|set(range(80,86))|set(range(90,96))|set(range(140,172))|set(range(173,189))
def validate(path):
    path=Path(path); size=path.stat().st_size
    if not 60<=size<=MAX:raise ValueError('package size')
    with path.open('rb') as f:
        magic,fmt,lo,hi,version,count,length,digest=struct.unpack('<4s6I32s',f.read(60))
        if magic!=b'MCAP' or fmt!=1 or not lo<=1<=hi:raise ValueError('incompatible runtime/engine')
        if not 1<=count<=256 or length!=size-60 or length<count*20:raise ValueError('catalog length')
        seen=set();offset=count*20
        for _ in range(count):
            sid,w,h,ax,ay,pixel,start,n=struct.unpack('<6H2I',f.read(20))
            valid=(pixel==1 and 1<=w<=256 and 1<=h<=320 and ax<w and ay<h and n==w*h*3)
            valid=valid or (pixel==2 and w==2400 and h==1 and ax==ay==0 and n==4800 and 200<=sid<=202)
            if sid>=256 or sid in seen or not valid or start!=offset or n>length-start:raise ValueError('invalid sprite metadata')
            seen.add(sid);offset+=n
        if offset!=length or not REQUIRED<=seen:raise ValueError('incomplete catalog')
        f.seek(60);sha=hashlib.sha256()
        while block:=f.read(4096):sha.update(block)
        if sha.digest()!=digest:raise ValueError('SHA-256 mismatch')
    return dict(version=version,count=count,bytes=size)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('package',type=Path,nargs='?',default=Path(__file__).resolve().parents[1]/'assets/package/microcity.mcap');a=p.parse_args();print(json.dumps(validate(a.package)))
