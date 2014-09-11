#!/usr/bin/python
#https://infohost.nmt.edu/tcc/help/pubs/pil/formats.html

import sys
import os
from PIL import Image
from PIL import ImageEnhance
size = 32, 32

if len(sys.argv)<2 or len(sys.argv)>4:
	print "Syntax:"
	print "  %s imagefile brightness contrast" % (sys.argv[0])
	quit()
		
print 'Number of arguments:', len(sys.argv), 'arguments.'
print 'Argument List:', str(sys.argv)

filename=os.path.splitext(os.path.basename(sys.argv[1]))[0]

im=Image.open(sys.argv[1]).convert('RGB')

im.thumbnail(size,Image.ANTIALIAS)

#Change the image brightness
bright = ImageEnhance.Brightness(im)
im = bright.enhance(float(sys.argv[2]))

#Change the image contrast
contr = ImageEnhance.Contrast(im)
im = contr.enhance(float(sys.argv[3]))

#Save the image in .ppm format
im.save(filename + ".ppm")

in_file = open(filename + ".ppm","r")
buf = in_file.read()
in_file.close()

out_file = open(filename + ".rgb","w")
out_file.write(buf[13:])
out_file.close()



