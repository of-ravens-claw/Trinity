all:
	make menu
	make exploit

exploit:
	cd payload; make; rm payload.bin.gz; gzip -9 -n -c payload.bin > payload.bin.gz; xxd -i payload.bin.gz > ../eboot/payload.h
	cd eboot; make

menu:
	cd bootstrap; mkdir build; cd build; cmake ..; make; xxd -i bootstrap.self > ../../payload/bootstrap.h

clean:
	-rm -rf bootstrap/build
	-rm payload/bootstrap.h
	-rm payload/payload.bin.gz
	-rm eboot/payload.h
	cd payload; make clean
	cd eboot; make clean
