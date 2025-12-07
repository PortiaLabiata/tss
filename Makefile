CC:=gcc
CFLAGS:=-Wall -Werror -Wextra -Wpedantic -g
SRCS:=tss.c apps.c tmux.c
OBJS:=${SRCS:.c=.o}
LINK_TARGET:=tss

.PHONY: all
all: $(LINK_TARGET)
	echo 'Done!'

$(LINK_TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(LINK_TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm *.o 
	rm $(LINK_TARGET)
