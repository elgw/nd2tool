# Create man pages and html pages from the md files
# requires groff and pandoc

all: nd2tool.txt

%.txt: %.1
	man ./$< | col -b > $@

%.1: %.md
	pandoc $< -s -t man -o $@
