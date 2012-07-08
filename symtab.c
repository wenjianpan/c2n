/*! \file */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"

typedef struct elt {
        struct list_head                link;
        char *                          key;
	bool    not_free_by_free_elt;  
        symvalue_t *                    value;
} elt_t;


#define REQUIRE(condition)  if(!(condition)){ \
       printf("error\n"); \
       exit(0);  \
}

int
symtab_create(unsigned int size,
                  symtabundefaction_t undefine_action,
                  void *undefine_arg,
                  int case_sensitive,
                  symtab_t **symtabp)
{
        symtab_t *symtab;
        unsigned int i;

        REQUIRE(symtabp != NULL && *symtabp == NULL);
        REQUIRE(size > 0);      /* Should be prime. */

        symtab = malloc(sizeof(*symtab));
        if (symtab == NULL)
                return (R_NOMEMORY);
		//printf("-------- sym table 0x%x create\n", symtab);
		
        symtab->table = malloc(size * sizeof(struct list_head));
        if (symtab->table == NULL) {
                free(symtab);
                return (R_NOMEMORY);
        }
        for (i = 0; i < size; i++){
                INIT_LIST_HEAD(&(symtab->table[i]));
        }
        symtab->size = size;
        symtab->undefine_action = undefine_action;
        symtab->undefine_arg = undefine_arg;
        symtab->case_sensitive = case_sensitive;

        *symtabp = symtab;

        return (R_SUCCESS);
}

static inline void
free_elt(symtab_t *symtab, elt_t *elt, void (*free_r)(void*)) {
        //printf("free %s\n", elt->key); 
        list_del(&(elt->link));
        if (symtab->undefine_action != NULL)
                (symtab->undefine_action)( elt->value, symtab->undefine_arg);

		if( (elt->not_free_by_free_elt != T) && free_r != NULL) 
			(*free_r)((void*)elt->value);
		free(elt);
}

void
symtab_destroy(symtab_t **symtabp, void (*free_r)(void*)) {
        symtab_t *symtab;
        unsigned int i;
        elt_t *elt, *nelt;

        REQUIRE(symtabp != NULL);
        symtab = *symtabp;
        REQUIRE(symtab != NULL);

        for (i = 0; i < symtab->size; i++) {
                    struct list_head *curr;
					struct list_head *head;
    
                    head = &(symtab->table[i]);
                    for (curr = head->next; curr != head; ) {
                       elt_t *nelt = list_entry(curr, elt_t, link);
					   curr = curr->next;
		      
                       free_elt(symtab, nelt, free_r);
                    }
        }
        free(symtab->table);
		//printf("-------- sym table 0x%x free\n", symtab);
        free(symtab);

        *symtabp = NULL;
}

static inline unsigned int
hash(const char *key, int case_sensitive) {
        const char *s;
        unsigned int h = 0;
        unsigned int g;
        int c;

        /*
         * P. J. Weinberger's hash function, adapted from p. 436 of
         * _Compilers: Principles, Techniques, and Tools_, Aho, Sethi
         * and Ullman, Addison-Wesley, 1986, ISBN 0-201-10088-6.
         */

        if (case_sensitive) {
                for (s = key; *s != '\0'; s++) {
                        h = ( h << 4 ) + *s;
                        if ((g = ( h & 0xf0000000 )) != 0) {
                                h = h ^ (g >> 24);
                                h = h ^ g;
                        }
                }
        } else {
                for (s = key; *s != '\0'; s++) {
                        c = *s;
                        c = tolower((unsigned char)c);
                        h = ( h << 4 ) + c;
                        if ((g = ( h & 0xf0000000 )) != 0) {
                                h = h ^ (g >> 24);
                                h = h ^ g;
                        }
                }
        }

        return (h);
}

/*
  * FIND: if found, f = 1 and e will be the found element
 */

#define FIND(s, k, t, b, e, f) \
	    f = 0; \
        b = hash((k), (s)->case_sensitive) % (s)->size; \
        if ((s)->case_sensitive) { \
                struct list_head *curr; \
                list_for_each(curr, &(s->table[b])){ \
                       e = list_entry(curr, elt_t, link); \
                       if (((e->value->symtype & (t)) > 0) && \
                            strcmp(e->key, (k)) == 0) { \
                                f = 1; \
                                break; \
                       } \
                } \
        } else { \
                struct list_head *curr; \
                list_for_each(curr, &(s->table[b])){ \
                       e = list_entry(curr, elt_t, link); \
                       if (((e->value->symtype & (t)) > 0) && \
                           strcasecmp(e->key, (k)) == 0){ \
                                f = 1; \
                                break; \
                       } \
                } \
        }

