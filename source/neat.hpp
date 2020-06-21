#pragma once

#include <stdint.h>
#include "hash_table.hpp"

struct gene_t {
    uint32_t x;
    // y variable just used for display
    float y;
};

typedef uint32_t gene_id_t;

struct gene_connection_t {
    uint32_t from, to;
    uint32_t innovation_number;
    uint32_t enabled;
    float weight;
};

using connection_finder_t = hash_table_t<uint32_t, 2000, 20, 20>;

// Structure which holds information on ALL the genes / gene connections
// that exist at the moment
struct neat_t {
    uint32_t gene_count, max_gene_count;
    gene_t *genes;

    uint32_t connection_count, max_connection_count;
    gene_connection_t *connections;

    connection_finder_t *connection_finder;
};

neat_t neat_init(
    uint32_t max_genes,
    uint32_t max_connection);

void prepare_neat(
    neat_t *neat,
    uint32_t input_count,
    uint32_t output_count);

// If gene connection doesn't exist, NULL gets returned
gene_connection_t *fetch_gene_connection(
    uint32_t gene_from,
    uint32_t gene_to);

struct genome_t {
    // Holds the index into the neat's genes array
    uint32_t gene_count, max_gene_count;
    uint32_t *genes;

    uint32_t connection_count, max_connection_count;
    gene_connection_t *connections;

    connection_finder_t *connection_finder;
};

genome_t genome_init(
    neat_t *neat);

// Simply adds connection between two genes
void mutate_add_connection(
    genome_t *genome,
    neat_t *neat);

// Takes a connection and adds a gene in between the end nodes of the connection
// Weight of the first connection = 1, weight of second = previous value
void mutate_add_gene(
    genome_t *genome,
    neat_t *neat);

void mutate_shift_weight(
    genome_t *genome,
    neat_t *neat);

void mutate_random_weight(
    genome_t *genome,
    neat_t *neat);

void mutate_connection_toggle(
    genome_t *genome,
    neat_t *neat);
