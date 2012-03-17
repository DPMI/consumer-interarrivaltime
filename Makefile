CFLAGS+=-Os -g -Wall $(shell pkg-config libcap_utils-0.7 --cflags)
LDFLAGS+=
LIBS=$(shell pkg-config libcap_utils-0.7 --libs)
OBJS=filepktinterarrivaltime.o
TARGET=interarrivaltime

.PHONY: clean

all: $(TARGET)

$(TARGET): $(OBJS)	
	$(CXX) $(LDFLAGS) $(OBJS) $(LIBS) -o $(TARGET)

clean:
	rm -f *.o $(TARGET)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@