int
symtab_lookup(symtab_t *symtab, const char *key, int type, symvalue_t **value)
{
        unsigned int bucket;
        elt_t *elt;
		int found;

        REQUIRE(symtab != NULL);
        REQUIRE(key != NULL);

        FIND(symtab, key, type, bucket, elt, found);

        if (found == 0)
                return (R_NOTFOUND);

        if (value != NULL)
                *value = elt->value;

        return (R_SUCCESS);
}


/*
free_flag is used to determine whether it's freed by free_elt, only set T when parse function definition.
function paramters are define in the symtab but not freed bt free_elt, it's free by free_ctype when free
function ctype. So if free_flag is True, when function return, symtab_destroy will not free paramters ctype.
*/

int
symtab_define(symtab_t *symtab, symvalue_t * value, 
                     symexists_t exists_policy, void (*free_r)(void*), bool free_flag)
{
        unsigned int bucket;
        elt_t *elt, *nelt;
		int found;

        REQUIRE(symtab != NULL);
	if(value->name == NULL) 
		return R_UNSUCCESS;
        REQUIRE(value->name[0] != '\0');

        FIND(symtab, (value->name), (value->symtype), bucket, elt, found);

        if (exists_policy != symexists_add && found) {
                if (exists_policy == symexists_reject) {
						return (R_EXISTS);
                }
                /* exists_policy == symexists_replace */
                elt->key = value->name;
				(*free_r)((void*)elt->value);
                elt->value = value;                
                if (symtab->undefine_action != NULL)
                        (symtab->undefine_action)(elt->value,
                                                  symtab->undefine_arg);
        } else {
			    nelt = malloc(sizeof(elt_t));
		            nelt->not_free_by_free_elt= free_flag;
			    if (nelt == NULL)
					return (R_NOMEMORY);
			    INIT_LIST_HEAD(&(nelt->link));
				nelt->key = value->name;
                nelt->value = value;

                list_add(&(nelt->link), &(symtab->table[bucket]));
        }

        return (R_SUCCESS);
}


int
symtab_undefine(symtab_t *symtab, char *key, int type, void (*free_r)(void*)) {
        unsigned int bucket;
        elt_t *elt;
		int found;

        REQUIRE(symtab != NULL);
        REQUIRE(key != NULL);

        FIND(symtab, key, type, bucket, elt, found);

        if (found == 0)
                return (R_NOTFOUND);

        free_elt(symtab, elt, free_r);

        return (R_SUCCESS);
}



#ifdef TEST
int main()
{
  symtab_t *symtab = NULL;
  symvalue_t *value;
  symvalue_t *v;

  int res;

  symtab_create(3, NULL, NULL, 1, &symtab);

  value = malloc(sizeof(symvalue_t));
  if (value == NULL)
      return (R_NOMEMORY);  
  value->name[0] = '\0';
  value->type = 1;
  strcat(value->name, "abc");
  printf("define \"%s\", type 0x%x\n", value->name, value->type);
  symtab_define(symtab, value, symexists_reject);

  value = malloc(sizeof(symvalue_t));
  if (value == NULL)
      return (R_NOMEMORY);  
  value->name[0] = '\0';
  value->type = 1;
  strcat(value->name, "eee");
  printf("define \"%s\", type 0x%x\n", value->name, value->type);
  symtab_define(symtab, value, symexists_reject);


  value = malloc(sizeof(symvalue_t));
  if (value == NULL)
      return (R_NOMEMORY);  
  value->name[0] = '\0';
  value->type = 1;
  strcat(value->name, "eee");
  printf("define \"%s\", type 0x%x\n", value->name, value->type);
  symtab_define(symtab, value, symexists_add);


  value = malloc(sizeof(symvalue_t));
  if (value == NULL)
      return (R_NOMEMORY);  
  value->name[0] = '\0';
  value->type = 1;
  strcat(value->name, "ccc");
  printf("define \"%s\", type 0x%x\n", value->name, value->type);
  symtab_define(symtab, value, symexists_reject);  
  
  printf("lookup \"abc\"\n");
  res = symtab_lookup(symtab, "abc" , 1, &v);
  printf("symtab_lookup return %d, name %s\n", res, v->name);

  printf("undefine \"bb\"\n");
  res = symtab_undefine(symtab, "bb" , 1);
  printf("symtab_undefine return %d\n", res);

  printf("undefine \"abc\"\n");
  res = symtab_undefine(symtab, "abc", 1);
  printf("symtab_undefine return %d\n", res); 

  printf("lookup \"abc\"\n");
  res = symtab_lookup(symtab, "abc", 1, &v);
  printf("symtab_lookup return %d, name %s\n", res, v->name);  

  printf("lookup \"eee\"\n");
  res = symtab_lookup(symtab, "eee", 1, &v);
  printf("symtab_lookup return %d, name %s\n", res, v->name);

  symtab_destroy(&symtab);
  return 0;
}

#endif   /* #TEST */



