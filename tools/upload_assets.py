#!/usr/bin/env python3
import argparse, http.client, json, os
from pathlib import Path
from urllib.parse import urlsplit
from validate_assets import validate

def upload(url,package,token):
    info=validate(package);u=urlsplit(url)
    if u.scheme not in ('http','https') or not u.hostname or u.username or u.password:raise ValueError('invalid receiver URL')
    conn=(http.client.HTTPSConnection if u.scheme=='https' else http.client.HTTPConnection)(u.hostname,u.port,timeout=15)
    try:
        conn.putrequest('POST',u.path or '/assets');conn.putheader('Authorization','Bearer '+token);conn.putheader('Content-Type','application/octet-stream');conn.putheader('Content-Length',str(info['bytes']));conn.endheaders()
        sent=0;last=-1
        with open(package,'rb') as f:
            while block:=f.read(4096):
                conn.send(block);sent+=len(block);percent=sent*100//info['bytes']
                if percent!=last:print(f'\rEnviando: {percent}%',end='',flush=True);last=percent
        response=conn.getresponse();body=response.read(4096).decode('utf-8','replace');print()
        if response.status!=200:raise RuntimeError(f'{response.status}: {body}')
        result=json.loads(body)
        if result.get('status')!='installed' or result.get('version')!=info['version']:raise RuntimeError('unexpected server response')
        return result
    finally:conn.close()
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('url');p.add_argument('--package',type=Path,default=Path(__file__).resolve().parents[1]/'assets/package/microcity.mcap');p.add_argument('--token',default=os.environ.get('MICROCITY_TOKEN'));a=p.parse_args()
    if not a.token:p.error('informe --token ou MICROCITY_TOKEN')
    try:print('Instalado:',upload(a.url,a.package,a.token))
    except (OSError,ValueError,RuntimeError) as e:p.exit(1,f'Erro: {e}\n')
