#include "neat.hpp"
#include "hash_table.hpp"
#include <cstring>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

static uint64_t s_connection_hash(
    gene_id_t gene_from,
    gene_id_t gene_to) {
    uint64_t u64_gene_from = (uint64_t)gene_from;
    uint64_t u64_gene_to = (uint64_t)gene_to;

    uint64_t hash = u64_gene_from + (u64_gene_to << 32);

    return hash;
}

void gene_connection_tracker_t::init(
    uint32_t max_connections) {
    max_connection_count = max_connections;
    connection_count = 0;
    connections = (gene_connection_t *)malloc(
        sizeof(gene_connection_t) * max_connections);
    memset(connections, 0, sizeof(gene_connection_t) * max_connections);

    finder = (connection_finder_t *)malloc(
        sizeof(connection_finder_t));

    connections_by_innovation_number = (uint32_t *)malloc(sizeof(uint32_t) * max_connection_count);
    connections_by_position = (uint32_t *)malloc(sizeof(uint32_t) * max_connection_count);

    memset(finder, 0, sizeof(connection_finder_t));
    finder->init();
}

uint32_t gene_connection_tracker_t::add_connection(
    gene_id_t from,
    gene_id_t to) {
    uint32_t new_connection_index = connection_count++;
    gene_connection_t *new_connection_ptr = &connections[new_connection_index];
    connections_by_innovation_number[new_connection_index] = new_connection_index;
    connections_by_position[new_connection_index] = new_connection_index;

    uint64_t hash = s_connection_hash(from, to);
    finder->insert(hash, new_connection_index);

    return new_connection_index;
}

uint32_t gene_connection_tracker_t::add_connection(
    gene_id_t from,
    gene_id_t to,
    uint32_t connection_index) {
    uint32_t new_connection_index = connection_index;
    gene_connection_t *new_connection_ptr = &connections[new_connection_index];

    connections_by_innovation_number[new_connection_index] = new_connection_index;
    connections_by_position[new_connection_index] = new_connection_index;

    uint64_t hash = s_connection_hash(from, to);
    finder->insert(hash, new_connection_index);

    return new_connection_index;
}

uint32_t gene_connection_tracker_t::remove_connection(
    gene_id_t from,
    gene_id_t to) {
    uint64_t hash = s_connection_hash(from, to);
    uint32_t index = *(finder->get(hash));

    // Just remove the hash
    finder->remove(hash);

    return index;
}

gene_connection_t *gene_connection_tracker_t::get(
    uint32_t index) {
    return &connections[index];
}

gene_connection_t *gene_connection_tracker_t::fetch_gene_connection(
    gene_id_t from,
    gene_id_t to) {
    uint64_t hash = s_connection_hash(from, to);
    uint32_t *index = finder->get(hash);

    if (index) {
        return &connections[*index];
    }
    else {
        return NULL;
    }
}

static float s_rand_01() {
    return (float)(rand()) / (float)(RAND_MAX);
}

neat_t neat_init(
    uint32_t max_genes,
    uint32_t max_connections) {
    neat_t neat;

    neat.max_gene_count = max_genes;
    neat.gene_count = 0;
    neat.genes = (gene_t *)malloc(
        sizeof(gene_t) * max_genes);
    memset(neat.genes, 0, sizeof(gene_t) * max_genes);

    neat.connections.init(max_connections);

    return neat;
}

void prepare_neat(
    neat_t *neat,
    uint32_t input_count,
    uint32_t output_count) {
    neat->gene_count = input_count + output_count;

    uint32_t i = 0;
    for (; i < input_count; ++i) {
        gene_t *gene = &neat->genes[i];
        gene->x = 0;
        gene->y = (float)i / (float)input_count;
    }

    for (; i < input_count + output_count; ++i) {
        gene_t *gene = &neat->genes[i];
        gene->x = neat->max_gene_count;
        gene->y = (float)(i - input_count) / (float)output_count;
    }

    neat->input_count = input_count;
    neat->output_count = output_count;
}

