#include "bom.h"

#include <CoreFoundation/CFDictionary.h>

#include <stdlib.h>
#include <string.h>

#define ALLOC_CHECK(expr) if((expr) == NULL) \
    { \
        perror("Unable to allocate memory"); \
        return ERR_MALLOC; \
    }
#define FREAD_CHECK(expr, count) if(expr != count) \
    { \
        perror("Unable to read file"); \
        return ERR_MALLOC; \
    }
#define ERR_CHECK(v, expr) if((v = expr) != ERR_NOERR) return v;

int bom_open_file(char const *path, bom_file_t **f)
{
    ALLOC_CHECK(*f = malloc(sizeof(bom_file_t)))

    (*f)->header = NULL;
    (*f)->index_header = NULL;
    (*f)->var_header = NULL;
    (*f)->index_section = NULL;
    (*f)->fd = fopen(path, "rb");

    if((*f)->fd == NULL)
    {
        perror("Error: unable to open BOM file");
        return ERR_FILERR;
    }

    return ERR_NOERR;
}

int bom_read_header(bom_file_t *f)
{
    if(f->fd == NULL)
        fprintf(stderr, "Invalid BOM file\n");

    ALLOC_CHECK(f->header = malloc(sizeof(bom_header_t)))

    if(fseek(f->fd, 0L, SEEK_SET) != 0)
    {
        perror("Error");
        return ERR_FILERR;
    }

    FREAD_CHECK(fread((void*) f->header, sizeof(bom_header_t), 1, f->fd), 1)
    return ERR_NOERR;
}

int bom_read_index(bom_file_t *f)
{
    uint32_t index_offset;
    uint32_t index_size;
    int err;

    if(f->header == NULL)
        ERR_CHECK(err, bom_read_header(f))

    ALLOC_CHECK(f->index_header = malloc(sizeof(bom_index_header_t)))

    index_offset = ntohl(f->header->index_header_offset);
    index_size = ntohl(f->header->index_header_size) - sizeof(bom_index_header_t);

    fseek(f->fd, index_offset, SEEK_SET);
    fread((void*) f->index_header, sizeof(bom_index_header_t), 1, f->fd);

    f->index_section = malloc(index_size);
    fread((void*) f->index_section, index_size, 1, f->fd);

    return ERR_NOERR;
}

int bom_read_var(bom_file_t *f, bom_var_t **ret)
{
    uint32_t c;
    uint32_t i = 0;
    int err;

    if(f->fd == NULL)
    {
        fprintf(stderr, "Invalid BOM file\n");
        return ERR_INVALID;
    }

    if(f->header == NULL)
        ERR_CHECK(err, bom_read_header(f))

    if(f->var_header == NULL)
    {
        /* read var section header */

        uint32_t var_hdr_off = ntohl(f->header->var_header_offset);
        ALLOC_CHECK(f->var_header = malloc(sizeof(bom_var_header_t)))

        fseek(f->fd, var_hdr_off, SEEK_SET);
        fread((void*) f->var_header, sizeof(bom_var_header_t), 1, f->fd);

        if(strncmp(f->header->magic, "BOMStore", 8))
        {
            fprintf(stderr, "File is not a BOMStore file\n");
            return ERR_INVALID;
        }
    }

    c = ntohl(f->var_header->var_count);
    *ret = NULL;

    for(; i < c; ++ i)
    {
        uint8_t name_size;

        bom_var_t *var = malloc(sizeof(bom_var_t));
        var->next = *ret;

        fread((void*) &var->data, sizeof(var->data), 1, f->fd);

        name_size = var->data.name_size;
        ALLOC_CHECK(var->name = malloc(name_size + 1))
        var->name[name_size] = '\0';

        fread((void*) var->name, name_size, 1, f->fd);
        *ret = var;
    }

    return ERR_NOERR;
}

