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

struct gene_connection_tracker_t {
    connection_finder_t *finder;

    uint32_t connection_count, max_connection_count;
    gene_connection_t *connections;

    // Indices of the connections that are sorted
    uint32_t *connections_by_innovation_number;
    uint32_t *connections_by_position;

    void init(
        uint32_t max_connection_count);

    // This function JUST adds a connection
    uint32_t add_connection(
        gene_id_t from,
        gene_id_t to);

    // This is if the connection index is known
    uint32_t add_connection(
        gene_id_t from,
        gene_id_t to,
        uint32_t connection_index);

    // Returns the index of the removed connection
    uint32_t remove_connection(
        gene_id_t from,
        gene_id_t to);

    // If we know that it exists
    gene_connection_t *get(
        uint32_t index);

    // If we don't know that it exists
    gene_connection_t *fetch_gene_connection(
        gene_id_t from,
        gene_id_t to);
};

// Structure which holds information on ALL the genes / gene connections
// that exist at the moment
struct neat_t {
    uint32_t input_output_count;

    uint32_t gene_count, max_gene_count;
    gene_t *genes;

    gene_connection_tracker_t connections;
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
    gene_id_t *genes;

    gene_connection_tracker_t connections;
};

genome_t genome_init(
    neat_t *neat);

void prepare_genome_for_breed(
    neat_t *neat,
    genome_t *a);

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

// Need the connections to be sorted by innovation number
float genome_distance(
    genome_t *a,
    genome_t *b);
