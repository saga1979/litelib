include $(PWD)/../Makefile.param

TARGERNAME=liblitelib
PUBFLAGS+=-DHI_DEBUG  

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
	mv -f  ./$(TARGERNAME).so  $(OUTDIR)
clean :
	-rm -f *.so *.o *.a
#	-rm -f $(OUTDIR)/$(TARGERNAME).*
#install:
#	cp *.h ../include -f