genome_t genome_init(
    neat_t *neat) {
    genome_t genome;

    genome.max_gene_count = neat->max_gene_count;
    // For the inputs and outputs
    genome.gene_count = neat->gene_count;
    genome.genes = (uint32_t *)malloc(
        sizeof(uint32_t) * neat->max_gene_count);

    genome.connections.init(neat->connections.max_connection_count);

    // For now, we just need to add the inputs and outputs
    for (uint32_t i = 0; i < neat->input_count + neat->output_count; ++i) {
        genome.genes[i] = i;
    }

    return genome;
}

#define WEIGHT_SHIFT 0.3f
#define WEIGHT_RANDOM 1.0f

void mutate_add_connection(
    genome_t *genome,
    neat_t *neat) {
    // Get two random genes
    // TODO: Must optimise this in the future
    for (uint32_t i = 0; i < 100; ++i) {
        uint32_t index_from = rand() % genome->gene_count;
        uint32_t index_to = rand() % genome->gene_count;

        gene_id_t gene_from_id = genome->genes[index_from];
        gene_id_t gene_to_id = genome->genes[index_to];

        gene_t *from = &neat->genes[gene_from_id];
        gene_t *to = &neat->genes[gene_to_id];

        if (from->x == to->x) {
            continue;
        }

        if (from->x > to->x) {
            // Swap
            gene_id_t tmp = gene_from_id;
            gene_from_id = gene_to_id;
            gene_to_id = tmp;
        }

        gene_connection_t *connection = genome->connections.fetch_gene_connection(
            gene_from_id,
            gene_to_id);

        // If the connection already exists in the genome
        if (connection) {
            continue;
        }
        else {
            // Need to check if the connection has been created in the NEAT
            connection = neat->connections.fetch_gene_connection(
                gene_from_id,
                gene_to_id);

            if (connection) {
                // Add it to the connections in the genome
                // and add it to the connection finder of the genome
                // WITH the innovation number that was in the NEAT
                uint32_t new_connection_index = genome->connections.add_connection(
                    gene_from_id,
                    gene_to_id);

                // Src = connection data that's in the NEAT
                gene_connection_t *src_connection = connection;
                gene_connection_t *dst_connection = genome->connections.get(
                    new_connection_index);

                memcpy(
                    dst_connection,
                    src_connection,
                    sizeof(gene_connection_t));

                dst_connection->enabled = 1;
                dst_connection->weight = (s_rand_01() * 2.0f - 1.0f) * WEIGHT_RANDOM;
                // No need to set the innovation number because the innovation
                // number will have been set if the connection has been added
                // before (also through the memcpy)

                printf("Connection was created before in the NEAT\n");
            }
            else {
                // Here we need to add the connection in the NEAT
                // because it is a new one that does not exist in the NEAT
                uint32_t new_connection_index = neat->connections.add_connection(
                    gene_from_id,
                    gene_to_id);

                gene_connection_t *src = neat->connections.get(new_connection_index);
                src->innovation_number = new_connection_index;
                src->from = gene_from_id;
                src->to = gene_to_id;
                src->enabled = 1;

                new_connection_index = genome->connections.add_connection(
                    gene_from_id,
                    gene_to_id);
                gene_connection_t *dst = genome->connections.get(new_connection_index);

                memcpy(dst, src, sizeof(gene_connection_t));
                dst->weight = (s_rand_01() * 2.0f - 1.0f) * WEIGHT_RANDOM;

                printf("New connection was created\n");
            }

            break;
        }
    }
}

