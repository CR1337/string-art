CC = gcc
CFLAGS = -march=native -Wall -Wno-unknown-pragmas -Wno-format-truncation -lm -lpthread
H_FILES = $(wildcard *.h)
C_FILES = $(wildcard *.c)
O_FILES = $(C_FILES:.c=.o)
ELF_NAME = main

all : prod

prod : CFLAGS += -Ofast
prod : main
dev : CFLAGS += -DDEBUG -pg -g -O0 -fsanitize=address
dev : main
verbose : CFLAGS += -DVERBOSE
verbose : dev
images : CFLAGS += -DIMAGES
images : dev
verbose_images : CFLAGS += -DVERBOSE -DIMAGES
verbose_images : dev

main : $(O_FILES)
	$(CC) -o $(ELF_NAME) $^ $(CFLAGS)
	chmod +x $(ELF_NAME)

%.o : %.c $(H_FILES)
	$(CC) -c $< $(CFLAGS)

clean :
	rm -f *.o

clean-all : clean
	rm -f $(ELF_NAME)
