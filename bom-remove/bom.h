#include <stdint.h>
#include <stdio.h>

#define ERR_NOERR   0
#define ERR_FILERR  1
#define ERR_INVALID 2
#define ERR_MALLOC  3

/*
 * Structs for BOM file memory-representation. 
 * A BOM file is made of 3 sections :
 *
 *   - a header with global infos about the file
 *   - an index section referencing data
 *   - a var section listing variables with a name and data 
 *     that refers to indexes inside the section section
 */

/*
 * First section
 * Basically contains offset and size of other sections
 */

typedef struct __attribute__((packed))
{
    char magic[8];
    uint32_t unknown0;
    uint32_t unknown1;

    uint32_t index_header_offset;
    uint32_t index_header_size;

    uint32_t var_header_offset;
    uint32_t unknown2;
} bom_header_t;

/*
 * second section
 */

typedef struct __attribute__((packed))
{ 
    uint32_t unknown0;
} bom_index_header_t;

typedef struct __attribute__((packed))
{
    uint32_t addr;
    uint32_t size;
} bom_index_t;

/*
 * A single variable entry in the third section.
 * Contains its name and the index of its data inside
 * the second section
 */
typedef struct bom_var
{
    struct __attribute__((packed))
    {
        uint32_t data_index;
        uint8_t name_size;
    } data;

    char *name;
    struct bom_var *next;
} bom_var_t;

/*
 * Header for the third section
 */
typedef struct __attribute__((packed))
{
    uint32_t var_count;
} bom_var_header_t;

/*
 * structs for data stored in the bom index section
 */

typedef struct __attribute__((packed))
{
    char magic[4];

    uint32_t unknown0;
    uint32_t child;
    uint32_t node_size;
    uint32_t count;
    uint8_t unknown1;
} bom_tree_t;

typedef struct __attribute__((packed))
{
    uint32_t index0;
    uint32_t index1;
} bom_indice_t;

typedef struct __attribute__((packed)) 
{
    uint16_t leaf;  
    uint16_t count;   
    uint32_t forward; 
    uint32_t backward;
} bom_path_t;

typedef struct __attribute__((packed))
{
    uint32_t parent_index;
    char *name;
} bom_file_entry_t;

typedef struct __attribute__((packed))
{
    uint32_t id;
    uint32_t info2_index;
} bom_fileinfo1_t;

/*
 * Handle for BOM files
 */

typedef struct
{
    bom_header_t *header;
    bom_index_header_t *index_header;
    bom_var_header_t *var_header;
    bom_index_t *index_section;

    FILE *fd;
} bom_file_t;

/*
 * BOM handling functions
 */

int bom_open_file(char const *path, bom_file_t **file);
int bom_read_header(bom_file_t *file);
int bom_read_index(bom_file_t *file);
int bom_read_var(bom_file_t *file, bom_var_t **var);
int bom_get_data_offset(bom_file_t *f, uint32_t id, uint32_t *addr, uint32_t *size);
int bom_get_value_tree(bom_var_t *var, bom_file_t *file, bom_tree_t **ret);
int bom_get_path(bom_file_t *f, uint32_t id, bom_path_t **p, bom_indice_t **in);
int bom_get_file_at_indice(bom_file_t *f, bom_indice_t *i, bom_file_entry_t **e, bom_fileinfo1_t **in);

void bom_file_free(bom_file_t *f);
void bom_var_free(bom_var_t *v);
