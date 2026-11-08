#!/usr/bin/env python3
"""Original MicroCity art. Seed 51, RGB565+A8 runtime, stable numeric IDs."""
import argparse, hashlib, json, math, random, struct, wave
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from service_art import create_services
from civic_art import create_civic
from transport_art import create_transport
GENERATOR='1.3.0'
def generate(root, version):
    source=root/'assets/source'; source.mkdir(parents=True,exist_ok=True)
    out=root/'assets/package'; out.mkdir(parents=True,exist_ok=True)
    sprites={}; names={}; anchors={}
    def add(sid,name,img,anchor=None):
        sprites[sid]=img;names[sid]=name;anchors[sid]=anchor or (32,79);img.save(source/f'{sid:03}-{name}.png')
    def base(color=(100,148,87),variant=0):
        im=Image.new('RGBA',(64,96));d=ImageDraw.Draw(im)
        d.polygon([(32,63),(63,79),(32,95),(0,79)],fill=color,outline=(48,86,65))
        rng=random.Random(51+variant)
        for _ in range(16):
            x=rng.randrange(4,60);y=rng.randrange(66,93)
            if abs(x-32)/32+abs(y-79)/16<.8:d.point((x,y),fill=tuple(min(255,c+18) for c in color))
        return im
    for n in range(3): add(n,'grass'+str(n),base(variant=n))
    for mask in range(16):
        im=base((53,132,167));d=ImageDraw.Draw(im)
        corners=[((32,63),(63,79)),((63,79),(32,95)),((32,95),(0,79)),((0,79),(32,63))]
        for bit,(a,b) in enumerate(corners):
            if mask&(1<<bit): d.line([a,b],fill=(204,194,142),width=3)
        d.line([(20,78),(29,78),(33,76)],fill=(116,190,201),width=1)
        add(10+mask,'water'+str(mask),im)
    for n in range(3):
        im=base();d=ImageDraw.Draw(im);d.ellipse((14,76,48,87),fill=(46,85,66,180));d.rectangle((30,44,34,81),fill=(115,83,59))
        for x,y,r in [(23,51,13),(38,51,14),(31,35,16)]:
            d.ellipse((x-r,y-r,x+r,y+r),fill=[(38,102,71),(45,121,75),(78,143,79)][n],outline=(28,85,62));d.arc((x-r+3,y-r+2,x+r-3,y+r-3),190,290,fill=(132,177,88),width=2)
        add(30+n,'tree'+str(n),im)
    centers=[(48,71),(48,87),(16,87),(16,71)]
    for mask in range(16):
        im=base();d=ImageDraw.Draw(im);d.polygon([(32,71),(48,79),(32,87),(16,79)],fill=(65,75,82))
        for bit,(x,y) in enumerate(centers):
            if mask&(1<<bit):
                ex=32+(x-32)*3//2;ey=79+(y-79)*3//2
                d.line([(32,79),(ex,ey)],fill=(65,75,82),width=15)
                d.line([(32,79),(ex,ey)],fill=(225,198,109),width=1)
        add(40+mask,'road'+str(mask),im)
    for sid,color,label in [(60,(120,212,135),'R'),(61,(100,182,228),'C'),(62,(235,193,89),'I')]:
        im=base();d=ImageDraw.Draw(im);d.polygon([(32,67),(56,79),(32,91),(8,79)],outline=color,width=2);d.text((29,72),label,fill=color);add(sid,'zone'+label,im)
    def building(color,height,variant,kind):
        im=base();d=ImageDraw.Draw(im);bottom=79;top=bottom-height
        d.ellipse((10,76,58,90),fill=(35,65,58,150))
        d.polygon([(10,top),(32,top+11),(32,bottom+11),(10,bottom)],fill=tuple(int(c*.72) for c in color),outline=(47,65,70))
        d.polygon([(32,top+11),(54,top),(54,bottom),(32,bottom+11)],fill=color,outline=(47,65,70))
        d.polygon([(10,top),(32,top-11),(54,top),(32,top+11)],fill=tuple(min(255,int(c*1.2)) for c in color),outline=(47,65,70))
        for y in range(top+15,bottom-2,9):
            for x in (16,24,38,46):d.rectangle((x,y,x+3,y+4),fill=(240,211,129) if (x+y+variant)%3 else (66,103,119))
        if kind=='R':d.polygon([(7,top),(32,top-15),(57,top),(32,top+9)],fill=(183,99+variant*12,75),outline=(110,66,59))
        if kind=='I':d.rectangle((39,top-18,45,top+2),fill=(122,137,138));d.rectangle((38,top-19,46,top-16),fill=(67,83,90))
        return im
    for k,origin,color in [('R',70,(210,184,140)),('C',80,(100,162,185)),('I',90,(190,158,100))]:
        for stage in range(3):
            for variant in range(2):add(origin+stage*2+variant,f'{k}{stage+1}v{variant}',building(tuple(c+variant*8 for c in color),20+stage*15,variant,k))
    services=create_services(base)
    for sid,(name,im) in services.items():add(sid,name,im)
    service_sheet=Image.new('RGB',(936,880),(24,43,53));sd=ImageDraw.Draw(service_sheet)
    labels=['Usina / transformadores','Bomba e reservatório','Corpo de bombeiros','Delegacia / viatura','Hospital / ambulância','Parque / fonte']
    label_font=ImageFont.truetype(str(root/'assets/fonts/NotoSans-Regular.ttf'),18)
    for n,(sid,(name,im)) in enumerate(services.items()):
        x=n%3*312;y=n//3*440
        scaled=im.resize((256,384),Image.Resampling.NEAREST);service_sheet.paste(scaled,(x+28,y),scaled);sd.text((x+15,y+392),labels[n],font=label_font,fill=(238,231,213))
    service_sheet.save(root/'assets/services-preview.png')
    for sid,name,color in [(110,'selection',(248,229,143)),(111,'invalid',(249,103,102)),(112,'abandoned',(145,149,150)),(113,'fire',(255,146,57))]:
        im=Image.new('RGBA',(64,96));d=ImageDraw.Draw(im);d.polygon([(32,64),(63,79),(32,95),(0,79)],outline=color,width=3)
        if sid==113:d.polygon([(25,80),(20,61),(29,65),(34,41),(46,68),(38,82)],fill=color)
        if sid==112:d.line([(20,63),(44,84)],fill=color,width=4);d.line([(44,63),(20,84)],fill=color,width=4)
        add(sid,name,im)
    # Low, unobstructed markers used by construction mode. They are deliberately
    # different from the thin outlines of empty zoning tiles.
    for sid,name,color,label in [(114,'built-residential',(71,184,103),'R'),
                                 (115,'built-commercial',(67,151,218),'C'),
                                 (116,'built-industrial',(222,164,67),'I'),
                                 (117,'built-service',(192,124,219),'S')]:
        im=base();d=ImageDraw.Draw(im)
        d.polygon([(32,66),(58,79),(32,92),(6,79)],fill=(*color,105),outline=color,width=3)
        d.rectangle((25,72,39,85),fill=(31,54,62),outline=color,width=2)
        d.text((29,73),label,fill=(248,239,210));add(sid,name,im)
    def lot_canvas(im, cells_w, cells_h, size, campus, anchor_y=None):
        """Compose a native-pixel campus across a multi-cell lot."""
        canvas=Image.new('RGBA',size)
        anchor=(size[0]//2,anchor_y if anchor_y is not None else size[1]//2)
        for diag in range(cells_w+cells_h-1):
            for yy in range(cells_h):
                xx=diag-yy
                if not 0<=xx<cells_w: continue
                tile=base(variant=(xx+yy)%3)
                tx=anchor[0]+(xx-yy)*32-32
                ty=anchor[1]+(xx+yy)*16-79
                canvas.alpha_composite(tile,(tx,ty))
        centre_x=(cells_w-cells_h)*16
        centre_y=(cells_w+cells_h-2)*8
        cx=anchor[0]+centre_x;cy=anchor[1]+centre_y
        d=ImageDraw.Draw(canvas)
        def wing(x,bottom,w,depth,height,color):
            shade=tuple(int(c*.68) for c in color)
            light=tuple(min(255,int(c*1.18)) for c in color)
            top=bottom-height;outline=(38,58,66)
            d.polygon([(x-w,top),(x,top+depth),(x,bottom+depth),(x-w,bottom)],fill=shade,outline=outline)
            d.polygon([(x,top+depth),(x+w,top),(x+w,bottom),(x,bottom+depth)],fill=color,outline=outline)
            d.polygon([(x-w,top),(x,top-depth),(x+w,top),(x,top+depth)],fill=light,outline=outline)
        if campus=='primary':
            wing(cx-35,cy-3,18,8,15,(226,183,91));wing(cx+35,cy+15,18,8,15,(226,183,91))
        elif campus=='secondary':
            wing(cx-38,cy-2,20,9,24,(93,151,193));wing(cx+38,cy+18,20,9,24,(93,151,193))
        elif campus=='university':
            wing(cx-55,cy-5,25,11,24,(146,119,171));wing(cx+55,cy+22,25,11,24,(146,119,171))
            wing(cx,cy-18,30,13,22,(160,137,183))
        elif campus=='park':
            d.line([(cx-55,cy-27),(cx+55,cy+28)],fill=(221,194,137),width=5)
            d.line([(cx+55,cy-27),(cx-55,cy+28)],fill=(221,194,137),width=5)
            d.ellipse((cx-53,cy-8,cx-18,cy+12),fill=(62,147,179),outline=(173,216,207))
            d.ellipse((cx+22,cy-15,cx+49,cy+2),fill=(73,139,79),outline=(239,201,99),width=2)
        elif campus=='port':
            wing(cx-45,cy-4,22,10,19,(170,105,68));wing(cx+41,cy+18,23,10,17,(181,118,70))
            d.line([(cx+65,cy+12),(cx+65,cy-33)],fill=(68,76,76),width=4)
            d.line([(cx+65,cy-31),(cx+84,cy-22)],fill=(68,76,76),width=3)
        left=cx-32
        top=cy-79
        canvas.alpha_composite(im,(left,top))
        return canvas,anchor
    civic=create_civic(base)
    for sid,(name,im) in list(civic.items()):
        cells=3 if sid==134 else 2
        size=(192,256) if cells==3 else (128,192)
        campus={132:'primary',133:'secondary',134:'university',135:'park'}[sid]
        im,anchor=lot_canvas(im,cells,cells,size,campus,160 if cells==3 else 128);civic[sid]=(name,im)
        add(sid,name,im,anchor)
    civic_sheet=Image.new('RGB',(1024,430),(24,43,53));cd=ImageDraw.Draw(civic_sheet)
    civic_labels=['Escola primária 2×2','Escola secundária 2×2','Universidade 3×3','Parque municipal 2×2']
    for n,(sid,(name,im)) in enumerate(civic.items()):
        x=n*256;scaled=im.resize((256,384),Image.Resampling.NEAREST)
        civic_sheet.paste(scaled,(x,0),scaled);cd.text((x+12,394),civic_labels[n],font=label_font,fill=(238,231,213))
    civic_sheet.save(root/'assets/civic-preview.png')
    for mask in range(16):
        im=base();d=ImageDraw.Draw(im);d.polygon([(32,68),(51,78),(32,90),(13,80)],fill=(48,58,65))
        for bit,(x,y) in enumerate(centers):
            if mask&(1<<bit):
                ex=32+(x-32)*3//2;ey=79+(y-79)*3//2
                d.line([(32,79),(ex,ey)],fill=(48,58,65),width=23)
                d.line([(32,76),(ex,ey-3)],fill=(238,208,111),width=1)
                d.line([(32,82),(ex,ey+3)],fill=(238,208,111),width=1)
        add(140+mask,'avenue'+str(mask),im)
    transport=create_transport(base,centers)
    for sid,(name,im,anchor) in transport.items():
        if sid==172:
            im,anchor=lot_canvas(im,3,2,(192,192),'port',112)
        add(sid,name,im,anchor)
    # Transparent per-cell boundary for occupied multi-cell footprints. Short
    # segments remain readable over grass and buildings without forming a
    # second opaque ground tile.
    im=Image.new('RGBA',(64,96));d=ImageDraw.Draw(im)
    diamond=[(32,63),(63,79),(32,95),(0,79),(32,63)]
    for a,b in zip(diamond,diamond[1:]):
        for part in (0,2,4):
            p0=part/6;p1=(part+1)/6
            x0=round(a[0]+(b[0]-a[0])*p0);y0=round(a[1]+(b[1]-a[1])*p0)
            x1=round(a[0]+(b[0]-a[0])*p1);y1=round(a[1]+(b[1]-a[1])*p1)
            d.line([(x0,y0),(x1,y1)],fill=(241,218,139,210),width=2)
    add(189,'occupied-grid',im)
    # Simple disaster overlays: storm and tremor. Fire remains sprite 113.
    im=Image.new('RGBA',(64,96));d=ImageDraw.Draw(im)
    d.ellipse((10,47,37,67),fill=(78,99,120,230),outline=(190,208,211))
    d.ellipse((24,41,53,68),fill=(66,86,108,235),outline=(190,208,211))
    for x,y in [(16,70),(29,74),(43,69),(51,77)]:
        d.line([(x,y),(x-4,y+10)],fill=(108,194,224,230),width=2)
    d.polygon([(34,66),(27,79),(34,78),(29,92),(45,73),(38,74),(43,66)],
              fill=(247,210,91),outline=(107,83,50))
    add(190,'disaster-storm',im)
    im=Image.new('RGBA',(64,96));d=ImageDraw.Draw(im)
    d.polygon([(32,63),(63,79),(32,95),(0,79)],fill=(150,117,82,75),outline=(240,180,103),width=2)
    d.line([(31,65),(24,75),(34,78),(25,88),(30,94)],fill=(255,102,76),width=3)
    d.line([(34,79),(46,74),(51,82)],fill=(255,177,82),width=2)
    add(191,'disaster-quake',im)
    for n,label in enumerate(['Via','R','C','I','Energia','Agua','Bombeiro','Policia','Saude','Parque','Demolir','Consulta']):
        im=Image.new('RGBA',(64,96));d=ImageDraw.Draw(im);d.rounded_rectangle((3,35,60,91),radius=8,fill=(39,66,78),outline=(224,196,138),width=2);d.text((8,56),label[:7],fill=(240,229,201));add(120+n,'icon'+label,im)
    sounds={}
    for n,(name,freq) in enumerate([('build',660),('error',180),('event',880)]):
        sounds[200+n]=b''.join(struct.pack('<h',int(4500*math.sin(2*math.pi*freq*i/16000)*(1-i/2400))) for i in range(2400))
    catalog=bytearray();pixels=bytearray();entries=[]
    asset_count=len(sprites)+len(sounds)
    for sid,im in sorted(sprites.items()):
        raw=bytearray()
        for r,g,b,a in im.getdata(): raw+=struct.pack('<HB',((r>>3)<<11)|((g>>2)<<5)|(b>>3),a)
        offset=asset_count*20+len(pixels)
        ax,ay=anchors[sid]
        catalog+=struct.pack('<6H2I',sid,im.width,im.height,ax,ay,1,offset,len(raw));pixels+=raw
        entries.append(dict(id=sid,name=names[sid],width=im.width,height=im.height,anchor=[ax,ay],format='RGB565LE_A8',offset=offset,length=len(raw)))
    for sid,raw in sounds.items():
        offset=asset_count*20+len(pixels);catalog+=struct.pack('<6H2I',sid,2400,1,0,0,2,offset,len(raw));pixels+=raw
        entries.append(dict(id=sid,name=['build','error','event'][sid-200],format='PCM_S16LE_16000_MONO',offset=offset,length=len(raw)))
    payload=catalog+pixels;header=struct.pack('<4s6I32s',b'MCAP',1,1,1,version,asset_count,len(payload),hashlib.sha256(payload).digest());bundle=header+payload
    (out/'microcity.mcap').write_bytes(bundle)
    manifest=dict(package_id='microcity-original',package_version=version,schema_version=1,runtime_format_version=1,engine_min=1,engine_max=1,tile=[64,32],generator_version=GENERATOR,seed=51,total_bytes=len(bundle),sprite_count=len(sprites),asset_count=asset_count,sprites=entries,files=[dict(path='microcity.mcap',size=len(bundle),sha256=hashlib.sha256(bundle).hexdigest())])
    for name,freq in [('build',660),('error',180),('event',880)]:
        with wave.open(str(source/(name+'.wav')),'wb') as f:
            f.setparams((1,2,16000,0,'NONE','not compressed'));f.writeframes(b''.join(struct.pack('<h',int(4500*math.sin(2*math.pi*freq*i/16000)*(1-i/2400))) for i in range(2400)))
    (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    sheet=Image.new('RGB',(8*140,math.ceil(len(sprites)/8)*128),(24,43,53));d=ImageDraw.Draw(sheet)
    for n,(sid,im) in enumerate(sorted(sprites.items())):
        x=n%8*140;y=n//8*128;sheet.paste(im,(x+38,y),im);d.text((x+5,y+99),f'{sid} {names[sid]}',fill=(234,225,201))
    sheet.save(root/'assets/contact-sheet.png')
    scene=Image.new('RGBA',(1080,680),(27,48,57));rng=random.Random(51)
    for diag in range(23):
        for y in range(12):
            x=diag-y
            if not 0<=x<12:continue
            sid=0
            if y==5 or x==6:sid=55
            elif x>9:sid=10
            elif y<4:sid=70+(x+y)%6
            elif y>6:sid=80+(x+y)%6 if x<6 else 90+(x+y)%6
            else:sid=[100,101,102,103,104,105][x%6]
            if x==0 and y%3==0:sid=30+y%3
            im=sprites[sid];scene.alpha_composite(im,(540+(x-y)*32-32,80+(x+y)*16-79))
    scene.convert('RGB').save(root/'assets/demo-scene.png')
    print(f'{len(sprites)} sprites; package {len(bundle)} bytes; decoded RGB565+A8 {len(pixels)} bytes')
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--version',type=int,default=10);a=p.parse_args();generate(Path(__file__).resolve().parents[1],a.version)
