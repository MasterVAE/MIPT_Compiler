#ifndef SCOPE_H
#define SCOPE_H

#include "tree.h"
#include "compilator.h"

Nametable* CreateNametable();
Nametable* CopyNametable(Nametable* nametable);
void DestroyNametable(Nametable* nametable);
void ClearNametables(Tree* tree);

int FindVarNameInNametable(Nametable* nametable, const char* variable);
int FindVarScopeInNametable(Nametable* nametable, const char* variable);

size_t FrameSize(TreeNode* node, Compilator* compilator);

void SetNametables(Tree* tree);

int VariableOffcet(TreeNode* node);

#endif // SCOPE_H