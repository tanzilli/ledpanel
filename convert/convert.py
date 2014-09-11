#https://infohost.nmt.edu/tcc/help/pubs/pil/formats.html

from PIL import Image

Image.open("favicon_1.ico").convert('RGB').save('output.ppm')

in_file = open("output.ppm","r")
buf = in_file.read()
in_file.close()

out_file = open("output_1.rgb","w")
out_file.write(buf[13:])
out_file.close()



