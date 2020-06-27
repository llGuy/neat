/*

  Things to test:
        - Running the genome
        - Speciation
        
  Things that work:
        - Crossover
        - Distance function

 */

#define GLEW_STATIC

#include <time.h>
#include <cstddef>
#include <math.h>
#include <stdint.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "neat.hpp"
static bool running;

struct bird_t {
    uint32_t neat_entity_index;

    // Each bird will have a different color (shade of gray or something)
    float shade;

    float current_y;
    float velocity_y;

    float score;
    bool dead;

    float distance = 0.0f;
};

bool to_break = 0;

static struct {
    neat_universe_t universe;

    // neat_universe_t universe;
    uint32_t bird_count;
    bird_t birds[150];

    // Distance between the pipes
    float pipe_distance;
    float pipe_opening_size;

    // Always 4 pipes at a time
    float pipe_positions[4];
    float opening_centres[4];

    float dt;

    int32_t behind_pipe;

    uint32_t generation;

    // Either render game or render ethe genomes
    bool rendering_game;
    uint32_t displayed_genome;

    uint32_t current_bird = 0;
} game;

static float s_rand_01() {
    return (float)(rand()) / (float)(RAND_MAX);
}

#define BIRD_X_POSITION -0.9f

static bool s_update_game() {
    uint32_t closest_pipe = 0;
    float smallest_x = 1.0f;

    for (uint32_t i = 0; i < 4; ++i) {
        game.pipe_positions[i] -= game.dt * 0.1f;

        if (game.pipe_positions[i] <= -1.0f) {
            game.pipe_positions[i] = 1.0f;
            game.opening_centres[i] = s_rand_01() * 1.6f + 0.2f - 1.0f;
        }

        if (i != game.behind_pipe) {
            if (game.pipe_positions[i] < smallest_x) {
                closest_pipe = i;
                smallest_x = game.pipe_positions[i];
            }
        }
    }

    float nearest_opening = game.opening_centres[closest_pipe];

    uint32_t dead_bird_count = 0;

    bool check_pipe = 0;

    for (uint32_t j = 0; j < 4; ++j) {
        if (game.pipe_positions[j] <= BIRD_X_POSITION && game.behind_pipe != j) {
            game.behind_pipe = j;
            check_pipe = 1;

            break;
        }
    }

    bird_t *bird = &game.birds[game.current_bird];

    if (!bird->dead) {
        // y position of the bird, y position of the top opening, y position of the bottom opening
        // x position of the opening
        float inputs[4] = {
            bird->current_y,
            nearest_opening + game.pipe_opening_size / 2.0f,
            nearest_opening - game.pipe_opening_size / 2.0f,
            smallest_x
        };

        float outputs[2] = {
                            0.0f, 0.0f
        };

        // Update
        run_genome(&game.universe.neat, &game.universe.entities[game.current_bird].genome, inputs, outputs);

        if (outputs[0] > outputs[1]) {
            // Jump
            bird->velocity_y = 1.0f;
        }

        bird->velocity_y -= game.dt * 3.0f;

        bird->current_y += bird->velocity_y * game.dt;

        bird->distance += game.dt * 0.01f;

        if (bird->current_y < -1.0f || bird->current_y > 1.0f) {
            // Lost
            bird->dead = 1;

            game.universe.entities[game.current_bird].score = bird->score + bird->distance;

            // printf("Bird %d died with score of %f\n", i, bird->score + bird->distance);

            if (bird->score + bird->distance > 0.05f) {
                printf("Something good happened to bird %d\n", game.current_bird);
            }
        }

        if (check_pipe) {
            // Check if the bird is in between the pipes
            if (bird->current_y < game.opening_centres[game.behind_pipe] + game.pipe_opening_size / 2.0f &&
                bird->current_y > game.opening_centres[game.behind_pipe] - game.pipe_opening_size / 2.0f) {
                bird->score += 100.0f;

                printf("Bird %d just got through a pipe!\n", game.current_bird);

                // printf("Score: %f\n", bird->score);
            }
            else {
                bird->dead = 1;

                if (bird->score + bird->distance > 0.05f) {
                    printf("Something good happened to bird %d\n", game.current_bird);
                }

                game.universe.entities[game.current_bird].score = bird->score + bird->distance;
            }
        }
    }
    else {
        game.current_bird++;
        game.behind_pipe = 0xFFFF;

        for (uint32_t i = 0; i < 4; ++i) {
            game.pipe_positions[i] = (float)i * game.pipe_distance + 1.0f;
            game.opening_centres[i] = (float)i / 8.0f - 0.5f;
            // game.opening_centres[i] = s_rand_01() * 1.6f + 0.2f - 1.0f;
        }
    }

    if (game.current_bird == game.bird_count) {
        uint32_t best_entity = 0;
        float best_score = 0.0f;

        for (uint32_t i = 0; i < game.bird_count; ++i) {
            if (game.universe.entities[i].score > best_score) {
                best_entity = i;
                best_score = game.universe.entities[i].score;
            }
        }

        if (to_break) {
            printf("BREAMING\n");
        }

        printf("Evolving...\n");

        end_evaluation_and_evolve(&game.universe);

        printf("Finished evolving!\n");

        ++game.generation;

        printf("Beginning training generation %d\n", game.generation);

        for (uint32_t i = 0; i < game.bird_count; ++i) {
            game.birds[i].score = 0.0f;
            game.birds[i].current_y = 0.0f;
            game.birds[i].shade = 1.0f;
            game.birds[i].velocity_y = 0.0f;
            game.birds[i].dead = 0;
            game.birds[i].distance = 0.0f;
        }

        game.behind_pipe = 0xFFFF;

        for (uint32_t i = 0; i < 4; ++i) {
            game.pipe_positions[i] = (float)i * game.pipe_distance + 1.0f;
            game.opening_centres[i] = (float)i / 4.0f - 1.0f;
            game.opening_centres[i] = (float)i / 8.0f - 0.5f;
            // game.opening_centres[i] = s_rand_01() * 1.6f + 0.2f - 1.0f;
        }

        game.current_bird = 0;

        return false;
    }

    return true;
}

