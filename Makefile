.PHONY: clean

vtf2png: vtf2png.c
	gcc -o vtf2png -lpng vtf2png.c

clean:
	rm vtf2png
