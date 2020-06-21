#include <stdint.h>

#if 0
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#endif

#include "neat.hpp"

int32_t main(
    int argc,
    char *argv[]) {
    neat_t neat = neat_init(10, 50);

    prepare_neat(&neat, 4, 2);

    

    return 0;
}
