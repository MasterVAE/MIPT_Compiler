#include "stdio.h"

#include "tree.h"
#include "optimizator.h"
#include "tree_logger.h"

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        fprintf(stderr, "ERROR: Not enough argument");
        return EXIT_FAILURE;
    }

    const char* tree_filename = argv[1];

    Tree* tree = LoadTree(tree_filename);
    if(!tree) 
    {
        fprintf(stderr, "Error processing file %s\n", tree_filename);
        return EXIT_FAILURE;
    }

    SetParents(tree);

    tree = OptimizeTree(tree);

    TreeDotDump(tree);

    SaveTree(tree, tree_filename);

    TreeDestroy(tree);

    printf("Midlend complete\n");

    return EXIT_SUCCESS;
}