int bom_get_data_offset(bom_file_t *f, uint32_t id, uint32_t *addr, uint32_t *size)
{
    bom_index_t *index;
    int err;

    if(f->index_header == NULL)
        ERR_CHECK(err, bom_read_index(f))

    index = f->index_section + ntohl(id);
    *addr = index->addr;
    *size = index->size;

    return ERR_NOERR;
}

int bom_get_value_tree(bom_var_t *v, bom_file_t *f, bom_tree_t **ret)
{
    uint32_t addr;
    uint32_t size;
    int err;

    ERR_CHECK(err, bom_get_data_offset(f, v->data.data_index, &addr, &size))
    fseek(f->fd, ntohl(addr), SEEK_SET);
    ALLOC_CHECK(*ret = malloc(sizeof(bom_tree_t)))

    fread((void*) *ret, sizeof(bom_tree_t), 1, f->fd);
    return ERR_NOERR;
}

int bom_read_file(bom_file_t *f, uint32_t index0, uint32_t index1,
    bom_file_entry_t **file, bom_fileinfo1_t **inf)
{
    int err;
    uint32_t addr;
    uint32_t size;
    uint32_t pos;
    uint8_t *buf;

    ALLOC_CHECK(*file = malloc(sizeof(bom_file_entry_t)))
    ERR_CHECK(err, bom_get_data_offset(f, ntohl(index1), &addr, &size));

    pos = ftell(f->fd);

    fseek(f->fd, ntohl(addr), SEEK_SET);
    size = ntohl(size);

    /* index1 is file */

    ALLOC_CHECK(buf = malloc(size))
    FREAD_CHECK(fread((void*) buf, size, 1, f->fd), 1)

    (*file)->parent_index = *((uint32_t*) buf);
    (*file)->name = malloc(size - sizeof(uint32_t));

    strncpy((*file)->name, (char*) buf + sizeof(uint32_t), size - sizeof(uint32_t));

    /* index0 is file info */

    ALLOC_CHECK(*inf = malloc(sizeof(bom_fileinfo1_t)))
    ERR_CHECK(err, bom_get_data_offset(f, ntohl(index0), &addr, &size))

    fseek(f->fd, ntohl(addr), SEEK_SET);
    FREAD_CHECK(fread((void*) *inf, sizeof(bom_fileinfo1_t), 1, f->fd), 1)

    return ERR_NOERR;
}

int bom_get_path(bom_file_t *f, uint32_t index, bom_path_t **p, bom_indice_t **in)
{
    int err;
    uint32_t addr;
    uint32_t size;
    uint32_t indice_count;
    uint32_t i = 0;

    ERR_CHECK(err, bom_get_data_offset(f, index, &addr, &size))
    ALLOC_CHECK(*p = realloc(*p, sizeof(bom_path_t)))

    fseek(f->fd, ntohl(addr), SEEK_SET);
    FREAD_CHECK(fread((void*) *p, sizeof(bom_path_t), 1, f->fd), 1)

    indice_count = ntohs((*p)->count);
    ALLOC_CHECK(*in = realloc(*in, sizeof(bom_indice_t) * indice_count))

    for(; i < indice_count; ++ i)
    {
        bom_indice_t *indice = *in + i;
        FREAD_CHECK(fread((void*) indice, sizeof(bom_indice_t), 1, f->fd), 1)
    }

    return ERR_NOERR; 
}

int bom_get_file_at_indice(bom_file_t *t, bom_indice_t *i, 
    bom_file_entry_t **file, bom_fileinfo1_t **inf)
{
    return bom_read_file(t, ntohl(i->index0), ntohl(i->index1), file, inf);
}

/* deallocation */

void bom_file_free(bom_file_t *f)
{
    if(f->header != NULL)
        free(f->header);

    if(f->index_header != NULL)
        free(f->index_header);

    if(f->var_header != NULL)
        free(f->var_header);

    if(f->index_section != NULL)
        free(f->index_section);

    fclose(f->fd);
    free(f);
}

void bom_var_free(bom_var_t *v)
{
    if(v->next != NULL)
        bom_var_free(v->next);

    free(v->name);
    free(v);
}

