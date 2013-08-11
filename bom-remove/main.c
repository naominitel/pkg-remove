#include "bom.h"

#include <CoreFoundation/CFDictionary.h>

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct file_tree
{
    struct file_tree *next;
    struct file_tree *first_child;

    char const *name;
    void* index;
} file_tree_t;

file_tree_t* arborize(CFMutableDictionaryRef files)
{
    CFMutableDictionaryRef indexes;
    bom_file_entry_t *root;
    bom_file_entry_t **values;
    void **keys;
    uint32_t count;
    uint32_t i;

    /* containts nodes already treated */

    indexes = CFDictionaryCreateMutable(NULL, 0, NULL, NULL);

    /* create tree with first index */

    file_tree_t *tree = malloc(sizeof(file_tree_t));
    root = (bom_file_entry_t*) CFDictionaryGetValue(files, (void*) htonl(1));

    tree->next = NULL;
    tree->first_child = NULL;
    tree->index = (void*) htonl(1);
    tree->name = root->name;

    CFDictionaryRemoveValue(files, (void*) htonl(1));
    CFDictionaryAddValue(indexes, (void*) htonl(1), tree);

    count = CFDictionaryGetCount(files);
    keys = malloc(sizeof(void*) * count);
    values = malloc(sizeof(bom_file_entry_t*) * count);

    CFDictionaryGetKeysAndValues(files, (void*) keys, (void*) values);

    for(i = 0; i < count; ++ i)
    {
        file_tree_t *child = NULL;
        file_tree_t *parent = NULL;
        file_tree_t *new = NULL;
        bom_file_entry_t *file = values[i];
        void *index = keys[i];
        void *parent_index;

        /* some keys may have been removed (dirty?) */

        if(!CFDictionaryContainsKey(files, index))
            continue;

        do
        {
            child = new;
            file = (bom_file_entry_t*) CFDictionaryGetValue(files, index);
            parent_index = (void*) file->parent_index;

            /* create a tree for the file */

            new = malloc(sizeof(file_tree_t));
            new->first_child = child;
            new->next = NULL;
            new->index = index;
            new->name = file->name;

            parent = (file_tree_t*) CFDictionaryGetValue(indexes, parent_index);

            CFDictionaryRemoveValue(files, index);  
            CFDictionaryAddValue(indexes, index, new);
            index = (void*) file->parent_index;
        } while(parent == NULL);

        /* append child */
        new->next = parent->first_child;
        parent->first_child = new;
    }

    return tree;
}

CFMutableDictionaryRef list_files(bom_file_t *f)
{
    bom_var_t *v;
    bom_var_t *var;
    CFMutableDictionaryRef ret;
    uint32_t i;

    if(bom_read_var(f, &var) != ERR_NOERR)
    {
        fprintf(stderr, "Unable to read file\n");
        return NULL;
    }

    for(v = var; v != NULL; v = v->next)
    {
        if(strncmp(v->name, "Paths", 5) == 0)
        {
            bom_tree_t *tree = NULL;
            bom_path_t *path = NULL;
            bom_indice_t *indices = NULL;

            bom_get_value_tree(v, f, &tree);
            bom_get_path(f, tree->child, &path, &indices);

            ret = CFDictionaryCreateMutable(NULL, path->count, NULL, NULL);

            while(path->leaf == htons(0))
                bom_get_path(f, indices->index0, &path, &indices);

            while(path)
            {
                for(i = 0; i < ntohs(path->count); ++ i)
                {
                    bom_file_entry_t *file;
                    bom_fileinfo1_t *inf;

                    bom_get_file_at_indice(f, indices + i, &file, &inf);
                    CFDictionaryAddValue(ret, (void*) inf->id, file);

                    free(inf);
                }

                if(path->forward != htonl(0))
                    bom_get_path(f, path->forward, &path, &indices);

                else
                    path = NULL;
            }

            free(indices);
            free(path);
            free(tree);
        }
    }

    bom_var_free(var);
    return ret;
}

void remove_files(file_tree_t *tree, char *prefix)
{
    struct stat s;
    char *fullpath;
    file_tree_t *child;
    size_t s1 = strlen(prefix);
    size_t s2 = strlen(tree->name);

    fullpath = malloc(s1 + s2 + 2);
    memcpy(fullpath, prefix, s1);
    memcpy(fullpath + s1 + 1, tree->name, s2);
    fullpath[s1 + s2 + 1] = '\0';
    fullpath[s1] = '/';

    lstat(fullpath, &s);

    if(!S_ISDIR(s.st_mode))
    {
        printf("Removing %s\n", fullpath);
        return;
    }

    for(child = tree->first_child; child != NULL; child = child->next)
        remove_files(child, fullpath);

    printf("Removing %s\n", fullpath);
    free(fullpath);
}

int main(int argc, char *argv[])
{
    CFMutableDictionaryRef files;
    bom_file_t *f;
    char *fname;

    if(argc < 2)
    {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    fname = argv[1];

    if(bom_open_file(fname, &f) != ERR_NOERR)
    {
        fprintf(stderr, "Unable to open file\n");
        return EXIT_FAILURE;
    }

    files = list_files(f);
    remove_files(arborize(files), "/");

    free(files);
    bom_file_free(f);
}
