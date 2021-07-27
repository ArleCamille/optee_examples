PLUGIN_UUID = 693a5d5a-0f1c-4ecf-a617-05804d5c9b0b

PLUGIN			= $(PLUGIN_UUID).plugin
PLUGIN_SRS		= $(wildcard ./*.c)
PLUGIN_OBJ		= $(patsubst %.c, %.o, $(PLUGIN_SRS))
PLUGIN_INCLUDES_DIR	= $(CURDIR) $(TEEC_EXPORT)/include

PLUGIN_INCLUDES		= $(addprefix -I, $(PLUGIN_INCLUDES_DIR))
PLUGIN_CCFLAGS		= -Wall -fPIC
PLUGIN_LDFLAGS		= -shared

.PHONY: all
all: $(PLUGIN)

$(PLUGIN): $(PLUGIN_OBJ)
	$(CROSS_COMPILE)gcc $(PLUGIN_LDFLAGS) $(PLUGIN_OBJ) -o $@

%.o: %.c
	$(CROSS_COMPILE)gcc $(PLUGIN_CCFLAGS) $(PLUGIN_INCLUDES) -c $*.c -o $*.o

.PHONY: clean
clean:
	$(RM) $(PLUGIN_OBJ) $(PLUGIN_UUID).plugin

