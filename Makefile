#
# Makefile for non-Microsoft compilers
#	tested only on Linux


include Makefile.config

#Config for TBB

TBBLIB = -ltbb
DAILIB = -ldai -lgmpxx -lgmp
TBBLIB_DEBUG = -ltbb_debug
TBBLIBPATH = include/tbb-core
DAILIBPATH = include/libDai
 

## Main application file
TARGET = src/Main

all: $(TARGET)

# COMPILE
$(TARGET): src/Main $(CSNAP)/Snap.o
	$(CC) $(CXXFLAGS) $(CXXOPENMP) -o $(TARGET) src/Main.cpp src/BP.cpp src/Utilities.cpp $(CSNAP)/Snap.o -I$(DAILIBPATH) -I$(CSNAP) -I$(CGLIB) $(LDFLAGS) -L$(TBBLIBPATH) -L$(DAILIBPATH) $(TBBLIB) $(DAILIB) $(LIBS)
	
$(CSNAP)/Snap.o:
	$(MAKE) -C $(CSNAP)

clean:
	rm -f *.o $(TARGET) *.exe
	rm -rf Debug Release
	rm -rf *.Err demo*.dat
	rm -f src/*.gv
	rm -f src/*.graph
	

