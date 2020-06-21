#include "neat.hpp"
#include "hash_table.hpp"
#include <cstring>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

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

    neat.max_connection_count = max_connections;
    neat.connection_count = 0;
    neat.connections = (gene_connection_t *)malloc(
        sizeof(gene_connection_t) * max_connections);
    memset(neat.connections, 0, sizeof(gene_connection_t) * max_connections);

    neat.connection_finder = (connection_finder_t *)malloc(
        sizeof(connection_finder_t));

    memset(neat.connection_finder, 0, sizeof(connection_finder_t));
    neat.connection_finder->init();

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
        gene->y = (float)i / (float)output_count;
    }
}

static uint64_t s_connection_hash(
    uint32_t gene_from,
    uint32_t gene_to) {
    uint64_t u64_gene_from = (uint64_t)gene_from;
    uint64_t u64_gene_to = (uint64_t)gene_to;

    uint64_t hash = u64_gene_from + (u64_gene_to << 32);

    return hash;
}

gene_connection_t *fetch_gene_connection(
    neat_t *neat,
    uint32_t gene_from,
    uint32_t gene_to) {
    // Create a hash
    uint64_t hash = s_connection_hash(gene_from, gene_to);
    uint32_t *index = neat->connection_finder->get(hash);

    if (index) {
        return &neat->connections[*index];
    }
    else {
        return NULL;
    }
}

genome_t genome_init(
    neat_t *neat) {
    genome_t genome;

    genome.max_gene_count = neat->max_gene_count;
    genome.gene_count = 0;
    genome.genes = (uint32_t *)malloc(
        sizeof(uint32_t) * neat->max_gene_count);

    genome.max_connection_count = neat->max_connection_count;
    genome.connection_count = 0;
    genome.connections = (gene_connection_t *)malloc(
        sizeof(gene_connection_t) * neat->max_connection_count);


    genome.connection_finder = (connection_finder_t *)malloc(
        sizeof(connection_finder_t));

    memset(genome.connection_finder, 0, sizeof(connection_finder_t));
    genome.connection_finder->init();

    // For now, we just need to add the inputs and outputs
    for (uint32_t i = 0; i < neat->gene_count; ++i) {
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
        uint32_t gene_from_i = rand() % genome->gene_count;
        uint32_t gene_to_i = rand() % genome->gene_count;

        uint32_t gene_from = genome->genes[gene_from_i];
        uint32_t gene_to = genome->genes[gene_to_i];

        gene_t *from = &neat->genes[gene_from];
        gene_t *to = &neat->genes[gene_to];

        if (from->x == to->x) {
            continue;
        }

        if (from->x > to->x) {
            // Swap
            uint32_t tmp = gene_from;
            gene_from = gene_to;
            gene_to = tmp;
        }

        uint64_t connection_hash = s_connection_hash(gene_from, gene_to);
        uint32_t *index_ptr = genome->connection_finder->get(connection_hash);

        // If the connection already exists in the genome
        if (index_ptr) {
            continue;
        }
        else {
            // Need to check if the connection has been created in the NEAT
            index_ptr = neat->connection_finder->get(connection_hash);
            if (index_ptr) {
                // Add it to the connections in the genome
                // and add it to the connection finder of the genome
                // WITH the innovation number that was in the NEAT
                uint32_t index = *index_ptr;

                gene_connection_t *src_connection = &neat->connections[index];

                uint32_t new_connection_index = genome->connection_count++;
                genome->connection_finder->insert(
                    connection_hash,
                    new_connection_index);

                gene_connection_t *dst_connection = &genome->connections[
                    new_connection_index];

                memcpy(
                    dst_connection,
                    src_connection,
                    sizeof(gene_connection_t));

                dst_connection->enabled = 1;
                dst_connection->weight = (s_rand_01() * 2.0f - 1.0f) * WEIGHT_RANDOM;
                // No need to set the innovation number because the innovation
                // number will have been set if the connection has been added
                // before (also through the memcpy)
            }
            else {
                // Here we need to add the connection in the NEAT
                // because it is a new one that does not exist in the NEAT
                uint32_t new_connection_index = neat->connection_count++;
                neat->connection_finder->insert(
                    connection_hash,
                    new_connection_index);

                gene_connection_t *src = &neat->connections[new_connection_index];
                src->innovation_number = new_connection_index;
                src->from = gene_from;
                src->to = gene_to;
                src->enabled = 1;

                new_connection_index = genome->connection_count++;
                gene_connection_t *dst = &genome->connections[new_connection_index];
                genome->connection_finder->insert(
                    connection_hash, 
                    new_connection_index);

                memcpy(dst, src, sizeof(gene_connection_t));
                dst->weight = (s_rand_01() * 2.0f - 1.0f) * WEIGHT_RANDOM;
            }

            break;
        }
    }
}

void mutate_add_gene(
    genome_t *genome,
    neat_t *neat) {
    // First get a random connection in the genome
    if (genome->connection_count) {
        uint32_t random_connection = rand() % genome->connection_count;
        gene_connection_t *connection = &genome->connections[random_connection];

        gene_t *dst = &neat->genes[connection->from];
        gene_t *src = &neat->genes[connection->to];

        uint32_t middle_x = (dst->x + src->x) / 2;
        float middle_y = (dst->y + src->y) / 2.0f + s_rand_01() / 5.0f; // Some noise

        uint32_t new_gene_index = neat->gene_count++;
        gene_t *new_gene_ptr = &neat->genes[new_gene_index];

        new_gene_ptr->x = middle_x;
        new_gene_ptr->y = middle_y;

        genome->genes[genome->gene_count++] = new_gene_index;

        // Prev = before the new node, next = after the new node
        gene_connection_t *prev_connection, *next_connection;

        uint32_t prev_connection_index = neat->connection_count++;
        uint64_t prev_connection_hash = s_connection_hash(
            connection->from,
            new_gene_index);
        neat->connection_finder->insert(
            prev_connection_hash,
            prev_connection_index);
        

        uint32_t next_connection_index = neat->connection_count++;
        uint64_t next_connection_hash = s_connection_hash(
            new_gene_index,
            connection->to);
    }
}

void mutate_shift_weight(
    genome_t *genome,
    neat_t *neat) {
    uint32_t random_connection_id = rand() % genome->connection_count;
    gene_connection_t *connection = &genome->connections[random_connection_id];

    connection->weight += (s_rand_01() * 2.0f - 1.0f) * WEIGHT_SHIFT;
}

void mutate_random_weight(
    genome_t *genome,
    neat_t *neat) {
    uint32_t random_connection_id = rand() % genome->connection_count;
    gene_connection_t *connection = &genome->connections[random_connection_id];

    connection->weight = (s_rand_01() * 2.0f - 1.0f) * WEIGHT_RANDOM;
}

void mutate_connection_toggle(
    genome_t *genome,
    neat_t *neat) {
    uint32_t random_connection_id = rand() % genome->connection_count;
    gene_connection_t *connection = &genome->connections[random_connection_id];

    connection->enabled = !connection->enabled;
}