void mutate_add_gene(
    genome_t *genome,
    neat_t *neat) {
    // First get a random connection in the genome
    if (genome->connections.connection_count) {
        uint32_t random_connection = rand() % genome->connections.connection_count;
        gene_connection_t *connection = genome->connections.get(random_connection);

        gene_t *src = &neat->genes[connection->from];
        gene_t *dst = &neat->genes[connection->to];

        uint32_t middle_x = (dst->x + src->x) / 2;
        float middle_y = (dst->y + src->y) / 2.0f + s_rand_01() / 5.0f; // Some noise

        gene_id_t new_gene_id = neat->gene_count++;
        gene_t *new_gene_ptr = &neat->genes[new_gene_id];

        printf("New gene ID: %d - produced from previous connection: %d to %d\n", new_gene_id, connection->from, connection->to);

        new_gene_ptr->x = middle_x;
        new_gene_ptr->y = middle_y;

        genome->genes[genome->gene_count++] = new_gene_id;

        // Prev = before the new node, next = after the new node
        gene_connection_t *prev_connection, *next_connection;

        uint32_t prev_connection_index = neat->connections.add_connection(
            connection->from,
            new_gene_id);
        prev_connection = neat->connections.get(prev_connection_index);
        
        uint32_t next_connection_index = neat->connections.add_connection(
            new_gene_id,
            connection->to);
        next_connection = neat->connections.get(next_connection_index);

        prev_connection->enabled = 1;
        prev_connection->from = connection->from;
        prev_connection->to = new_gene_id;
        prev_connection->weight = 1.0f;
        prev_connection->innovation_number = prev_connection_index;

        next_connection->enabled = 1;
        next_connection->from = new_gene_id;
        next_connection->to = connection->to;
        next_connection->weight = connection->weight;
        next_connection->innovation_number = next_connection_index;

        // Just remove the connection for the genome's connection tracker
        uint32_t removed_index = genome->connections.remove_connection(
            connection->from,
            connection->to);

        uint32_t prev_genome_index = genome->connections.add_connection(
            prev_connection->from,
            prev_connection->to,
            removed_index);

        // Need to remove this gene connection
        memcpy(
            genome->connections.get(prev_genome_index),
            prev_connection,
            sizeof(gene_connection_t));

        uint32_t next_genome_index = genome->connections.add_connection(
            next_connection->from,
            next_connection->to);

        memcpy(
            genome->connections.get(next_genome_index),
            next_connection,
            sizeof(gene_connection_t));
    }
}

void mutate_shift_weight(
    genome_t *genome,
    neat_t *neat) {
    uint32_t random_connection_id = rand() % genome->connections.connection_count;
    gene_connection_t *connection = genome->connections.get(random_connection_id);

    connection->weight += (s_rand_01() * 2.0f - 1.0f) * WEIGHT_SHIFT;
}

void mutate_random_weight(
    genome_t *genome,
    neat_t *neat) {
    uint32_t random_connection_id = rand() % genome->connections.connection_count;
    gene_connection_t *connection = genome->connections.get(random_connection_id);

    connection->weight = (s_rand_01() * 2.0f - 1.0f) * WEIGHT_RANDOM;
}

void mutate_connection_toggle(
    genome_t *genome,
    neat_t *neat) {
    uint32_t random_connection_id = rand() % genome->connections.connection_count;
    gene_connection_t *connection = genome->connections.get(random_connection_id);

    connection->enabled = !connection->enabled;
}

#define MUTATION_PROBABILITY 0.4f

void mutate_genome(
    neat_t *neat,
    genome_t *genome) {
    if (s_rand_01() > MUTATION_PROBABILITY) {
        mutate_add_connection(genome, neat);
    }
    if (s_rand_01() > MUTATION_PROBABILITY) {
        mutate_add_gene(genome, neat);
    }
    if (s_rand_01() > MUTATION_PROBABILITY) {
        mutate_shift_weight(genome, neat);
    }
    if (s_rand_01() > MUTATION_PROBABILITY) {
        mutate_random_weight(genome, neat);
    }
    if (s_rand_01() > MUTATION_PROBABILITY) {
        mutate_connection_toggle(genome, neat);
    }
}

