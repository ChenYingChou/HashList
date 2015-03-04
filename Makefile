# makefile of tqi

CC = gcc -c $(CFLAGS) -g -DUNIX
CPP = g++ -c $(CFLAGS) -g -DUNIX
LINK = g++ $(CFLAGS) $(LDFLAGS) -g -o
##############################################################################
.c.o:
	$(CC) $<

.cc.o:
	$(CPP) $<

##############################################################################
OBJS=hint hstr	# hchr

ALL		: $(OBJS)
	@echo ALL done

clean	:
	@rm -rf $(OBJS) h???.dSYM

####### program ###########################################################
hint 	: hash.cc HashList.h
	g++ $(CFLAGS) $(LDFLAGS) -g -DINTEGER_VER=1 -o $@ hash.cc

hchr 	: hash.cc HashList.h
	g++ $(CFLAGS) $(LDFLAGS) -g -DCHARPTR_VER=1 -o $@ hash.cc

hstr 	: hash.cc HashList.h
	g++ $(CFLAGS) $(LDFLAGS) -g -DSTRING_VER=1 -o $@ hash.cc

htest	: htest.cc
	g++ $(CFLAGS) $(LDFLAGS) -g -o $@ $<

##############################################################################
