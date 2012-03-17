CFLAGS+=-Os -g -Wall $(shell pkg-config libcap_utils-0.7 --cflags)
LDFLAGS+=
LIBS=$(shell pkg-config libcap_utils-0.7 --libs)
OBJS=main.o
TARGET=interarrivaltime

.PHONY: clean

all: $(TARGET)

$(TARGET): $(OBJS)	
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $(TARGET)

clean:
	rm -f *.o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
