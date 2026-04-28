#include "tree.h"
#include "tree_logger.h"
#include "compilator.h"

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        fprintf(stderr, "ERROR: Not enough argument");
        return EXIT_FAILURE;
    }

    const char* tree_filename = argv[1];
    const char* out_filename = argv[2];

    Tree* tree = LoadTree(tree_filename);
    if(!tree)
    {
        fprintf(stderr, "Error loading tree from %s\n", tree_filename);

        return EXIT_FAILURE;
    }

    FILE* file = fopen(out_filename, "w+");
    if(!file)
    {
        fprintf(stderr, "Error opening file %s\n", out_filename);
        TreeDestroy(tree);

        return EXIT_FAILURE;
    }

    CompileTree(tree, file);

    fclose(file);

    TreeDestroy(tree);

    printf("Backend complete\n");

    return EXIT_SUCCESS;
}