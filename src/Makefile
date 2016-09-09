SDL_LIBS = -static -L/mingw32/lib -lSDL_gfx -lSDL_image -lSDL_mixer  -L/mingw32/lib -lvorbisfile -lvorbis -lmingw32 -lSDLmain -lSDL  \
			-lbz2 -lharfbuzz -lglib-2.0 -lintl -liconv -ltiff -ljpeg -llzma -lpng16 -lstdc++ -lwebp -lwinpthread -lz -larchive -lwinmm -lgdi32 -ldxguid \
			-lasprintf -lcharset -lcrypto -lcurl -lexpat -lffi -lFLAC++ -lFLAC -lfontconfig -lformw -lfreeglut_static -lgdbm -lgettextlib -lgettextpo -lgif \
			-lgio-2.0 -lglew32 -lglew32mx -lgmodule-2.0 -lgmp -lgmpxx -lgnurx -lgnutls -lgnutlsxx -lgobject-2.0 -lgthread-2.0 -lhistory -lhogweed -lidn -lisl -ljansson  \
			-ljsoncpp -llua  -llzo2  -lmenuw -lmetalink -lminizip -lmpc -lmpfr -lncurses++w -lncursesw -lnettle -lnghttp2 -logg -lopenal -lpanelw -lphysfs -lpixman-1 \
			-lreadline -lregex -lrtmp -lssh2 -lssl -lsystre -ltasn1 -ltclstub86 -ltermcap -ltheora -ltheoradec -ltheoraenc \
			-ltkstub86 -ltre -lturbojpeg -lvorbisenc -lwebpdecoder -lwebpdemux -lwebpmux -lole32 -lws2_32 -mwindows 

CFLAGS:= -g -O2 -Wall -Wno-unused $(OPTIONS)

CFLAGS += $(shell sdl-config --cflags) -Isprite -I. -Isupport -DINLINE=inline
CFLAGS += -DLINUX 
CFILES:= $(shell find . -name '*.c')
OBJECTS:= $(CFILES:.c=.o)

airstrike: $(OBJECTS)
#	$(CC) -o $@ $(CFLAGS) $(OBJECTS) `sdl-config --libs` -lSDL_image
	$(CC) -o $@ $(CFLAGS) $(OBJECTS) $(SDL_LIBS)

airstrike-sound: $(OBJECTS)
#	$(CC) -o $@ $(CFLAGS) $(OBJECTS) `sdl-config --libs` -lSDL_image -lSDL_mixer
	$(CC) -o $@ $(CFLAGS) $(OBJECTS) $(SDL_LIBS)

# Use the rule below when coding new sprite types
#sprites_autogen.h: sprite_types/*.c
#	../utils/genspritelist.sh $^

clean:
	rm -f `find . -name '*.[od]' -or -name '*~' -or -name '*.bb' -or -name '*.bbg'`

dryclean: 
	rm -f `find . -name '*.[^ch]' -type f` 

#Automatic dependency handling. This is not needed for a simple compile.

#include $(CFILES:.c=.d)
#
#%.d: %.c
#	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
#	sed 's,$(*F)\.o[ :]*,$(@:.d=.o) $@ : ,g' < $@.$$$$ > $@; \
#	rm -f $@.$$$$