static void s_render_game() {
    if (game.rendering_game) {
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
        bird_t *bird = &game.birds[game.current_bird];
        if (!bird->dead) {
            glColor4f(0.0f, bird->shade, 0.0f, bird->shade);
            glVertex2f(BIRD_X_POSITION, bird->current_y);
        }
        glEnd();
    }
    else {
        glBegin(GL_LINES);

        genome_t *current_genome = &game.universe.entities[game.displayed_genome].genome;

        for (uint32_t i = 0; i < current_genome->connections.connection_count; ++i) {
            gene_connection_t *connection = &current_genome->connections.connections[i];
            gene_t *gene_from = &game.universe.neat.genes[connection->from];
            gene_t *gene_to = &game.universe.neat.genes[connection->to];

            if (connection->enabled) {
                glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
            }
            else {
                glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
            }

            float x = (float)gene_from->x / (float)game.universe.neat.max_gene_count - 0.5f;
            float y = gene_from->y - 0.5f;
            glVertex2f(x, y);
            x = (float)gene_to->x / (float)game.universe.neat.max_gene_count - 0.5f;
            y = gene_to->y - 0.5f;
            glVertex2f(x, y);
        }

        glEnd();

        glBegin(GL_POINTS);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        for (uint32_t i = 0; i < current_genome->gene_count; ++i) {
            gene_t *gene = &game.universe.neat.genes[current_genome->genes[i]];
            float x = (float)gene->x / (float)game.universe.neat.max_gene_count - 0.5f;
            glVertex2f(x, gene->y - 0.5f);
        }

        glEnd();
    }
    
}

static void s_game_init() {
    // There will be 20 birds
    universe_init(&game.universe, 150, 4, 2);

    game.bird_count = 150;
    game.generation = 0;

    for (uint32_t i = 0; i < game.bird_count; ++i) {
        game.birds[i].current_y = 0.0f;
        game.birds[i].shade = 1.0f;
        game.birds[i].velocity_y = 0.0f;
        game.birds[i].dead = 0;
        game.birds[i].score = 0.0f;
        game.birds[i].distance = 0.0f;
    }

    to_break = 0;

    game.pipe_opening_size = 0.2f;
    game.pipe_distance = 0.5f;

    game.behind_pipe = 0xFFFF;

    for (uint32_t i = 0; i < 4; ++i) {
        game.pipe_positions[i] = (float)i * game.pipe_distance + 1.0f;
        game.opening_centres[i] = s_rand_01() * 1.6f + 0.2f - 1.0f;
        game.opening_centres[i] = (float)i / 8.0f - 0.5f;
    }

    game.rendering_game = 0;
    game.displayed_genome = 0;
}

