CFLAGS := -I . -lpthread -Wall -Wpedantic -Wextra
CC := g++

MANAGER_SRCS := vInputManager.cpp vInputDevice.cpp

APPL_SRCS := sendKey.cpp

all : clean vinput-manager sendkey

vinput-manager :
	$(CC) $(MANAGER_SRCS) $(CFLAGS) -o vinput-manager

sendkey :
	$(CC) $(APPL_SRCS) $(CFLAGS) -o sendkey

clean:
	$(RM) vinput-manager sendkey
