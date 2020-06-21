#include "neat.hpp"
#include "hash_table.hpp"
#include <cstring>
#include <stdlib.h>
#include <memory.h>

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

    neat.connection_finder = (connection_finder_t *)malloc(sizeof(connection_finder_t));
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
        gene->x = 0.0f;
        gene->y = (float)i / (float)input_count;

        gene->innovation_number = 1;
    }

    for (; i < input_count + output_count; ++i) {
        gene_t *gene = &neat->genes[i];
        gene->x = 1.0f;
        gene->y = (float)i / (float)output_count;

        gene->innovation_number = 1;
    }
}

gene_connection_t *fetch_gene_connection(
    neat_t *neat,
    uint32_t gene_from,
    uint32_t gene_to) {
    // Create a hash
    uint64_t u64_gene_from = (uint64_t)gene_from;
    uint64_t u64_gene_to = (uint64_t)gene_to;

    uint64_t hash = u64_gene_from + (u64_gene_to << 32);

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

    // For now, we just need to add the inputs and outputs
    for (uint32_t i = 0; i < neat->gene_count; ++i) {
        genome.genes[i] = i;
    }

    return genome;
}
