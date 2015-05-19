CC	= gcc
#INCLUDE	= -I/usr/include -I/usr/local/include
CFLAGS	= -Wall $(INCLUDE) -Winline -pipe -std=gnu99 -g
#LDFLAGS	= -L/usr/lib 
#LDLIBS	= -lircclient -lmaxminddb -lcurl
#OBJ = geoip.o tidbot.o curl.o cfg.o tell.o tidbit.o manual.o 8ball.o hiscore.o ctcptime.o hangman.o seen.o random.o

OBJ = spvtool.o

all: spvtool

tidbot: $(OBJ)
	@echo [link]
	@$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf *.o spvtool