static void s_sort_by_position(
    neat_t *neat,
    genome_t *a) {
    // Sort for the positions (of the output node)
    for (uint32_t i = 0; i < a->connections.connection_count - 1; ++i) {
        uint32_t current_index = a->connections.connections_by_position[i];
        uint32_t next_index = a->connections.connections_by_position[i + 1];

        gene_connection_t *current_connection = a->connections.get(current_index);
        gene_t *current_output_gene = &neat->genes[current_connection->to];
        uint32_t current_x = current_output_gene->x;

        gene_connection_t *next_connection = a->connections.get(next_index);
        gene_t *next_output_gene = &neat->genes[next_connection->to];
        uint32_t next_x = next_output_gene->x;
        
        if (current_x > next_x) {
            uint32_t tmp = current_index;
            a->connections.connections_by_position[i] = next_index;
            a->connections.connections_by_position[i + 1] = tmp;

            // Problem, need to switch
            for (int32_t j = i - 1; j >= 0; ++j) {
                current_index = a->connections.connections_by_position[j];
                next_index = a->connections.connections_by_position[j + 1];

                current_connection = a->connections.get(current_index);
                current_output_gene = &neat->genes[current_connection->to];
                current_x = current_output_gene->x;

                next_connection = a->connections.get(next_index);
                next_output_gene = &neat->genes[next_connection->to];
                next_x = next_output_gene->x;
        
                if (current_x > next_x) {
                    tmp = current_index;
                    a->connections.connections_by_position[j] = next_index;
                    a->connections.connections_by_position[j + 1] = tmp;
                }
                else {
                    break;
                }
            } 
        }
    }
}

static void s_sort_by_innovation_number(
    neat_t *neat,
    genome_t *a) {
    // Sort for the innovation numbers
    for (uint32_t i = 0; i < a->connections.connection_count - 1; ++i) {
        uint32_t current_index = a->connections.connections_by_innovation_number[i];
        uint32_t next_index = a->connections.connections_by_innovation_number[i + 1];

        gene_connection_t *current_connection = a->connections.get(current_index);
        uint32_t current_innovation = current_connection->innovation_number;

        gene_connection_t *next_connection = a->connections.get(next_index);
        uint32_t next_innovation = next_connection->innovation_number;
        
        if (current_innovation > next_innovation) {
            uint32_t tmp = current_index;
            a->connections.connections_by_innovation_number[i] = next_index;
            a->connections.connections_by_innovation_number[i + 1] = tmp;

            // Problem, need to switch
            for (int32_t j = i - 1; j >= 0; ++j) {
                current_index = a->connections.connections_by_innovation_number[j];
                next_index = a->connections.connections_by_innovation_number[j + 1];

                current_connection = a->connections.get(current_index);
                current_innovation= current_connection->innovation_number;

                next_connection = a->connections.get(next_index);
                next_innovation= next_connection->innovation_number;
        
                if (current_innovation > next_innovation) {
                    tmp = current_index;
                    a->connections.connections_by_innovation_number[j] = next_index;
                    a->connections.connections_by_innovation_number[j + 1] = tmp;
                }
                else {
                    break;
                }
            } 
        }
    }
}

#define DISTANCE_FACTOR0 1.0f
#define DISTANCE_FACTOR1 1.0f
#define DISTANCE_FACTOR2 1.0f

