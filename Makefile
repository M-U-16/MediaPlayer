player:
	gcc -o player main.c \
		util.c \
		stream.c \
		decoder.c \
		queue.c \
		player.c \
		audio.c \
		video.c \
		-lavutil \
		-lavformat \
		-lavcodec \
		-lswscale \
		-lswresample \
		-lSDL2 \
		-lSDL2_ttf

debug:
	gcc -o player_d main.c \
		util.c \
		stream.c \
		decoder.c \
		queue.c \
		player.c \
		audio.c \
		video.c \
		-lavutil \
		-lavformat \
		-lavcodec \
		-lswscale \
		-lswresample \
		-lSDL2 \
		-lSDL2_ttf -g

clean:
	del *.exe
default:
	player

.PHONY: clean