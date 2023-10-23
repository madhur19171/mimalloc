/* ----------------------------------------------------------------------------
Copyright (c) 2018-2020, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Walloc-size-larger-than="
#endif

/*
Testing allocators is difficult as bugs may only surface after particular
allocation patterns. The main approach to testing _mimalloc_ is therefore
to have extensive internal invariant checking (see `page_is_valid` in `page.c`
for example), which is enabled in debug mode with `-DMI_DEBUG_FULL=ON`.
The main testing is then to run `mimalloc-bench` [1] using full invariant checking
to catch any potential problems over a wide range of intensive allocation bench
marks.

However, this does not test well for the entire API surface. In this test file
we therefore test the API over various inputs. Please add more tests :-)

[1] https://github.com/daanx/mimalloc-bench
*/

#define _GNU_SOURCE

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>

#include <math.h>

#ifdef __cplusplus
#include <vector>
#endif

#include "mimalloc.h"
// #include "mimalloc/internal.h"
#include "mimalloc/types.h" // for MI_DEBUG and MI_ALIGNMENT_MAX

#include "testhelper.h"

// ---------------------------------------------------------------------------
// Main testing
// ---------------------------------------------------------------------------
int main(void) {
  mi_option_disable(mi_option_verbose);
  
  // ---------------------------------------------------
  // Malloc
  // ---------------------------------------------------

  #define NUM_COMPARTMENTS (1 << 4)
  #define PTR_PER_COMPARTMENT (1 << 4)
  #define ALLOCATION_SIZE  (1 << 20)  /*In Bytes*/
  #define NUM_INT_ELEMENTS  (ALLOCATION_SIZE / 4)

  typedef struct compartment_s {
    uint64_t compartment_id;
    int * p[PTR_PER_COMPARTMENT];
  } compartment_t;

  CHECK_BODY("malloc-zero") {

    compartment_t compartments[NUM_COMPARTMENTS];

    // Creating half of the compartments right now
    // Rest half will be created later
    for(int i = 0; i < NUM_COMPARTMENTS / 2; i++) {
      compartments[i].compartment_id = create_compartment();
    }

    // Round 1
    printf("Round 1\n");

    for(int i = 0; i < NUM_COMPARTMENTS / 2; i++){
      for(int j = 0; j < PTR_PER_COMPARTMENT; j++){
        switch_compartment(compartments[i].compartment_id);
        compartments[i].p[j] = (int * ) mi_malloc(ALLOCATION_SIZE);
        if (compartments[i].p[j] == NULL) return -1;

        // Initialize the allocated memory
        for(int k = 0; k < NUM_INT_ELEMENTS; k++){
          compartments[i].p[j][k] = (i << 24) | (j << 20) | (k);
        }

        printf("[C%d\tP%d]\tBase Pointer: %p\n", i, j, compartments[i].p[j]);

        // mi_free(compartments[i].p[j]);
      }

      for(int j = 0; j < PTR_PER_COMPARTMENT; j++){
        mi_free(compartments[i].p[j]);
      }
    }

  };
}
