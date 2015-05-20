CC	= gcc
CFLAGS = -Wall $(INCLUDE) -Winline -pipe -std=gnu99 -g
#INCLUDE =
#LDFLAGS =
#LDLIBS	=

OBJ = spvtool.o

all: spvtool

tidbot: $(OBJ)
	@echo [link]
	@$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

clean:
	rm -rf *.o spvtool
