CFLAGS := -I . -lpthread -Wall -Wpedantic -Wextra
CC := clang++

MANAGER_SRCS := vInputManager.cpp vInputDevice.cpp

LG_MANAGER_SRCS := lgInputManager.cpp lgInputDevice.cpp

APPL_SRCS := sendKey.cpp

all : clean sendkey lg-input-manager vinput-manager

vinput-manager :
	$(CC) $(MANAGER_SRCS) $(CFLAGS) -o vinput-manager

lg-input-manager :
	$(CC) $(LG_MANAGER_SRCS) $(CFLAGS) -o lg-input-manager

sendkey :
	$(CC) $(APPL_SRCS) $(CFLAGS) -o sendkey

clean:
	$(RM) vinput-manager sendkey lg-input-manager
