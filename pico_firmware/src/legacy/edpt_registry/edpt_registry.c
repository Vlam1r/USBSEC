// Copyright (c) 2021. Vladimir Viktor Mirjanic

//
// Created by vlamir on 11/16/21.
//

#include "edpt_registry.h"

static tusb_desc_endpoint_t registry[REGISTRY_MAX_SIZE];
static int count = 0;

void insert_into_registry(const tusb_desc_endpoint_t *const edpt) {
    assert(count < REGISTRY_MAX_SIZE);
    memcpy(&registry[count++], edpt, sizeof(tusb_desc_endpoint_t));
}

const tusb_desc_endpoint_t *get_first_in_registry(void) {
    return registry;
}

const tusb_desc_endpoint_t *get_next_in_registry(const tusb_desc_endpoint_t *const edpt) {
    assert(edpt >= registry);
    assert(edpt <= registry + count * sizeof(tusb_desc_endpoint_t));
    if (edpt == registry + count * sizeof(tusb_desc_endpoint_t)) {
        return NULL;
    }
    return edpt + sizeof(tusb_desc_endpoint_t);
}
