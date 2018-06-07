
TARGERNAME=liblitelib


sobjects:=$(patsubst %.c,%.s.o,$(wildcard *.c))
aobjects:=$(patsubst %.c,%.a.o,$(wildcard *.c))

all: $(TARGERNAME).so #$(TARGERNAME).a
%.s.o : %.c
	$(CC) -fpic -fPIC  $(CFLAGS) $(PUBFLAGS) $(INC) -c -o $@ $<
#%.a.o : %.c
#	$(CC) $(CFLAGS) $(PUBFLAGS) $(INC)  -c -o $@ $<

#$(TARGERNAME).a: $(aobjects)
#	$(AR)  -rcs $@ $^
$(TARGERNAME).so : $(sobjects)
	$(CC) -shared -Wl,--soname=$(TARGERNAME).so -o $@ $^ $(MPP_LIB)
clean :
	-rm -f *.so *.o *.a

