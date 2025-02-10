#ifndef COMPUTE_GRAPH_H
#define COMPUTE_GRAPH_H

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#define MAX_DEPS 4      // Maximum dependencies per task
#define MAX_TASKS 16    // Maximum tasks per compute graph

typedef void (*task_func_t)(void *arg);  // Function pointer for task execution

/**
 * @brief Represents a node (task) in the compute graph.
 */
typedef struct graph_node {
    task_func_t func;                     // Task function to execute
    void *arg;                             // Argument passed to the function
    int num_deps;                          // Static: Total dependencies
    atomic_t remaining_deps;               // Dynamic: Number of unfinished dependencies
    struct graph_node *deps[MAX_DEPS];     // List of dependent tasks
} graph_node_t;

/**
 * @brief Represents a full compute graph (DAG).
 */
typedef struct compute_graph {
    int num_nodes;                         // Total number of nodes in the graph
    graph_node_t nodes[MAX_TASKS];         // Array of task nodes
} compute_graph_t;

/**
 * @brief Initializes a task node in the compute graph.
 * 
 * @param node Pointer to the task node.
 * @param func Task function to execute.
 * @param arg Argument for the task function.
 */
static inline void graph_node_init(graph_node_t *node, task_func_t func, void *arg) {
    node->func = func;
    node->arg = arg;
    node->num_deps = 0;
    atomic_set(&node->remaining_deps, 0);
}

/**
 * @brief Adds a dependency to a task node.
 * 
 * @param node Task node that depends on `dep_node`.
 * @param dep_node The dependency node that must complete before `node`.
 */
static inline void graph_node_add_dependency(graph_node_t *node, graph_node_t *dep_node) {
    if (node->num_deps < MAX_DEPS) {
        node->deps[node->num_deps++] = dep_node;
        atomic_inc(&node->remaining_deps); // Increment dependency count
    }
}

/**
 * @brief Resets the compute graph before dispatching a new AoT schedule.
 * 
 * @param graph Pointer to the compute graph.
 */
static inline void reset_compute_graph(compute_graph_t *graph) {
    for (int i = 0; i < graph->num_nodes; i++) {
        atomic_set(&graph->nodes[i].remaining_deps, graph->nodes[i].num_deps);
    }
}

#endif /* COMPUTE_GRAPH_H */
