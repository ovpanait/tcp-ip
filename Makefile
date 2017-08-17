SV_OBJECTS = ovsv.o helpers.o 
CL_OBJECTS = ovcl.o helpers.o

image: $(SV_OBJECTS) $(CL_OBJECTS)
	gcc -o server $(SV_OBJECTS)
	gcc -o client $(CL_OBJECTS)

clean:
	rm server client *.o 2> /dev/null

$(SV_OBJECTS) $(CL_OBJECTS): helpers.c
helpers.c : helpers.h
