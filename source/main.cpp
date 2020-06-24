#define GLEW_STATIC

#include <time.h>
#include <cstddef>
#include <math.h>
#include <stdint.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "neat.hpp"

struct bird_t {
    uint32_t neat_entity_index;

    // Each bird will have a different color (shade of gray or something)
    float shade;

    float current_y;
    float velocity_y;
};

static struct {
    // neat_universe_t universe;
    uint32_t bird_count;
    bird_t birds[100];

    // Distance between the pipes
    float pipe_distance;
    float pipe_opening_size;

    // Always 4 pipes at a time
    float pipe_positions[4];
    float opening_centres[4];

    float dt;
} game;

static float s_rand_01() {
    return (float)(rand()) / (float)(RAND_MAX);
}

static void s_update_game() {
    for (uint32_t i = 0; i < 4; ++i) {
        game.pipe_positions[i] -= game.dt * 0.1f;

        if (game.pipe_positions[i] <= -1.0f) {
            game.pipe_positions[i] = 1.0f;
            game.opening_centres[i] = s_rand_01() * 1.6f + 0.2f - 1.0f;
        }
    }

    for (uint32_t i = 0; i < game.bird_count; ++i) {
        bird_t *bird = &game.birds[i];
        bird->velocity_y -= game.dt * 3.0f;

        bird->current_y += bird->velocity_y * game.dt;
    }
}

static void s_render_game() {
    glBegin(GL_LINES);
    for (uint32_t i = 0; i < 4; ++i) {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        glVertex2f(game.pipe_positions[i], 1.0f);
        glVertex2f(game.pipe_positions[i], game.opening_centres[i] + game.pipe_opening_size / 2.0f);

        glVertex2f(game.pipe_positions[i], -1.0f);
        glVertex2f(game.pipe_positions[i], game.opening_centres[i] - game.pipe_opening_size / 2.0f);
    }
    glEnd();

    glBegin(GL_POINTS);
    for (uint32_t i = 0; i < game.bird_count; ++i) {
        bird_t *bird = &game.birds[i];
        glColor4f(bird->shade, bird->shade, bird->shade, bird->shade);
        glVertex2f(-0.9f, bird->current_y);
    }
    glEnd();
}

static void s_game_init() {
    // There will be 20 birds
    // universe_init(&game.universe, 20, 4, 2);

    game.bird_count = 1;
    for (uint32_t i = 0; i < game.bird_count; ++i) {
        game.birds[i].current_y = 0.0f;
        game.birds[i].shade = 1.0f;
        game.birds[i].velocity_y = 0.0f;
    }

    game.pipe_opening_size = 0.3f;
    game.pipe_distance = 0.5f;

    for (uint32_t i = 0; i < 4; ++i) {
        game.pipe_positions[i] = (float)i * game.pipe_distance;
        game.opening_centres[i] = s_rand_01() * 1.6f + 0.2f - 1.0f;
    }
}

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

        case GLFW_KEY_SPACE: {
            game.birds[0].velocity_y = 1.0f;
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
    // neat = neat_init(1000, 50000);
    // prepare_neat(&neat, 6, 3);
    // genome = genome_init(&neat);

    // // Actual
    // universe_init(&universe, 20, 4, 2);

    // float inputs[] = { 1.0f, 0.0f, 3.0f, 0.0f, -1.0f, 2.0f };
    // float outputs[] = { 0.0f, 0.0f, 0.0f };

    // for (uint32_t i = 0; i < universe.entity_count; ++i) {
    //    run_genome(&universe.neat, &universe.entities[i].genome, inputs, outputs);

    //     if (outputs[0] > outputs[1] && outputs[0] > outputs[2]) {
    //         universe.entities[i].score = 10.0f;
    //     }
    //     else {
    //         universe.entities[i].score = 0.0f;
    //     }
    // }

    // end_evaluation_and_evolve(&universe);

    // for (uint32_t i = 0; i < universe.entity_count; ++i) {
    //     universe.entities[i].score = universe.entity_count - i;
    // }

    gl_context_init();

    s_game_init();

    game.dt = 0.0f;

    while(running) {
        float frame_time = glfwGetTime();

        gl_begin_frame();

        glClear(GL_COLOR_BUFFER_BIT);

        // glBegin(GL_LINES);

        // for (uint32_t i = 0; i < genome.connections.connection_count; ++i) {
        //     gene_connection_t *connection = &genome.connections.connections[i];
        //     gene_t *gene_from = &neat.genes[connection->from];
        //     gene_t *gene_to = &neat.genes[connection->to];

        //     if (connection->enabled) {
        //         glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
        //     }
        //     else {
        //         glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
        //     }

        //     float x = (float)gene_from->x / (float)neat.max_gene_count - 0.5f;
        //     float y = gene_from->y - 0.5f;
        //     glVertex2f(x, y);
        //     x = (float)gene_to->x / (float)neat.max_gene_count - 0.5f;
        //     y = gene_to->y - 0.5f;
        //     glVertex2f(x, y);
        // }

        // glEnd();

        // glBegin(GL_POINTS);
        // glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        // for (uint32_t i = 0; i < genome.gene_count; ++i) {
        //     gene_t *gene = &neat.genes[genome.genes[i]];
        //     float x = (float)gene->x / (float)neat.max_gene_count - 0.5f;
        //     glVertex2f(x, gene->y - 0.5f);
        // }

        // glEnd();

        s_render_game();
        s_update_game();

        gl_end_frame();

        frame_time = glfwGetTime() - frame_time;
        game.dt = frame_time;
    }

    gl_context_terminate();

    return 0;
}
