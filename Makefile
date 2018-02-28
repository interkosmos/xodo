CC           = gcc8
LDFLAGS      = -I/usr/local/include -L/usr/local/lib
LIBS         = -lX11 -lm
EVAP_MM_PATH = /usr/local/bin

all:
	$(CC) -DP_EVAP_MM_PATH=\"$(EVAP_MM_PATH)\" -c evap/evap.c
	$(CC) $(LDFLAGS) -o xodo xodo.c evap.o $(LIBS)

clean:
	rm xodo evap.o
