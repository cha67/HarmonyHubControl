# build helloworld executable when user executes "make" 

CC       = $(CROSS_COMPILE)g++
CFLAGS  +=  -c -Wall $(EXTRAFLAGS)
LDFLAGS += -lstdc++
HHC_OBJ  = $(patsubst %.cpp,%.o,$(wildcard *.cpp)) $(patsubst %.cpp,%.o,$(wildcard HarmonyHubAPI/jsoncpp/*.cpp)) $(patsubst %.cpp,%.o,$(wildcard HarmonyHubAPI/*.cpp))
DEPS     = $(wildcard *.h) $(wildcard jsoncpp/*.h)


HarmonyHubControl : $(HHC_OBJ)
	$(CC) $(HHC_OBJ) $(LDFLAGS) -o HarmonyHubControl

%.o: %.cpp $(DEP)
	$(CC) $(CFLAGS) $(EXTRAFLAGS) $< -o $@

#HarmonyHubControl.o: HarmonyHubControl.cpp 
#	$(CC) $(CFLAGS) -c HarmonyHubControl.cpp csocket.cpp -lstdc++ 

# remove object files and executable when user executes "make clean"
clean:
	rm $(HHC_OBJ) HarmonyHubControl 
