from ablib import Pin
from time import sleep

R0 = Pin('J4.8','LOW')
G0 = Pin('J4.10','LOW')
B0 = Pin('J4.12','LOW')
R1 = Pin('J4.14','LOW')
G1 = Pin('J4.24','LOW')
B1 = Pin('J4.26','LOW')

A = Pin('J4.28','LOW')
B = Pin('J4.30','LOW')
C = Pin('J4.32','LOW')
D = Pin('J4.34','HIGH')

CLK = Pin('J4.36','LOW')
STB = Pin('J4.38','HIGH')
OE = Pin('J4.40','HIGH')

def PanelAddr(value):
	OE.high()
	
	A.low()	
	B.low()	
	C.low()	
	D.low()	
	
	if value & 1:
		A.high()	
	if value & 2:
		B.high()	
	if value & 4:
		C.high()	
	if value & 8:
		D.high()	
	
	OE.low()

def PanelClear():
	R0.low()
	R1.low()

	B0.low()
	B1.low()

	G0.low()
	G1.low()

	for i in range(192):
		CLK.on()
		CLK.off()

	STB.low()
	STB.high()


PanelClear()

R0.low()
R1.low()

B0.low()
B1.low()

G0.high()
G1.high()

CLK.on()
CLK.off()

R0.high()
R1.high()

B0.high()
B1.high()

G0.high()
G1.high()

CLK.on()
CLK.off()

R0.high()
R1.high()

B0.low()
B1.low()

G0.low()
G1.low()

CLK.on()
CLK.off()
	
STB.low()
STB.high()


for z in range(100):
	for i in range(16):
		PanelAddr(i)


OE.high()


