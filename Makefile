CFLAGS := -I . -lpthread -Wall -Wpedantic -Wextra
CC := g++

MANAGER_SRCS := vInputManager.cpp vInputDevice.cpp

APPL_SRCS := sendKey.cpp

all : vinput-manager send-key

vinput-manager :
	$(CC) $(MANAGER_SRCS) $(CFLAGS) -o vinput-manager

send-key :
	$(CC) $(APPL_SRCS) $(CFLAGS) -o send-key

clean:
	$(RM) vinput-manager send-key
