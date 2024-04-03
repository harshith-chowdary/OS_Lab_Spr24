#include "foothread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NODES 400
#define MAX_LINE_LENGTH 10

typedef struct {
    int value; // User input value (if leaf node)
    int sum;   // Sum of user input values of subtree rooted at this node
    int parent; // Parent node index
    int children; // Children
} TreeNode;

int num_nodes = 0;
TreeNode tree[MAX_NODES]; // Assuming maximum 400 nodes in the tree
foothread_barrier_t barriers[MAX_NODES]; // Barrier to synchronize node threads
foothread_barrier_t common_barrier_start; // Barrier to synchronize all threads
foothread_mutex_t mutex; // Mutex

// Function prototypes
int root;
int node_thread(void *arg);
void initialize_tree(char *filename);
void cleanup();

int main() {
    // Read tree structure from file
    char *filename = "tree.txt";
    initialize_tree(filename);

    // Read user input values for leaf nodes
    printf("\nReading values for leaf nodes:\n\n");
    for (int i = 0; i < num_nodes; ++i) {
        if (tree[i].children == 0) {
            printf("Leaf node   %2d :: Enter a positive integer: ", i);
            scanf("%d", &tree[i].value);

            // printf("Node %d value: %d\n", i, tree[i].value);
        }
        else {
            foothread_barrier_init(&barriers[i], 1 + tree[i].children); // Initialize barrier for this node
        }
    }
    printf("\n");

    foothread_mutex_init(&mutex);

    foothread_barrier_init(&common_barrier_start, num_nodes + 1); // Initialize barrier for all threads

    // Create threads for each node in the tree
    foothread_t threads[num_nodes];
    for (int i = 0; i < num_nodes; i++) {
        foothread_attr_t attr = { FOOTHREAD_JOINABLE, FOOTHREAD_DEFAULT_STACK_SIZE };
        // foothread_attr_t attr = FOOTHREAD_ATTR_INITIALIZER;
        // printf("Creating thread for node => %d\n", i);
        int * arg = malloc(sizeof(*arg));
        *arg = i;
        foothread_create(&threads[i], &attr, node_thread, (void*)arg);
    }

    // foothread_barrier_wait(&common_barrier_start); // Wait for all threads to start
    // printf("All threads started\n");

    foothread_exit();

    // Use this anytime to print the Table
    // print_thread_table();

    foothread_mutex_lock(&mutex);
    // Print total sum at root node
    printf("\nSOL => Sum at root (node %d) = %d\n", root, tree[root].sum);

    // for(int i = 0; i < num_nodes; i++){
    //     printf("Node %d sum: %d\n", i, tree[i].sum);
    // }
    foothread_mutex_unlock(&mutex);

    // Cleanup synchronization resources
    cleanup();

    return 0;
}

// Thread function for each node
int node_thread(void *arg) {
    int node_index = *((int *)arg);
    TreeNode *node = &tree[node_index];

    // foothread_barrier_wait(&common_barrier_start); // Wait for all threads to start

    // foothread_mutex_lock(&mutex);
    // printf("**********************************\n");
    // printf("Thread %d for node %d started\n", gettid(), node_index);

    if (node->children > 0) {
        // printf("sem => %d\n", barriers[node_index].semaphore);

        // printf("Node %d blocked by barrier\n", node_index);

        // printf("----------------------------------\n");
        // foothread_mutex_unlock(&mutex);
        foothread_barrier_wait(&barriers[node_index]); // Wait for all children to finish

        foothread_mutex_lock(&mutex);
        // printf("**********************************\n");
        // printf("Node %d passed the barrier\n", node_index);

        node->sum = node->value;
        for (int i = 0; i < num_nodes; ++i) {
            if (tree[i].parent == node_index) {
                node->sum += tree[i].sum;
            }
        }

        // printf("Node %d sum: %d\n", node_index, node->sum);

        // printf("----------------------------------\n");
        foothread_mutex_unlock(&mutex);
    }
    else{
        // foothread_mutex_lock(&mutex);
        // printf("**********************************\n");
        
        // printf("Node %d is a leaf node\n", node_index);
        node->sum = node->value;
        // printf("Node %d sum: %d\n", node_index, node->sum);

        // printf("----------------------------------\n");
        // foothread_mutex_unlock(&mutex);
    }

    if(node->parent != -1) foothread_barrier_wait(&barriers[node->parent]); // Wait for parent to finish

    if(node->children > 0){
        foothread_mutex_lock(&mutex);
        printf("Internal node  %2d gets the partial sum %3d from its children\n", node_index, node->sum);
        foothread_mutex_unlock(&mutex);
    }

    foothread_exit();

    return 0;
}

// Read tree structure from file
void initialize_tree(char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char line[MAX_LINE_LENGTH];
    fgets(line, sizeof(line), file);
    sscanf(line, "%d", &num_nodes);

    for(int i = 0; i < num_nodes; i++) {
        tree[i].parent = -1;
        tree[i].children = 0;
        tree[i].sum = 0;
        tree[i].value = 0;
    }

    int parent, child;
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%d %d", &child, &parent);

        if(child != parent){
            tree[child].parent = parent;
            tree[parent].children += 1;
        }
        else{
            root = child;
            tree[child].parent = -1;
        }
    }

    fclose(file);
}

// Cleanup synchronization resources
void cleanup() {
    // Destroy mutexe
    foothread_mutex_destroy(&mutex);
    // Destroy barriers
    for (int i = 0; i < num_nodes; ++i) {
        foothread_barrier_destroy(&barriers[i]);
    }
}
