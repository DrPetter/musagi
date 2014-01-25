OBJECTS=DPInput.o fileselector.o Log.o main.o part.o song.o Texture.o

musagi: ${OBJECTS}
	g++ -o $@ $^ -lSDL -lGL -lGLU -lportaudio

%.o: %.cpp
	g++ -c -o $@ $<

clean:
	rm -f ${OBJECTS} musagi
