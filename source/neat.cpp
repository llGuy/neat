#include "neat.hpp"
#include "hash_table.hpp"
#include <cstring>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

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

    memset(finder, 0, sizeof(connection_finder_t));
    finder->init();
}

uint32_t gene_connection_tracker_t::add_connection(
    gene_id_t from,
    gene_id_t to) {
    uint32_t new_connection_index = connection_count++;
    gene_connection_t *new_connection_ptr = &connections[new_connection_index];

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

    uint64_t hash = s_connection_hash(from, to);
    finder->insert(hash, new_connection_index);

    return new_connection_index;
}

uint32_t gene_connection_tracker_t::remove_connection(
    gene_id_t from,
    gene_id_t to) {
    uint64_t hash = s_connection_hash(from, to);
    uint32_t *index = finder->get(hash);

    // Just remove the hash
    finder->remove(hash);

    return *index;
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
        gene->y = (float)i / (float)output_count;
    }
}

genome_t genome_init(
    neat_t *neat) {
    genome_t genome;

    genome.max_gene_count = neat->max_gene_count;
    genome.gene_count = 0;
    genome.genes = (uint32_t *)malloc(
        sizeof(uint32_t) * neat->max_gene_count);

    genome.connections.init(neat->connections.max_connection_count);

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

        gene_t *dst = &neat->genes[connection->from];
        gene_t *src = &neat->genes[connection->to];

        uint32_t middle_x = (dst->x + src->x) / 2;
        float middle_y = (dst->y + src->y) / 2.0f + s_rand_01() / 5.0f; // Some noise

        gene_id_t new_gene_id = neat->gene_count++;
        gene_t *new_gene_ptr = &neat->genes[new_gene_id];

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
