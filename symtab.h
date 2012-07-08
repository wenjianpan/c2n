

#ifndef SYMTAB_H
#define SYMTAB_H 1

/*****
 ***** Module Info
 *****/

/*! \file
 * \brief
 * Provides a simple memory-based symbol table.
 *
 * Keys are C strings.  A type may be specified when looking up,
 * defining, or undefining.  A type value of 0 means "match any type";
 * any other value will only match the given type.
 *
 * It's possible that a client will attempt to define a <key, type,
 * value> tuple when a tuple with the given key and type already
 * exists in the table.  What to do in this case is specified by the
 * client.  Possible policies are:
 *
 *\li   symexists_reject  Disallow the define, returning #R_EXISTS
 *\li   symexists_replace Replace the old value with the new.  The
 *                              undefine action (if provided) will be called
 *                              with the old <key, type, value> tuple.
 *\li   symexists_add     Add the new tuple, leaving the old tuple in
 *                              the table.  Subsequent lookups will retrieve
 *                              the most-recently-defined tuple.
 *
 * A lookup of a key using type 0 will return the most-recently
 * defined symbol with that key.  An undefine of a key using type 0
 * will undefine the most-recently defined symbol with that key.
 * Trying to define a key with type 0 is illegal.
 *
 * The symbol table library does not make a copy the key field, so the
 * caller must ensure that any key it passes to symtab_define()
 * will not change until it calls symtab_undefine() or
 * symtab_destroy().
 *
 * A user-specified action will be called (if provided) when a symbol
 * is undefined.  It can be used to free memory associated with keys
 * and/or values.
 */

/***
 *** Imports.
 ***/

/***
 *** Symbol Tables.
 ***/

#include "c_type.h"

typedef CType symvalue_t;

typedef void (*symtabundefaction_t)(symvalue_t *value, void *userarg);

typedef int (*symtabforeachaction_t)(symvalue_t *value, void *userarg);

typedef enum {
        symexists_reject = 0,
        symexists_replace = 1,
        symexists_add = 2
} symexists_t;


typedef struct symtab {
        unsigned int                    size;
        struct list_head *              table;
        symtabundefaction_t             undefine_action;
        void *                          undefine_arg;
        int           case_sensitive;
} symtab_t;


int
symtab_create(unsigned int size, 
                   symtabundefaction_t undefine_action, void *undefine_arg,
                   int case_sensitive, symtab_t **symtabp);

void
symtab_destroy(symtab_t **symtabp, void (*free_r)(void*));

int
symtab_lookup(symtab_t *symtab, const char *key, int type, symvalue_t **value);

int
symtab_define(symtab_t *symtab, symvalue_t *value, 
		symexists_t exists_policy, void (*free_r)(void*), bool free_flag);

int
symtab_undefine(symtab_t *symtab, char *key, int type, void (*free_r)(void*));


#endif /* SYMTAB_H */

