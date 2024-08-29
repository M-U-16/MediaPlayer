player:
	gcc -o player main.c \
		util.c \
		stream.c \
		queue.c \
		player.c \
		-lavutil \
		-lavformat \
		-lavcodec \
		-lswscale \
		-lswresample \
		-lSDL2 \
		-lSDL2_ttf

clean:
	del *.exe
default:
	player

.PHONY: clean