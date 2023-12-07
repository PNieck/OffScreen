CXXFLAGS = -std=c++11
LIBS = `pkg-config --libs --cflags egl gl`

main: main.cpp
	g++ $(CXXFLAGS) -o $@ $< $(LIBS)

test: main
	./main

test_without_x11: main
	EGL_PLATFORM=drm ./main

clean:
	rm -f main
