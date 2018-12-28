release:
	g++ lodepng.cpp main.cpp -ansi -pedantic -Wall -Wextra -g -O3 -std=c++17 -pthread
debug:
	g++ lodepng.cpp main.cpp -ansi -pedantic -Wall -Wextra -g -std=c++17 -pthread
