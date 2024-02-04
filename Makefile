CFLAGS=-g
TARGET=pimodem
OBJDIR=obj
OBJS = $(addprefix $(OBJDIR)/, pimodem.o terminal.o hayes.o utils.o)
DEPS := $(OBJS:.o=.d)
_dummy := $(shell mkdir -p obj)

all: $(TARGET)

-include $(DEPS)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

obj/%.o: %.c
	$(CC) $(CFLAGS) -MMD -c -o $@ $<

clean:
	rm -rf $(OBJDIR) $(TARGET)


