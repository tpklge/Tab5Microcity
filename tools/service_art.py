"""Original isometric utility/service sprites. Same 64x96 canvas and base anchor."""
from PIL import ImageDraw

def block(d,cx,cy,w,depth,height,color):
    outline=(42,65,71)
    top=tuple(min(255,int(c*1.18)) for c in color)
    shade=tuple(int(c*.72) for c in color)
    d.polygon([(cx-w,cy-height),(cx,cy+depth-height),(cx,cy+depth),(cx-w,cy)],fill=shade,outline=outline)
    d.polygon([(cx,cy+depth-height),(cx+w,cy-height),(cx+w,cy),(cx,cy+depth)],fill=color,outline=outline)
    d.polygon([(cx,cy-depth-height),(cx+w,cy-height),(cx,cy+depth-height),(cx-w,cy-height)],fill=top,outline=outline)

def pipe(d,points,width=5,color=(73,167,192)):
    d.line(points,fill=(34,69,84),width=width+2,joint='curve')
    d.line(points,fill=color,width=width,joint='curve')
    d.line([(x-1,y-1) for x,y in points],fill=(159,218,219),width=1,joint='curve')

def vehicle(d,x,y,color,service):
    d.polygon([(x,y-5),(x+15,y-12),(x+25,y-7),(x+10,y+1)],fill=tuple(min(255,c+28) for c in color),outline=(34,57,65))
    d.polygon([(x,y-5),(x+10,y+1),(x+25,y-7),(x+25,y),(x+10,y+8),(x,y+2)],fill=color,outline=(34,57,65))
    d.polygon([(x+3,y-5),(x+9,y-8),(x+14,y-5),(x+9,y-2)],fill=(141,208,216))
    d.ellipse((x+3,y+1,x+8,y+7),fill=(32,46,54));d.ellipse((x+18,y-5,x+23,y+1),fill=(32,46,54))
    d.line([(x+12,y+1),(x+23,y-5)],fill=(246,230,187),width=2)
    d.rectangle((x+13,y-11,x+16,y-9),fill=(246,94,80));d.rectangle((x+17,y-12,x+20,y-10),fill=(87,170,246))
    if service=='fire':
        d.line([(x+9,y-8),(x+20,y-13)],fill=(228,220,196),width=2)
        d.line([(x+10,y-5),(x+21,y-10)],fill=(228,220,196),width=1)
    if service=='health':
        d.line([(x+14,y-2),(x+19,y-4)],fill=(204,68,71),width=2);d.line([(x+16,y-5),(x+16,y)],fill=(204,68,71),width=2)

