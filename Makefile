SV_OBJECTS = ovsv.o server_fc.o
CL_OBJECTS = ovcl.o

image: $(SV_OBJECTS) $(CL_OBJECTS)
	gcc -o server $(SV_OBJECTS)
	gcc -o client $(CL_OBJECTS)

clean:
	rm server client *.o 2> /dev/null

$(SV_OBJECTS): common.h ovsv.h
$(CL_OBJECTS): common.h ovcl.h