float genome_distance(
    genome_t *a,
    genome_t *b) {
    int32_t ahighest_innovation = a->connections.get(a->connections.connection_count - 1)->innovation_number;
    int32_t bhighest_innovation = b->connections.get(b->connections.connection_count - 1)->innovation_number;

    if (ahighest_innovation < bhighest_innovation) {
        genome_t *c = a;
        a = b;
        b = c;
    }

    int32_t aindex = 0;
    int32_t bindex = 0;

    int32_t disjoint_count = 0;
    int32_t excess_count = 0;
    float weight_diff = 0.0f;
    float similar_genes = 0.0f;

    while(
        aindex < a->connections.connection_count &&
        bindex < b->connections.connection_count) {
        gene_connection_t *a_connection = a->connections.get(aindex);
        gene_connection_t *b_connection = b->connections.get(bindex);

        uint32_t ainnovation_number = a_connection->innovation_number;
        uint32_t binnovation_number = b_connection->innovation_number;

        if (ainnovation_number == binnovation_number) {
            similar_genes += 1.0f;
            weight_diff += fabs(a_connection->weight - b_connection->weight);

            ++aindex;
            ++bindex;
        }
        else if (ainnovation_number < binnovation_number) {
            ++disjoint_count;
            ++bindex;
        }
        else {
            ++disjoint_count;
            ++aindex;
        }
    }

    if (similar_genes == 0.0f) {
        weight_diff = 1000000.0f;
    }
    else {
        weight_diff /= similar_genes;
    }
    excess_count = a->connections.connection_count - aindex;

    float n = MAX(a->connections.connection_count, b->connections.connection_count);
    if (n < 20.0f) {
        n = 1.0f;
    }

    return (float)disjoint_count * DISTANCE_FACTOR0 / n + (float)excess_count * DISTANCE_FACTOR1 / n + weight_diff * DISTANCE_FACTOR2;
}

static connection_finder_t duplication_avoider;

genome_t genome_crossover(
    neat_t *neat,
    genome_t *a,
    genome_t *b) {
    genome_t result = genome_init(neat);
    int32_t aindex = 0;
    int32_t bindex = 0;

    while(
        aindex < a->connections.connection_count &&
        bindex < b->connections.connection_count) {
        gene_connection_t *a_connection = a->connections.get(aindex);
        gene_connection_t *b_connection = b->connections.get(bindex);

        uint32_t ainnovation_number = a_connection->innovation_number;
        uint32_t binnovation_number = b_connection->innovation_number;

        if (ainnovation_number == binnovation_number) {
            if (s_rand_01() > 0.5f) {
                uint32_t index = result.connections.add_connection(
                    a_connection->from,
                    a_connection->to);
                gene_connection_t *connection = result.connections.get(index);
                memcpy(connection, a_connection, sizeof(gene_connection_t));
            }
            else {
                uint32_t index = result.connections.add_connection(
                    b_connection->from,
                    b_connection->to);
                gene_connection_t *connection = result.connections.get(index);
                memcpy(connection, b_connection, sizeof(gene_connection_t));
            }

            ++aindex;
            ++bindex;
        }
        else if (ainnovation_number < binnovation_number) {
            ++bindex;

            uint32_t index = result.connections.add_connection(
                b_connection->from,
                b_connection->to);
            gene_connection_t *connection = result.connections.get(index);
            memcpy(connection, b_connection, sizeof(gene_connection_t));
        }
        else {
            ++aindex;

            uint32_t index = result.connections.add_connection(
                a_connection->from,
                a_connection->to);
            gene_connection_t *connection = result.connections.get(index);
            memcpy(connection, a_connection, sizeof(gene_connection_t));
        }
    }

    while(aindex < a->connections.connection_count) {
        gene_connection_t *a_connection = a->connections.get(aindex);

        uint32_t index = result.connections.add_connection(
            a_connection->from,
            a_connection->to);
        gene_connection_t *connection = result.connections.get(index);
        memcpy(connection, a_connection, sizeof(gene_connection_t));
    }

    duplication_avoider.clear();

    // TODO: This won't work - there would be duplication issues - need to resolve now
    for (uint32_t i = 0; i < result.connections.connection_count; ++i) {
        uint32_t from = result.connections.get(i)->from;
        uint32_t to = result.connections.get(i)->to;

        uint32_t *p = duplication_avoider.get(from);

        if (!p) {
            result.genes[result.gene_count++] = from;
        }

        p = duplication_avoider.get(to);
        if (!p) {
            result.genes[result.gene_count++] = to;
        }
    }

    return result;
}

