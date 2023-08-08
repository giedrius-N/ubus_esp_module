LDLIBS = -lserialport -lblobmsg_json -lubox -lubus -lutils_modules
INCLUDES := -I../

BIN:=esp_app
SRCS := $(wildcard *.c)
OBJS := $(filter-out main.o, $(SRCS:.c=.o))
MAIN_OBJ := main.o

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS) $(MAIN_OBJ)
	$(CC) -o $@ $(OBJS) $(MAIN_OBJ) $(LDLIBS)

%.o: %.c
	$(CC) -c $(INCLUDES) $< -o $@

$(MAIN_OBJ): main.c
	$(CC) -c $(INCLUDES) $< -o $@

clean:
	$(RM) $(OBJS) $(MAIN_OBJ) $(BIN)