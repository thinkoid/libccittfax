/* -*- mode: c; -*- */

#ifndef CCITTFAX_SRC_CFD_TRIE_H
#define CCITTFAX_SRC_CFD_TRIE_H

struct cfd_trie_state_t {
        int color, state;
};

void
cfd_trie_init(struct cfd_trie_state_t *state, int color);

int
cfd_trie_walk(struct cfd_trie_state_t *state, int symbol);

int
cfd_trie_is_terminal(struct cfd_trie_state_t *state);

void
cfd_trie_get_value(struct cfd_trie_state_t *state, int *value);

#endif /* CCITTFAX_SRC_CFD_TRIE_H */