void prepare_genome_for_breed(
    neat_t *neat,
    genome_t *a) {
    s_sort_by_innovation_number(neat, a);
    s_sort_by_position(neat, a);
}

static float s_activation_function(
    float x) {
    return 1.0f / (1.0f + exp(x));
}

struct connection_t {
    uint32_t node_from;
    uint32_t node_to;
};

// Need to create a new sort of structure because an
// activation function needs to be applied to the output
// of each node.
struct node_t {
    gene_id_t gene_id;

    uint32_t x;
    float current_value;

    uint32_t connection_count;
    // May have to increase this in the future
    uint32_t connections[50];
};

static connection_t *dummy_connections;
static node_t *dummy_nodes;
static uint32_t *node_indices;
static connection_finder_t finder;

void run_genome(
    neat_t *neat,
    genome_t *genome,
    float *inputs,
    float *outputs) {
    finder.clear();

    uint32_t node_count = 0;

    // Set all the values in the nodes to 0
    for (uint32_t i = 0; i < genome->gene_count; ++i) {
        finder.insert(genome->genes[i], i);
        dummy_nodes[i].gene_id = genome->genes[i];

        gene_t *gene = &neat->genes[dummy_nodes[i].gene_id];

        dummy_nodes[i].x = gene->x;
        dummy_nodes[i].connection_count = 0;

        if (i < neat->input_count) {
            dummy_nodes[i].current_value = inputs[i];
        }

        // We will sort this later according to the x values
        node_indices[i] = i;
    }

    // Fill the connections:
    for (uint32_t i = 0; i < genome->connections.connection_count; ++i) {
        gene_connection_t *connection = genome->connections.get(i);
        
        uint32_t *node_index = finder.get(connection->to);

        if (!node_index) {
            printf("ERROR: Connection has destination which does not exist in the genome's gene list\n");
            assert(0);
        }

        dummy_nodes[*node_index].connections[dummy_nodes->connection_count++] = i;
    }

    // Sort out in terms of the x values
    for (uint32_t i = 0; i < genome->gene_count - 1; ++i) {
        uint32_t current_node_index = node_indices[i];
        uint32_t current_x = dummy_nodes[current_node_index].x;

        uint32_t next_node_index = node_indices[i + 1];
        uint32_t next_x = dummy_nodes[next_node_index].x;
        
        if (current_x > next_x) {
            uint32_t tmp = current_node_index;
            node_indices[i] = next_node_index;
            node_indices[i + 1] = tmp;

            // Problem, need to switch
            for (int32_t j = i - 1; j >= 0; ++j) {
                current_node_index = node_indices[j];
                current_x = dummy_nodes[current_node_index].x;

                next_node_index = node_indices[j + 1];
                next_x = dummy_nodes[next_node_index].x;
        
                if (current_x > next_x) {
                    tmp = current_node_index;
                    node_indices[j] = next_node_index;
                    node_indices[j + 1] = tmp;
                }
                else {
                    break;
                }
            } 
        }
    }

    // This is not going to work - need to do something creative
    for (uint32_t i = 0; i < genome->gene_count; ++i) {
        node_t *node = &dummy_nodes[node_indices[i]];

        for (uint32_t c = 0; c < node->connection_count; ++c) {
            uint32_t connection_index = node->connections[c];
            gene_connection_t *connection = genome->connections.get(connection_index);

            if (connection->enabled) {
                uint32_t *from_index = finder.get(connection->from);
                uint32_t *to_index = finder.get(connection->to);
                node_t *from = &dummy_nodes[*from_index];
                node_t *to = &dummy_nodes[*to_index];

                if (to != node) {
                    printf("ERROR: WE GOT A PROBLEM\n");
                    assert(0);
                }

                to->current_value += connection->weight * from->current_value;
            }
        }

        node->current_value = s_activation_function(node->current_value);
    }
}

