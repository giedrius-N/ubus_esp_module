LDLIBS = -lserialport -lblobmsg_json -lubox -lubus -luci -largp

BIN:=esp_app
.PHONY: all clean

all: $(BIN)

$(BIN): main.c
	$(CC) -o $@ $^ $(LDLIBS)

clean:
	$(RM) $(BIN)
