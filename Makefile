CC   = gcc
CFLAGS = -lwsock32 -lpthread
RM = rm -f

.PHONY: all server client tftp

all: server client tftp
server:server.exe
client:client.exe
tftp:tftp.exe
rebuild:clean all

%.exe: %.c
	$(CC) $< $(CFLAGS) -o $@

clean:
	-@$(RM) *.exe