void neat_module_init() {
    dummy_connections = (connection_t *)malloc(sizeof(connection_t) * 50000);
    dummy_nodes = (node_t *)malloc(sizeof(node_t) * 10000);
    node_indices = (uint32_t *)malloc(sizeof(uint32_t) * 10000);
    finder.init();
}

bool add_entity(
    bool do_check,
    neat_entity_t *entity,
    species_t *species) {
    if (species->entity_count == 0) {
        do_check = 0;
    }

    // Representative is just the first one
    if (do_check) {
        if (genome_distance(&entity->genome, &species->entities[0]->genome) < 4.0f) {
            species->entities[species->entity_count++] = entity;
            entity->species = species;
            return true;
        }

        return false;
    }
    else {
        species->entities[species->entity_count++] = entity;
            entity->species = species;
        return true;
    }
}

void force_extinction(
    species_t *species) {
    for (uint32_t i = 0; i < species->entity_count; ++i) {
        species->entities[i]->species= NULL;
    }
}

void score(
    species_t *species) {
    float total = 0.0f;
    for (uint32_t i = 0; i < species->entity_count; ++i) {
        total += species->entities[i]->score;
    }

    species->average_score = total / (float)species->entity_count;
}

void reset_species(
    species_t *species) {
    for (uint32_t i = 0; i < species->entity_count; ++i) {
        species->entities[i]->species = NULL;
    }

    neat_entity_t *random_entity = species->entities[rand() % species->entity_count];

    species->entity_count = 1;

    species->entities[0] = random_entity;
    random_entity->species = species;

    species->average_score = 0.0f;
}

void eliminate_weakest(
    species_t *species) {
    // Need to sort in terms of the score
    if (species->entity_count > 1) {
        for (uint32_t i = 0; i < species->entity_count - 1; ++i) {
            neat_entity_t *a = species->entities[i];
            neat_entity_t *b = species->entities[i + 1];

            if (a->score > b->score) {
                neat_entity_t *tmp = a;
                species->entities[i] = b;
                species->entities[i + 1] = tmp;

                // Problem, need to switch
                for (int32_t j = i - 1; j >= 0; ++j) {
                    a = species->entities[j];
                    b = species->entities[j + 1];
        
                    if (a->score > b->score) {
                        tmp = a;
                        species->entities[j] = b;
                        species->entities[j + 1] = tmp;
                    }
                    else {
                        break;
                    }
                } 
            }
        }

        uint32_t removed_count = 0;
        for (uint32_t i = 0; i < species->entity_count / 2; ++i) {
            neat_entity_t *entity = species->entities[i];

            entity->species = NULL;

            if (i < species->entity_count - 1) {
                // Remove this entity
                uint32_t opposing = species->entity_count - removed_count - 1;
                species->entities[i] = species->entities[opposing];
            }

            ++removed_count;
        }

        species->entity_count -= species->entity_count / 2;
    }
}

genome_t breed_genomes(
    neat_t *neat,
    species_t *species) {
    uint32_t first = rand() % species->entity_count;
    neat_entity_t *a = species->entities[first];

    uint32_t other = rand() % species->entity_count;

    while (other == first) {
        other = rand() % species->entity_count;
    }

    neat_entity_t *b = species->entities[other];

    if (a->score > b->score) {
        return genome_crossover(neat, &a->genome, &b->genome);
    }
    else {
        return genome_crossover(neat, &b->genome, &a->genome);
    }
}

