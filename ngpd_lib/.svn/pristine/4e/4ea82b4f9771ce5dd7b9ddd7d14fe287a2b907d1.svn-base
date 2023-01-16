all: CFLAGS += $(shell pkg-config --cflags libadq) -DLINUX
all: LIBS += $(shell pkg-config --libs libadq)
all: adqapisimpleexample
debug: CFLAGS += $(shell pkg-config --cflags libadq) -DLINUX
debug: LIBS += $(shell pkg-config --libs libadq)

local: CFLAGS += -DLINUX
local: LIBS += -ladq -ludev -lpthread -lstdc++
local:ADQAPI_simple_example.c
	$(CXX) $(CFLAGS) -I../../Release -o adqapisimpleexample ADQAPI_simple_example.c -L../../source/.libs $(LIBS)

debug:ADQAPI_simple_example.c
	$(CXX) $(CFLAGS) -g -o adqapisimpleexample ADQAPI_simple_example.c $(LIBS)

adqapisimpleexample:ADQAPI_simple_example.c
	$(CXX) $(CFLAGS) -o adqapisimpleexample ADQAPI_simple_example.c $(LIBS)

clean:
	rm -f adqapisimpleexample
