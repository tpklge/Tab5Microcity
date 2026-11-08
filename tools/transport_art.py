"""Bridges, port, bounded road traffic, ships and RCI construction markers."""
from PIL import Image, ImageDraw


def create_transport(base, centers):
    out = {}
    for mask in range(16):
        im = base((53, 132, 167)); d = ImageDraw.Draw(im)
        d.polygon([(32, 70), (48, 79), (32, 89), (16, 79)], fill=(79, 78, 75), outline=(205, 199, 169))
        for bit, (x, y) in enumerate(centers):
            if mask & (1 << bit):
                d.line([(32, 79), (x, y)], fill=(79, 78, 75), width=14)
                d.line([(32, 79), (x, y)], fill=(234, 202, 105), width=1)
                d.line([(32, 72), (x, y-6)], fill=(207, 210, 194), width=2)
                d.line([(32, 86), (x, y+6)], fill=(207, 210, 194), width=2)
        out[156 + mask] = ('bridge' + str(mask), im, (32, 79))

    # Port will be scaled to its 3x2 logical footprint by the main generator.
    im = base((96, 140, 105)); d = ImageDraw.Draw(im)
    d.polygon([(5, 76), (29, 64), (59, 79), (34, 92)], fill=(125, 119, 101), outline=(55, 74, 70))
    d.polygon([(32, 67), (61, 80), (47, 88), (20, 75)], fill=(157, 143, 109), outline=(72, 79, 71))
    d.rectangle((10, 47, 33, 71), fill=(170, 105, 68), outline=(52, 64, 67))
    d.polygon([(8, 47), (21, 39), (36, 47), (22, 55)], fill=(205, 139, 75), outline=(72, 65, 58))
    for x, y in [(12, 68), (19, 72), (37, 72), (44, 76)]:
        d.rectangle((x, y, x+7, y+6), fill=(202, 151, 72), outline=(82, 71, 57))
    d.line([(45, 69), (45, 35)], fill=(68, 76, 76), width=3)
    d.line([(45, 37), (58, 43)], fill=(68, 76, 76), width=2)
    d.line([(57, 43), (53, 62)], fill=(68, 76, 76), width=2)
    d.polygon([(36, 82), (56, 72), (62, 76), (42, 87)], fill=(49, 99, 123), outline=(31, 60, 73))
    out[172] = ('port', im, (32, 79))

    colors = [(218, 75, 61), (69, 144, 204), (231, 186, 71), (216, 218, 205)]
    for n, color in enumerate(colors):
        im = Image.new('RGBA', (64, 96)); d = ImageDraw.Draw(im)
        x = 22 + (n % 2) * 5; y = 75 - (n // 2) * 4
        d.polygon([(x, y), (x+12, y-6), (x+22, y-1), (x+10, y+5)], fill=color, outline=(31, 46, 52))
        d.polygon([(x+4, y-3), (x+10, y-7), (x+16, y-4), (x+10, y)], fill=(151, 211, 220))
        d.ellipse((x+3, y+1, x+7, y+5), fill=(27, 35, 40)); d.ellipse((x+16, y-4, x+20, y), fill=(27, 35, 40))
        out[173+n] = ('vehicle'+str(n), im, (32, 79))

    for n, color in enumerate([(213, 218, 202), (194, 111, 72), (73, 143, 181)]):
        im = Image.new('RGBA', (64, 96)); d = ImageDraw.Draw(im)
        d.polygon([(13, 80), (43, 65), (58, 72), (29, 88)], fill=color, outline=(35, 65, 77))
        d.polygon([(24, 69), (39, 62), (49, 67), (34, 75)], fill=(224, 222, 193), outline=(54, 70, 75))
        d.rectangle((34, 55, 37, 66), fill=(66, 73, 73)); d.polygon([(37, 56), (49, 60), (37, 64)], fill=(235, 189, 85))
        d.line([(8, 87), (28, 90), (55, 78)], fill=(163, 215, 219), width=1)
        out[177+n] = ('ship'+str(n), im, (32, 79))

    # Three visibly different occupancy markers for each R/C/I growth stage.
    for category, (name, color, label) in enumerate([
            ('r', (71, 184, 103), 'R'), ('c', (67, 151, 218), 'C'), ('i', (222, 164, 67), 'I')]):
        for stage in range(3):
            im = base(); d = ImageDraw.Draw(im)
            inset = 8 - stage * 2
            d.polygon([(32, 65+inset//2), (61-inset, 79), (32, 93-inset//2), (3+inset, 79)],
                      fill=(*color, 90+stage*35), outline=color, width=2+stage)
            for k in range(stage+1):
                d.rectangle((22+k*8, 75-k*2, 27+k*8, 84-k*2), fill=(31, 54, 62), outline=color)
            d.text((7, 68), label+str(stage+1), fill=(247, 237, 207))
            out[180+category*3+stage] = ('built-'+name+str(stage+1), im, (32, 79))
    return out
