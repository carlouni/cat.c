all: cat
cat: cat.c
    gcc -Wall -std=c99 -o cat cat.c
.PHONY: clean
clean: ; rm -rf cat
