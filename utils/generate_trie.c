/* -*- mode: c; -*- */

/* This program reads a description of CCITTFAX white and black codes (from data
 * directory) and their corresponding values and generates a text representation
 * of the trie.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED(x) ((void)x)

enum cfd_trie_node_type {
        CFD_TRIE_NONE = 0, CFD_TRIE_NODE, CFD_TRIE_LEAF
};

union cfd_trie_any_t;

struct cfd_trie_node_t
{
        int type, ord;
        union cfd_trie_any_t *arr[2];
};

struct cfd_trie_leaf_t
{
        int type, ord, value;
};

union cfd_trie_any_t
{
        int type;
        struct cfd_trie_node_t node;
        struct cfd_trie_leaf_t leaf;
};

static int ord = 0;

static union cfd_trie_any_t *
cfd_trie_walk(union cfd_trie_any_t *root, const char *s, int insert)
{
        int c;

        assert(root);
        assert(s && s[0]);

        union cfd_trie_any_t *ptr = root;

        for (; s[0]; ++s) {
                c = s[0] - '0';
                assert(0 == c || 1 == c);

                if (CFD_TRIE_LEAF == ptr->type) {
                        fprintf(stderr, "error : matching prefix\n");
                        return 0;
                }

                if (CFD_TRIE_NONE == ptr->type) {
                        ptr->type = CFD_TRIE_NODE;
                }

                if (0 == ptr->node.arr[c]) {
                        if (!insert)
                                return 0;

                        ptr->node.arr[c] = malloc(sizeof *ptr);
                        if (0 == ptr->node.arr[c]) {
                                perror("trie node alloc");
                                return 0;
                        }

                        memset(ptr->node.arr[c], 0, sizeof *ptr);
                        ptr->node.arr[c]->node.ord = ++ord;
                }

                ptr = ptr->node.arr[c];
        }

        return ptr;
}

static int
cfd_trie_insert(union cfd_trie_any_t *root, const char *s, int value)
{
        union cfd_trie_any_t *ptr;

        ptr = cfd_trie_walk(root, s, 1);
        if (0 == ptr) {
                fprintf(stderr, "trie insert fail\n");
                return 1;
        }

        assert(ptr->type == CFD_TRIE_NONE);

        ptr->type = CFD_TRIE_LEAF;
        ptr->leaf.value = value;

        return 0;
}

static int
cfd_trie_query(union cfd_trie_any_t *root, const char *s, int *value)
{
        union cfd_trie_any_t *ptr;

        ptr = cfd_trie_walk(root, s, 0);
        if (0 == ptr) {
                fprintf(stderr, "trie query fail\n");
                return 1;
        }

        if (0 == ptr || CFD_TRIE_LEAF != ptr->type) {
                fprintf(stderr, "invalid query\n");
                return 1;
        }

        *value = ptr->leaf.value;

        return 0;
}

static inline int
ordinal_of(const union cfd_trie_any_t *ptr)
{
        return ptr ? ptr->node.ord : 0;
}

static void
cfd_trie_visit_node(const union cfd_trie_any_t *ptr)
{
        if (0 == ptr)
                return;

        switch(ptr->type) {
        case CFD_TRIE_LEAF:
                printf("    { /* %3d */ %3d, %4d,    0 },\n",
                       ptr->leaf.ord,
                       ptr->leaf.type,
                       ptr->leaf.value);
                break;

        case CFD_TRIE_NODE: {
                printf("    { /* %3d */ %3d, %4d, %4d },\n",
                       ptr->node.ord, ptr->node.type,
                       ordinal_of(ptr->node.arr[0]),
                       ordinal_of(ptr->node.arr[1]));

                cfd_trie_visit_node(ptr->node.arr[0]);
                cfd_trie_visit_node(ptr->node.arr[1]);
        }
                break;

        case CFD_TRIE_NONE:
        default:
                break;
        }
}

static void
cfd_trie_traverse(const union cfd_trie_any_t *root)
{
        cfd_trie_visit_node(root);
}

int
main(int argc, char **argv)
{
        char code[32], ignore[8];
        int value;

        union cfd_trie_any_t root;
        root.node = (struct cfd_trie_node_t){ CFD_TRIE_NODE, 0, { 0, 0 } };

        UNUSED(argc);
        UNUSED(argv);

        while (EOF != scanf("%s %d %s\n", code, &value, ignore)) {
                cfd_trie_insert(&root, code, value);
                /* printf("%s:%d:%s\n", code, value, ignore); */
        }

        if (argv[1] && argv[1][0]) {
                if (0 == cfd_trie_query(&root, argv[1], &value)) {
                        printf(" --> %d\n", value);
                }
        }

        cfd_trie_traverse(&root);

        return 0;
}
