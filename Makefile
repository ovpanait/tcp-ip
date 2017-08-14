OBJECTS = ovsv.o ovcl.o

image:
	gcc -o server ovsv.c
	gcc -o client ovcl.c

clean:
	rm server client *.o

$(OBJECTS): ovsrvcli.h
