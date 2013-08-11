#include "bom.h"

#include <CoreFoundation/CFDictionary.h>

#include <stdlib.h>
#include <string.h>

char* get_full_path(bom_file_entry_t *f, CFDictionaryRef files)
{
    bom_file_entry_t *p;
    size_t s1, s2;
    char *ret = NULL;

    if(ntohl(f->parent_index) == 0)
    {
        ret = strdup(f->name);
        return ret;
    }

    p = (bom_file_entry_t*) CFDictionaryGetValue(files, (void*) f->parent_index);
    ret = get_full_path(p, files);
    
    s1 = strlen(ret);
    s2 = strlen(f->name);
    ret = realloc(ret, s1 + s2 + 2);

    strncpy(ret + s1 + 1, f->name, s2);
    ret[s1] = '/';
    ret[s1 + s2 + 1] = '\0';

    return ret;
}

int main(int argc, char *argv[])
{
    bom_var_t *v;
    bom_var_t *var;
    bom_file_t *f;
    bom_file_entry_t **values;
    void const **keys;
    uint32_t count;
    uint32_t i;
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

    if(bom_read_var(f, &var) != ERR_NOERR)
    {
        fprintf(stderr, "Unable to read file\n");
        return EXIT_FAILURE;
    }

    for(v = var; v != NULL; v = v->next)
    {
        if(strncmp(v->name, "Paths", 5) == 0)
        {
            bom_tree_t *tree = NULL;
            bom_path_t *path = NULL;
            bom_indice_t *indices = NULL;
            CFMutableDictionaryRef files;

            bom_get_value_tree(v, f, &tree);
            bom_get_path(f, tree->child, &path, &indices);

            files = CFDictionaryCreateMutable(NULL, path->count, NULL, NULL);

            while(path->leaf == htons(0))
                bom_get_path(f, indices->index0, &path, &indices);

            while(path)
            {
                for(i = 0; i < ntohs(path->count); ++ i)
                {
                    bom_file_entry_t *file;
                    bom_fileinfo1_t *inf;

                    bom_get_file_at_indice(f, indices + i, &file, &inf);
                    CFDictionaryAddValue(files, (void*) inf->id, file);

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

            count = CFDictionaryGetCount(files);

            keys = malloc(sizeof(void*) * count);
            values = malloc(sizeof(bom_file_entry_t*) * count);

            CFDictionaryGetKeysAndValues(files, keys, (void*) values);

            for(i = 0; i < count; ++i)
                printf("%s\n", get_full_path(values[i], files));

            for(i = 0; i < count; ++i)
                free(values[i]);

            free(files);
        }
    }

    bom_var_free(var);
    bom_file_free(f);
}
