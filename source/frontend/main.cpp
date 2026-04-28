#include "tree.h"
#include "read.h"
#include "tree_logger.h"
#include "verificator.h"
#include "tokenizator.h"
#include "scope.h"

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        fprintf(stderr, "ERROR: Not enough argument");
        return EXIT_FAILURE;
    }

    const char* in_filename = argv[1];
    const char* tree_filename = argv[2];

    Program* prog = Tokenize(in_filename);
    if(!prog) 
    {
        fprintf(stderr, "Error tokenizing file %s\n", in_filename);
        return EXIT_FAILURE;
    }

    Tree* tree = ReadProgramm(prog);

    DestroyProg(prog);

    if(!tree || !tree->root) 
    {
        fprintf(stderr, "Error processing file %s\n", in_filename);
        if(tree)
        {
            TreeDestroy(tree);
        }
        return EXIT_FAILURE;
    }

    SetParents(tree);

    tree = Verify(tree);

    TreeDotDump(tree);

    SaveTree(tree, tree_filename);

    TreeDestroy(tree);

    printf("Frontend complete\n");

    return EXIT_SUCCESS;
}