def create_services(base):
    sprites={}
    # Power: turbine hall, transformer coils, busbar and banded chimney.
    im=base((128,144,126));d=ImageDraw.Draw(im)
    block(d,28,78,23,11,17,(109,139,148))
    d.rectangle((39,24,46,65),fill=(153,162,159),outline=(48,69,74))
    d.ellipse((38,21,47,27),fill=(73,90,95),outline=(184,194,186))
    d.rectangle((39,34,46,39),fill=(195,107,79));d.rectangle((39,48,46,52),fill=(195,107,79))
    d.line([(9,70),(29,80)],fill=(186,206,190),width=2)
    for x,y in [(14,65),(21,68),(28,71)]:
        d.rectangle((x-2,y-8,x+2,y+2),fill=(74,88,97))
        for yy in (y-8,y-5,y-2):d.ellipse((x-3,yy,x+3,yy+2),fill=(191,204,184),outline=(80,109,105))
    d.line([(14,57),(28,63)],fill=(233,190,95),width=2)
    d.polygon([(40,68),(36,75),(40,74),(38,82),(46,71),(41,72),(44,66)],fill=(255,218,93),outline=(118,90,44))
    sprites[100]=('power',im)
    # Water: raised reservoir, visible motor/volute, flanges, valve and delivery pipe.
    im=base((113,151,143));d=ImageDraw.Draw(im)
    d.polygon([(6,78),(32,65),(59,78),(32,92)],fill=(150,170,161),outline=(65,103,100))
    d.line([(36,57),(33,76)],fill=(63,101,113),width=3);d.line([(50,57),(53,76)],fill=(63,101,113),width=3)
    d.line([(34,70),(51,63)],fill=(73,120,130),width=2)
    d.rectangle((32,34,54,57),fill=(91,162,185),outline=(44,89,110))
    d.ellipse((32,51,54,62),fill=(80,146,174),outline=(44,89,110))
    d.ellipse((32,28,54,39),fill=(163,214,218),outline=(44,89,110))
    d.line([(35,43),(51,43)],fill=(181,224,222),width=2)
    pipe(d,[(43,57),(43,69),(30,75),(26,75)])
    d.ellipse((9,66,27,81),fill=(40,107,138),outline=(28,66,90))
    d.ellipse((10,67,22,80),fill=(110,193,207),outline=(33,89,116))
    d.ellipse((13,70,19,76),fill=(39,100,131),outline=(194,231,222))
    block(d,29,71,8,4,7,(67,129,148))
    for x in (25,28,31):d.line([(x,65),(x,72)],fill=(173,213,212))
    pipe(d,[(12,75),(12,78),(12,82)],4)
    d.ellipse((39,64,47,70),outline=(196,82,64),width=2);d.line([(43,63),(43,71)],fill=(210,104,76))
    d.polygon([(43,44),(39,51),(40,54),(44,55),(47,52)],fill=(222,246,233))
    sprites[101]=('waterworks',im)
    # Fire: two garage doors, hose tower and a fire engine with ladder.
    im=base();d=ImageDraw.Draw(im)
    block(d,33,74,23,11,20,(180,84,66));block(d,18,74,7,3,40,(160,86,66))
    d.rectangle((12,39,19,45),fill=(48,62,64),outline=(211,164,110));d.arc((13,40,18,45),180,360,fill=(238,196,100),width=2)
    d.line([(34,62),(53,52)],fill=(236,202,154),width=2)
    for x,y in [(35,67),(45,62)]:
        d.polygon([(x,y),(x+7,y-3),(x+7,y+10),(x,y+14)],fill=(46,64,69),outline=(230,180,126))
        for yy in range(y+2,y+11,3):d.line([(x+1,yy),(x+6,yy-2)],fill=(126,143,141))
    vehicle(d,18,83,(193,63,51),'fire');sprites[102]=('firestation',im)
    # Police: blue precinct, shield badge, communications antenna and patrol car.
    im=base();d=ImageDraw.Draw(im);block(d,32,76,23,11,23,(83,121,159))
    d.line([(10,61),(32,72),(54,61)],fill=(219,227,211),width=3)
    for x,y in [(15,64),(23,68),(38,65),(46,61)]:d.rectangle((x,y,x+4,y+5),fill=(125,196,207))
    d.polygon([(30,45),(37,48),(36,56),(32,61),(27,56),(26,48)],fill=(231,196,99),outline=(65,82,95))
    d.polygon([(32,48),(33,52),(36,52),(34,54),(35,57),(32,55),(29,57),(30,54),(28,52),(31,52)],fill=(72,105,146))
    d.line([(47,50),(47,29)],fill=(146,169,173),width=2);d.line([(42,34),(52,34)],fill=(181,202,194))
    vehicle(d,18,84,(208,219,209),'police');d.line([(29,84),(41,78)],fill=(53,103,162),width=3)
    sprites[103]=('police',im)
    # Hospital: white wards, emergency entrance, roof/facade crosses and ambulance.
    im=base();d=ImageDraw.Draw(im);block(d,30,76,22,11,31,(206,217,204));block(d,47,76,10,5,16,(186,207,196))
    for x,y in [(14,56),(22,60),(36,58),(43,54)]:
        for yy in (y,y+8):d.rectangle((x,yy,x+4,yy+4),fill=(83,162,181))
    d.polygon([(31,37),(35,39),(32,43),(36,45),(33,48),(29,46),(26,50),(22,48),(26,44),(22,42),(25,39),(29,41)],fill=(199,66,67))
    d.rectangle((34,59,37,70),fill=(193,63,67));d.rectangle((30,63,41,66),fill=(193,63,67))
    d.line([(41,75),(53,69)],fill=(211,76,71),width=3)
    vehicle(d,15,83,(225,230,210),'health');sprites[104]=('hospital',im)
    # Park: open paths, fountain, benches, flowers and a small tree.
    im=base((105,159,94));d=ImageDraw.Draw(im)
    d.line([(8,79),(51,79)],fill=(204,184,130),width=3);d.line([(32,66),(32,92)],fill=(204,184,130),width=3)
    d.ellipse((21,72,44,85),fill=(186,198,171),outline=(71,111,97));d.ellipse((24,74,41,82),fill=(77,166,188))
    d.line([(33,78),(33,64)],fill=(160,222,223),width=2);d.arc((26,65,39,79),185,355,fill=(193,237,225),width=1)
    for x,y in [(11,82),(43,72)]:
        d.line([(x,y),(x+9,y-4)],fill=(139,89,57),width=3);d.line([(x+1,y+1),(x+1,y+4)],fill=(69,86,69));d.line([(x+8,y-2),(x+8,y+1)],fill=(69,86,69))
    d.rectangle((10,62,12,75),fill=(108,80,49));d.ellipse((3,48,21,66),fill=(43,115,70),outline=(86,151,76))
    for x,y,color in [(46,85,(231,139,108)),(49,83,(230,196,103)),(17,88,(238,193,115)),(20,87,(218,115,111))]:d.ellipse((x,y,x+2,y+2),fill=color)
    sprites[105]=('park',im)
    return sprites