// UNIVERSE CODE //////////////////////////////////////////////////////////////
void universe_init(
    neat_universe_t *universe,
    uint32_t entity_count,
    uint32_t input_count,
    uint32_t output_count) {
    universe->neat = neat_init(500, 5000);
    prepare_neat(&universe->neat, input_count, output_count);

    universe->species_count = 0;
    universe->species = (species_t *)malloc(sizeof(species_t) * entity_count);
    // ^ Maximum amount of species

    universe->entity_count = entity_count;
    universe->entities = (neat_entity_t *)malloc(sizeof(neat_entity_t) * entity_count);

    for (uint32_t i = 0; i < universe->entity_count; ++i) {
        neat_entity_t *entity = &universe->entities[i];

        entity->score = 0.0f;
        entity->genome = genome_init(&universe->neat);
        entity->species = NULL;

        mutate_add_connection(&entity->genome , &universe->neat);
    }
}

species_t *species_init(
    struct neat_universe_t *universe) {
    species_t *species = &universe->species[universe->species_count++];
    species->average_score = 0.0f;
    species->entity_count = 0;
    species->entities = (neat_entity_t **)malloc(sizeof(neat_entity_t *) * universe->entity_count);

    return species;
}

// When evolving:

// - create the species
// --- Reset all the species
// --- for each client - check if it's not the representative of a species, otherwise, add it to a species it's similar to
// --- If it wasn't added to a species, create a new species and make this client the representative
// --- Calculate the scores of the species

// - kill the weakest
// --- Kills 50% (make a variable) of the population (for each species)

// - delete the extinct ones
// --- Iterate through each species
// --- If there are no clients in there, remove it (or if there is only one left)

// - reproduce
// --- Iterate through all the clients - check if it has been killed - if it has, set it to a new one
//     created from breeding a random species and assign it to that species (with do_check = false)

// - mutate
// --- Iterate through all clients and mutate each one
void end_evaluation_and_evolve(
    neat_universe_t *universe) {
    // This will select a new representative
    for (uint32_t i = 0; i < universe->species_count; ++i) {
        reset_species(&universe->species[i]);
    }

    for (uint32_t i = 0; i < universe->entity_count; ++i) {
        neat_entity_t *entity = &universe->entities[i];

        // If it's not the representative of the species
        if (!entity->species) {
            bool added_to_species = 0;
            for (uint32_t j = 0; j < universe->species_count; ++j) {
                if (add_entity(1, entity, &universe->species[j])) {
                    added_to_species = 1;
                    break;
                }
            }

            if (!added_to_species) {
                // Create a new species
                species_t *species = species_init(universe);
                add_entity(0, entity, species);
            }
        }
    }

    for (uint32_t i = 0; i < universe->species_count; ++i) {
        score(&universe->species[i]);
    }

    for (uint32_t i = 0; i < universe->species_count; ++i) {
        eliminate_weakest(&universe->species[i]);
    }

    uint32_t eliminated_species = 0;
    for (uint32_t i = 0; i < universe->species_count; ++i) {
        if (universe->species[i].entity_count <= 1) {
            for (uint32_t e = 0; e < universe->species[i].entity_count; ++e) {
                universe->species[i].entities[e]->species = NULL;
            }

            if (i < universe->species_count - eliminated_species - 1) {
                // Eliminate this species (by swapping)
                uint32_t opposing = universe->species_count - eliminated_species - 1;
                universe->species[i] = universe->species[opposing];
            }

            ++eliminated_species;
        }
    }

    universe->species_count -= eliminated_species;

    for (uint32_t i = 0; i < universe->entity_count; ++i) {
        neat_entity_t *entity = &universe->entities[i];

        if (entity->species == NULL) {
            // Get a random species and breed!
            uint32_t index = rand() % universe->species_count;
            species_t *species = &universe->species[index];

            entity->genome = breed_genomes(&universe->neat, species);
            add_entity(0, entity, species);
        }
    }

    // Mutate all the entities now
    for (uint32_t i = 0; i < universe->entity_count; ++i) {
        mutate_genome(&universe->neat, &universe->entities[i].genome);
    }
}
