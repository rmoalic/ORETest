CC = clang
INCPATHS = -I/usr/local/include -I fastore
CFLAGS = -g -Wall -O3 $(INCPATHS) -march=native
LDPATH = -lgmp -lssl -lcrypto -lmariadb -ledit
LDLIBS = -L/usr/local/lib


BUILD = build
TESTS = tests

SRC = 
TESTPROGS = middle


OBJPATHS = $(patsubst %.c,$(BUILD)/%.o, $(SRC)) fastore/build/*.o
TESTPATHS = $(addprefix $(TESTS)/, $(TESTPROGS))

all: $(OBJPATHS) $(TESTPATHS)

obj: $(OBJPATHS)

$(BUILD): fastore/build/ore.o
	mkdir -p $(BUILD)

fastore/build/ore.o:
	cd fastore && make

$(TESTS):
	mkdir -p $(TESTS)

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -o $@ -c $<

$(TESTS)/%: %.c $(OBJPATHS) $(TESTS)
	$(CC) $(CFLAGS) -o $@ $< $(LDPATH) $(OBJPATHS) $(LDLIBS)

clean:
	rm -rf $(BUILD) $(TESTS) *~

clean-all: clean
	cd fastore && $(MAKE) clean

