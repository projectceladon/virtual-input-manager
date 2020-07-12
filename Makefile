CFLAGS := -I . -lpthread -Wall -Wpedantic -Wextra
CC := g++

MANAGER_SRCS := vInputManager.cpp vInputDevice.cpp

APPL_SRCS := sendKey.cpp

all : clean vinput-manager sendkey

vinput-manager :
	$(CC) $(MANAGER_SRCS) $(CFLAGS) -o ./bin/vinput-manager

sendkey :
	$(CC) $(APPL_SRCS) $(CFLAGS) -o ./bin/sendkey

clean:
	$(RM) ./bin/vinput-manager ./bin/sendkey
