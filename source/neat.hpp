#pragma once

#include <bits/stdint-uintn.h>
#include <stdint.h>
#include "hash_table.hpp"

struct gene_t {
    // y variable just used for display
    float x, y;
    uint32_t innovation_number;
};

struct gene_connection_t {
    uint16_t from, to;
    uint32_t innovation_number;
    uint32_t enabled;
    float weight;
};

using connection_finder_t = hash_table_t<uint32_t, 20000, 20, 20>;

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
    neat_t  *neat,
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
};

genome_t genome_init(
    neat_t *neat);