static neat_universe_t universe;

static neat_t neat;
static genome_t genome;

static GLFWwindow *window;

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
            while (s_update_game()) {
                game.dt = 0.2f;
            }
        } break;

        case GLFW_KEY_ESCAPE: {
            game.rendering_game = !game.rendering_game;
        } break;

        case GLFW_KEY_P: {
            to_break = 1;
            printf("GOING TO BREAK\n");
        } break;

        case GLFW_KEY_RIGHT: {
            game.displayed_genome = (game.displayed_genome + 1) % game.universe.entity_count;
            printf("Displaying genome of %d\n", game.displayed_genome);
        } break;

        case GLFW_KEY_LEFT: {
            game.displayed_genome = (game.displayed_genome - 1) % game.universe.entity_count;
            printf("Displaying genome of %d\n", game.displayed_genome);
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

    glPointSize(10.0f);
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

void test_some_shit() {
    neat_t neat = neat_init(1000, 5000);

    prepare_neat(&neat, 4, 2);
    neat.genes[4].x = 1000000;

    genome_t parent1 = genome_init(&neat);
    genome_t parent2 = genome_init(&neat);

    mutate_add_connection(1, 4, &parent1, &neat);
    mutate_add_connection(2, 4, &parent1, &neat);
    parent1.connections.connections[1].enabled = 0;
    mutate_add_connection(3, 4, &parent1, &neat);
    mutate_add_connection(2, 5, &parent1, &neat);
    mutate_add_connection(5, 4, &parent1, &neat);

    mutate_add_connection(1, 4, &parent2, &neat);
    mutate_add_connection(2, 4, &parent2, &neat);
    parent2.connections.connections[1].enabled = 0;
    mutate_add_connection(3, 4, &parent2, &neat);
    mutate_add_connection(2, 5, &parent2, &neat);
    mutate_add_connection(5, 4, &parent2, &neat);
    parent2.connections.connections[4].enabled = 0;
    mutate_add_gene(5, 4, &parent2, &neat);

    mutate_add_connection(1, 5, &parent1, &neat);

    mutate_add_connection(3, 5, &parent2, &neat);
    mutate_add_connection(1, 6, &parent2, &neat);

    print_genome(&neat, &parent1);
    print_genome(&neat, &parent2);

    neat_entity_t a, b;
    a.genome = parent1;
    b.genome = parent2;
    a.score = 10;
    b.score = 100;

    genome_t offspring = genome_crossover(&neat, &parent2, &parent1);

    print_genome(&neat, &offspring);

    printf("Distance between the parents: %f\n", genome_distance(&parent2, &parent1));
    printf("Distance between the parents: %f\n", genome_distance(&parent1, &parent2));

    float inputs[4] = {
                       1.5f,
                       4.2f,
                       2.3f,
                       3.1f
    };

    float outputs[2] = {
                        0.0f, 0.0f
    };

    run_genome(&neat, &parent1, inputs, outputs);

    printf("Outputs: %f %f\n", outputs[0], outputs[1]);

    run_genome(&neat, &parent2, inputs, outputs);

    printf("Outputs: %f %f\n", outputs[0], outputs[1]);

    neat_universe_t universe = {};
    universe_init(&universe, 3, 4, 2);

    universe.neat = neat;
    universe.entities[0].genome = parent1;
    universe.entities[1].genome = parent2;
    universe.entities[2].genome = offspring;

    universe.entities[0].score = 10.0f;
    universe.entities[1].score = 3.0f;
    universe.entities[2].score = 5.0f;

    end_evaluation_and_evolve(&universe);
}

int32_t main(
    int argc,
    char *argv[]) {
    srand(time(NULL));

    neat_module_init();

    // test_some_shit();

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
        if (game.rendering_game) {
            s_update_game();
        }

        gl_end_frame();

        frame_time = glfwGetTime() - frame_time;
        game.dt = frame_time;
        game.dt = 0.2f;
    }

    gl_context_terminate();

    return 0;
}
