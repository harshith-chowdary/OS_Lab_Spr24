#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>

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

pthread_mutex_t mutex; // Mutex
pthread_barrier_t barriers[MAX_NODES]; // Barrier to synchronize node threads

// Function prototypes
int node_thread(void *arg);
void initialize_tree(char *filename);
void cleanup();

int main() {
    // Read tree structure from file
    char *filename = "tree.txt";
    initialize_tree(filename);

    // Read user input values for leaf nodes
    printf("Reading values for leaf nodes:\n");
    for (int i = 0; i < num_nodes; ++i) {
        if (tree[i].children == 0) {
            printf("Enter value for node %d: ", i);
            scanf("%d", &tree[i].value);

            printf("Node %d value: %d\n", i, tree[i].value);
        }
    }

    pthread_mutex_init(&mutex, NULL); // Initialize mutex

    // Create threads for each node in the tree
    pthread_t threads[num_nodes];
    for (int i = 0; i < num_nodes; ++i) {
        printf("Creating thread for node => %d\n", i);
        int *arg = malloc(sizeof(*arg));
        *arg = i;
        pthread_create(&threads[i], NULL, (void *)&node_thread, (void*)arg);
    }
    for(int i = 0; i < num_nodes; i++){
        pthread_join(threads[i], NULL);
        printf("Node %d sum: %d\n", i, tree[i].sum);
    }

    // Print total sum at root node
    printf("Total sum: %d\n", tree[0].sum);

    pthread_exit(NULL);

    // Cleanup synchronization resources
    // cleanup();

    return 0;
}

// Thread function for each node
int node_thread(void *arg) {
    int node_index = *((int *)arg);
    TreeNode *node = &tree[node_index];

    pthread_mutex_lock(&mutex);
    // printf("**********************************\n");
    printf("Thread for node %d started\n", node_index);

    if (node->children > 0) {
        printf("Node %d has %d children => Barrier init\n", node_index, node->children);
        pthread_barrier_init(&barriers[node_index], NULL, 1 + node->children); // Initialize barrier for this node

        printf("Node %d blocked by barrier\n", node_index);

        // printf("----------------------------------\n");
        pthread_mutex_unlock(&mutex);
        pthread_barrier_wait(&barriers[node_index]); // Wait for all children to finish

        pthread_mutex_lock(&mutex);
        // printf("**********************************\n");
        printf("Node %d passed the barrier\n", node_index);

        node->sum = node->value;
        for (int i = 0; i < num_nodes; ++i) {
            if (tree[i].parent == node_index) {
                node->sum += tree[i].sum;
            }
        }

        printf("Node %d sum: %d\n", node_index, node->sum);

        // printf("----------------------------------\n");
        pthread_mutex_unlock(&mutex);
    }
    else{
        printf("Node %d is a leaf node\n", node_index);
        node->sum = node->value;
        printf("Node %d sum: %d\n", node_index, node->sum);

        // printf("----------------------------------\n");
        pthread_mutex_unlock(&mutex);
    }

    if(node->parent !=-1) pthread_barrier_wait(&barriers[node->parent]); // Wait for parent to finish

    pthread_exit(NULL);
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
        tree[child].parent = parent;
        tree[parent].children += 1;
    }

    fclose(file);
}

// Cleanup synchronization resources
void cleanup() {
    // Destroy mutexe
    pthread_mutex_destroy(&mutex);
    // Destroy barriers
    for (int i = 0; i < num_nodes; ++i) {
        pthread_barrier_destroy(&barriers[i]);
    }
}
