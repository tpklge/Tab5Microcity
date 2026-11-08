"""Distinct isometric sprites for MicroCity education and large civic park."""
from PIL import ImageDraw


def _block(d, cx, bottom, width, depth, height, color):
    outline = (38, 58, 66)
    shade = tuple(int(c * .68) for c in color)
    light = tuple(min(255, int(c * 1.18)) for c in color)
    top = bottom - height
    d.polygon([(cx-width, top), (cx, top+depth), (cx, bottom+depth), (cx-width, bottom)],
              fill=shade, outline=outline)
    d.polygon([(cx, top+depth), (cx+width, top), (cx+width, bottom), (cx, bottom+depth)],
              fill=color, outline=outline)
    d.polygon([(cx-width, top), (cx, top-depth), (cx+width, top), (cx, top+depth)],
              fill=light, outline=outline)


def _tree(d, x, y, scale=1):
    d.rectangle((x-1, y, x+1, y+7*scale), fill=(102, 72, 47))
    d.ellipse((x-6*scale, y-8*scale, x+6*scale, y+4*scale),
              fill=(42, 116, 68), outline=(30, 79, 58))
    d.ellipse((x-3*scale, y-11*scale, x+5*scale, y-2*scale), fill=(65, 143, 72))


def create_civic(base):
    sprites = {}

    # Primary school: warm single-storey building, red roof, flag and playground.
    im = base((113, 153, 103)); d = ImageDraw.Draw(im)
    d.polygon([(7, 78), (31, 66), (58, 79), (33, 92)], fill=(184, 164, 113), outline=(72, 91, 72))
    _block(d, 28, 72, 19, 9, 17, (226, 183, 91))
    d.polygon([(7, 54), (28, 42), (50, 54), (28, 66)], fill=(184, 77, 61), outline=(83, 58, 55))
    for x, y in [(15, 61), (23, 65), (35, 63), (43, 59)]:
        d.rectangle((x, y, x+4, y+5), fill=(102, 184, 204), outline=(52, 87, 96))
    d.rectangle((26, 69, 32, 82), fill=(65, 83, 79)); d.ellipse((27, 72, 28, 73), fill=(239, 208, 105))
    d.line([(51, 49), (51, 72)], fill=(221, 220, 195), width=2)
    d.polygon([(52, 49), (61, 52), (52, 56)], fill=(239, 105, 76))
    d.rectangle((8, 77, 18, 86), outline=(239, 222, 153), width=1)
    d.line([(13, 77), (13, 86), (8, 81), (18, 81)], fill=(239, 222, 153))
    sprites[132] = ('primary-school', im)

    # Secondary school: two floors, clock, broad entrance and sports court.
    im = base((104, 145, 111)); d = ImageDraw.Draw(im)
    d.polygon([(5, 79), (29, 67), (60, 81), (34, 94)], fill=(137, 153, 137), outline=(63, 88, 76))
    _block(d, 30, 72, 22, 10, 29, (93, 151, 193))
    d.polygon([(7, 44), (30, 31), (55, 44), (30, 57)], fill=(78, 111, 147), outline=(42, 65, 76))
    for yy in (53, 63):
        for x in (13, 21, 36, 44):
            d.rectangle((x, yy, x+4, yy+5), fill=(143, 213, 219), outline=(48, 83, 97))
    d.rectangle((26, 67, 35, 83), fill=(49, 69, 79)); d.line([(30, 67), (30, 82)], fill=(157, 190, 192))
    d.ellipse((26, 37, 35, 46), fill=(232, 223, 185), outline=(53, 72, 80))
    d.line([(30, 41), (30, 38), (33, 41)], fill=(67, 72, 72))
    d.rectangle((49, 75, 59, 87), outline=(232, 212, 138), width=1)
    d.ellipse((52, 78, 56, 82), outline=(225, 132, 75)); d.line([(54, 74), (54, 83)], fill=(214, 218, 194))
    sprites[133] = ('secondary-school', im)

    # University: classical central hall, wings, columns and copper dome.
    im = base((112, 143, 111)); d = ImageDraw.Draw(im)
    d.ellipse((5, 75, 60, 91), fill=(37, 67, 59, 145))
    _block(d, 18, 74, 12, 6, 20, (146, 119, 171))
    _block(d, 45, 73, 12, 6, 20, (146, 119, 171))
    _block(d, 32, 72, 15, 7, 31, (174, 151, 191))
    d.polygon([(17, 42), (32, 31), (48, 42), (32, 52)], fill=(211, 193, 215), outline=(57, 60, 72))
    d.polygon([(23, 39), (32, 32), (41, 39)], fill=(116, 169, 143), outline=(57, 78, 73))
    d.ellipse((25, 25, 39, 38), fill=(91, 151, 130), outline=(46, 76, 72))
    d.ellipse((27, 23, 37, 30), fill=(139, 190, 156), outline=(46, 76, 72))
    d.line([(32, 23), (32, 18)], fill=(218, 211, 177)); d.polygon([(32, 18), (38, 20), (32, 22)], fill=(232, 185, 83))
    for x in (24, 29, 35, 40):
        d.rectangle((x, 53, x+2, 73), fill=(229, 220, 198)); d.line([(x, 53), (x+2, 53)], fill=(73, 66, 78))
    for x, y in [(9, 62), (18, 66), (45, 64), (52, 60)]:
        d.rectangle((x, y, x+3, y+4), fill=(136, 205, 211))
    d.rectangle((29, 65, 35, 80), fill=(51, 64, 73))
    sprites[134] = ('university', im)

    # Large park: crossing paths, pond, bridge, gazebo, flower beds and trees.
    im = base((91, 157, 91)); d = ImageDraw.Draw(im)
    d.polygon([(5, 79), (31, 65), (60, 79), (32, 94)], fill=(73, 139, 79), outline=(42, 99, 66))
    d.line([(7, 79), (56, 79)], fill=(221, 194, 137), width=4)
    d.line([(31, 66), (32, 92)], fill=(221, 194, 137), width=4)
    d.ellipse((18, 72, 42, 85), fill=(62, 147, 179), outline=(173, 216, 207))
    d.arc((17, 69, 43, 83), 190, 350, fill=(112, 79, 53), width=3)
    d.line([(19, 76), (41, 76)], fill=(222, 194, 138), width=2)
    d.ellipse((42, 59, 57, 68), fill=(224, 210, 168), outline=(82, 81, 70))
    d.polygon([(41, 60), (50, 53), (59, 60), (50, 66)], fill=(189, 91, 67), outline=(83, 60, 55))
    for x in (44, 49, 55): d.line([(x, 62), (x, 73)], fill=(224, 210, 168), width=2)
    _tree(d, 12, 67); _tree(d, 54, 75); _tree(d, 29, 62)
    for x, y, color in [(9, 86, (235, 135, 116)), (14, 88, (239, 201, 99)),
                        (47, 87, (190, 127, 209)), (52, 85, (237, 151, 107))]:
        d.ellipse((x, y, x+2, y+2), fill=color)
    sprites[135] = ('large-park', im)
    return sprites
