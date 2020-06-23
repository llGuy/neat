#define GLEW_STATIC

#include <time.h>
#include <cstddef>
#include <math.h>
#include <stdint.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "neat.hpp"

static neat_universe_t universe;

static neat_t neat;
static genome_t genome;

static GLFWwindow *window;
static bool running;

static void s_window_key_callback(
    GLFWwindow *window,
    int key,
    int scancode,
    int action,
    int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_C: {
            mutate_add_connection(&genome, &neat);
        } break;

        case GLFW_KEY_S: {
            mutate_shift_weight(&genome, &neat);
        } break;

        case GLFW_KEY_R: {
            mutate_random_weight(&genome, &neat);
        } break;

        case GLFW_KEY_T: {
            mutate_connection_toggle(&genome, &neat);
        } break;

        case GLFW_KEY_G: {
            mutate_add_gene(&genome, &neat);
        } break;
        }
    }
}

void gl_context_init() {
    glewExperimental = true;

    if (!glfwInit()) {
        printf("ERROR: Failed to initialise GLFW\n");
    }

    window = glfwCreateWindow(1000, 600, "NEAT", NULL, NULL);
    glfwSetKeyCallback(window, s_window_key_callback);

    glfwMakeContextCurrent(window);
    
    if (glewInit() != GLEW_OK) {
        printf("ERROR: Failed to initialise GLEW\n");
    }

    running = 1;

    glPointSize(20.0f);
}

void gl_context_terminate() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void gl_begin_frame() {
    glfwPollEvents();
}

void gl_end_frame() {
    glfwSwapBuffers(window);

    running = !glfwWindowShouldClose(window);
}

int32_t main(
    int argc,
    char *argv[]) {
    neat_module_init();

    srand(time(NULL));

    // Test
    neat = neat_init(1000, 50000);
    prepare_neat(&neat, 6, 3);
    genome = genome_init(&neat);

    // Actual
    universe_init(&universe, 20, 4, 2);
    for (uint32_t i = 0; i < universe.entity_count; ++i) {
        universe.entities[i].score = i;
    }

    end_evaluation_and_evolve(&universe);

    for (uint32_t i = 0; i < universe.entity_count; ++i) {
        universe.entities[i].score = universe.entity_count - i;
    }

    gl_context_init();

    while(running) {
        gl_begin_frame();

        glClear(GL_COLOR_BUFFER_BIT);

        glBegin(GL_LINES);

        for (uint32_t i = 0; i < genome.connections.connection_count; ++i) {
            gene_connection_t *connection = &genome.connections.connections[i];
            gene_t *gene_from = &neat.genes[connection->from];
            gene_t *gene_to = &neat.genes[connection->to];

            if (connection->enabled) {
                glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
            }
            else {
                glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            }

            float x = (float)gene_from->x / (float)neat.max_gene_count - 0.5f;
            float y = gene_from->y - 0.5f;
            glVertex2f(x, y);
            x = (float)gene_to->x / (float)neat.max_gene_count - 0.5f;
            y = gene_to->y - 0.5f;
            glVertex2f(x, y);
        }

        glEnd();

        glBegin(GL_POINTS);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        for (uint32_t i = 0; i < genome.gene_count; ++i) {
            gene_t *gene = &neat.genes[genome.genes[i]];
            float x = (float)gene->x / (float)neat.max_gene_count - 0.5f;
            glVertex2f(x, gene->y - 0.5f);
        }

        glEnd();

        gl_end_frame();
    }

    gl_context_terminate();

    return 0;
}
