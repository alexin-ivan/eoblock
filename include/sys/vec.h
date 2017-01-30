#ifndef VEC_H_
#define VEC_H_
/** https://github.com/rxi/vec
 * Copyright (c) 2014 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

#ifndef EOB_KERNEL
#   include <stdlib.h>
#   include <memory.h>
#define vec_realloc realloc
#define vec_free free
#endif

/******************************************************************************/
/* Convert vec into args (with arity of 3) */
#define vec_unpack_(v) \
    (char **) &(v)->data, &(v)->length, &(v)->capacity, sizeof(*(v)->data)

/******************************************************************************/
/* Generic struct */
#define vec_t(T)              \
    struct                    \
    {                         \
        T *data;              \
        int length, capacity; \
    }

/******************************************************************************/
/* Members access */
#define vec_length(v) ((v)->length)
#define vec_data(v) ((v)->data)
#define vec_capacity(v) ((v)->capacity)

/******************************************************************************/
/** Init vec_t as memset call. */
#define vec_init(v) memset((v), 0, sizeof(*(v)))

/******************************************************************************/
/** Init vec_t as static. */
#define VEC_INIT { .data = NULL, .length = 0, .capacity = 0 }

/******************************************************************************/
/** Deinit (free) vec */
#define vec_deinit(v) (vec_free((v)->data), vec_init(v))

/******************************************************************************/
/** Element at index (lvalue) */
#define vec_at(v, i) ((v)->data[i])

/******************************************************************************/
/** Pointer to element with index */
#define vec_at_ptr(v, i) (((v)->data + i))

/******************************************************************************/
/** Element after last element (lvalue). */
#define vec_end_ref(v) (((v)->data[(v)->length]))

/******************************************************************************/
/** Element after last element. */
#define vec_end_ptr(v) (((v)->data + (v)->length))

/******************************************************************************/
/** Add element into end of vectro via '=' */
#define vec_push(v, val) \
    (vec_expand_(vec_unpack_(v)) ? -1 : ((v)->data[(v)->length++] = (val), 0))

/******************************************************************************/
/** Add element into end of vectro via memcpy */
#define vec_emplace_push(v, val) \
    (vec_expand_(vec_unpack_(v)) \
         ? -1                    \
         : (memcpy(&((v)->data[(v)->length++]), &(val), sizeof((val))), 0))

/******************************************************************************/
/** Pop last element and return it. */
#define vec_pop(v) (v)->data[--(v)->length]

/******************************************************************************/
/* ? */
#define vec_splice(v, start, count) \
    (vec_splice_(vec_unpack_(v), start, count), (v)->length -= (count))

/******************************************************************************/
/* ? */
#define vec_swapsplice(v, start, count) \
    (vec_swapsplice_(vec_unpack_(v), start, count), (v)->length -= (count))

/******************************************************************************/
/** Insert value 'val' in position 'idx' */
#define vec_insert(v, idx, val)                                           \
    (vec_insert_(vec_unpack_(v), idx) ? -1 : ((v)->data[idx] = (val), 0), \
     (v)->length++, 0)

/******************************************************************************/
#define vec_sort(v, fn) qsort((v)->data, (v)->length, sizeof(*(v)->data), fn)

/******************************************************************************/
/** Swap values */
#define vec_swap(v, idx1, idx2) vec_swap_(vec_unpack_(v), idx1, idx2)

/******************************************************************************/
/** Truncate to len */
#define vec_truncate(v, len) \
    ((v)->length = (len) < (v)->length ? (len) : (v)->length)

/******************************************************************************/
#define vec_clear(v) ((v)->length = 0)

/******************************************************************************/
#define vec_first(v) (v)->data[0]

/******************************************************************************/
/** Get last element (lvalue). */
#define vec_last(v) (v)->data[(v)->length - 1]

/******************************************************************************/
/** Get ptr to last element. */
#define vec_last_ptr(v) (v)->data + ((v)->length - 1)

/******************************************************************************/
/** Reserve n elements */
#define vec_reserve(v, n) vec_reserve_(vec_unpack_(v), n)

/******************************************************************************/
/** Resize vec to n elements. */
#define vec_resize(v, n) ((v)->length = n, vec_reserve(v, n + 1))

/******************************************************************************/
/** Resize with zero filling. */
#define vec_resize_zero(v, n) \
    ((vec_reserve(v, n)) ? -1 \
                         : (memset((v)->data, 0, sizeof((v)->data[0]) * n), 0))

/******************************************************************************/
/** Update capacity according to len. */
#define vec_compact(v) vec_compact_(vec_unpack_(v))

/******************************************************************************/
/** Insert attray */
#define vec_pusharr(v, arr, count, ret__)                               \
    do                                                                \
    {                                                                 \
        int i__, n__ = (count);                                       \
        if (vec_reserve_po2_(vec_unpack_(v), (v)->length + n__) != 0) \
        { \
            ret__ = -1; \
            break;    \
        } \
        for (i__ = 0; i__ < n__; i__++)                               \
        {                                                             \
            (v)->data[(v)->length++] = (arr)[i__];                    \
        }                                                             \
    } while (0)

/******************************************************************************/
/** Concat 2 vecs. */
#define vec_extend(v, v2, ret__) vec_pusharr((v), (v2)->data, (v2)->length, ret__)

/******************************************************************************/
/** Find value */
#define vec_find(v, val, idx)                         \
    do                                                \
    {                                                 \
        for ((idx) = 0; (idx) < (v)->length; (idx)++) \
        {                                             \
            if ((v)->data[(idx)] == (val))            \
                break;                                \
        }                                             \
        if ((idx) == (v)->length)                     \
            (idx) = -1;                               \
    } while (0)

/******************************************************************************/
/** Find value in idx where v[idx].field == val or idx=-1 if not found */
#define vec_find_by_field(v, field, val, idx)         \
    do                                                \
    {                                                 \
        for ((idx) = 0; (idx) < (v)->length; (idx)++) \
        {                                             \
            if ( ((v)->data[(idx)]). field == (val)) \
                break;                                \
        }                                             \
        if ((idx) == (v)->length)                     \
            (idx) = -1;                               \
    } while (0)

/******************************************************************************/
/** Compare values with __cmp (function or macros) */
#define vec_find_by_cmp(v, __cmp, val, idx)         \
    do                                                \
    {                                                 \
        for ((idx) = 0; (idx) < (v)->length; (idx)++) \
        {                                             \
            if (__cmp( &((v)->data[(idx)]), &(val))) \
                break;                                \
        }                                             \
        if ((idx) == (v)->length)                     \
            (idx) = -1;                               \
    } while (0)

/******************************************************************************/
#define vec_find_by_str_field(v, val, iter, field)           \
    do                                                       \
    {                                                        \
        int found = 0;                                       \
        vec_foreach_ptr(v, iter)                           \
        {\
            if(strcmp(iter-> field, (val)) == 0) { found = 1; break; }\
        }\
        if (!found) { iter = vec_end_ptr(v); } \
    } while (0)

#define vec_find_by_str_getter(v, val__, iter__, getter__)            \
    do                                                       \
    {                                                        \
        int found = 0;                                       \
        vec_foreach_ptr(v, iter__)                           \
        {\
            if(strcmp(getter__(iter__), (val__)) == 0) { found = 1; break; }\
        }\
        if (!found) { iter__ = vec_end_ptr(v); } \
    } while (0)

/******************************************************************************/
/** Remove element with selected value. */
#define vec_remove(v, val)           \
    do                               \
    {                                \
        int idx__;                   \
        vec_find(v, val, idx__);     \
        if (idx__ != -1)             \
            vec_splice(v, idx__, 1); \
    } while (0)

/******************************************************************************/
/** Reverse vec. */
#define vec_reverse(v)                                   \
    do                                                   \
    {                                                    \
        int i__ = (v)->length / 2;                       \
        while (i__--)                                    \
        {                                                \
            vec_swap((v), i__, (v)->length - (i__ + 1)); \
        }                                                \
    } while (0)

/******************************************************************************/
/** Simple foreach. */
#define vec_foreach(v, var)                                                        \
    if ((v)->length > 0)                                                           \
        for (int iter__ = 0; iter__ < (v)->length && (((var) = (v)->data[iter__]), 1); \
             ++iter__)

/******************************************************************************/
/** Foreach with index (iter) */
#define vec_foreach_(v, var, iter)                                                  \
    if ((v)->length > 0)                                                           \
        for ((iter) = 0; (iter) < (v)->length && (((var) = (v)->data[(iter)]), 1); \
             ++(iter))

/******************************************************************************/
/** Iterate over two vectors 
  \param v1 - first vector
  \param v2 - second vector
  \param var1 - first vector iter
  \param var2 - second vector iter
  \param iter - index
  */
#define vec_zip(v1, v2, var1, var2, iter)                                 \
    if ((v1)->length > 0 && (v2)->length == (v1)->length)                 \
        for ((iter) = 0;                                                  \
             (iter) < (v1)->length && (((var1) = (v1)->data[(iter)]),     \
                                       ((var2) = (v2)->data[(iter)]), 1); \
             ++(iter))

/******************************************************************************/
#define vec_zip_ptr(v1, v2, var1, var2, iter)                              \
    if ((v1)->length > 0 && (v2)->length == (v1)->length)                  \
        for ((iter) = 0;                                                   \
             (iter) < (v1)->length && (((var1) = &(v1)->data[(iter)]),     \
                                       ((var2) = &(v2)->data[(iter)]), 1); \
             ++(iter))


/******************************************************************************/
#define vec_foreach_rev(v, var)  \
    if ((v)->length > 0)               \
        for (int (iter__) = (v)->length - 1; \
             (iter__) >= 0 && (((var) = (v)->data[(iter__)]), 1); --(iter__))

/******************************************************************************/
#define vec_foreach_rev_(v, var, iter)  \
    if ((v)->length > 0)               \
        for ((iter) = (v)->length - 1; \
             (iter) >= 0 && (((var) = (v)->data[(iter)]), 1); --(iter))

/******************************************************************************/
#define vec_foreach_ptr(v, var) \
    if ((v)->length > 0)              \
        for (int iter__ = 0;              \
             iter__ < (v)->length && (((var) = &(v)->data[iter__]), 1); ++iter__)

/******************************************************************************/
#define vec_foreach_ptr_(v, var, iter) \
    if ((v)->length > 0)              \
        for ((iter) = 0;              \
             (iter) < (v)->length && (((var) = &(v)->data[(iter)]), 1); ++(iter))

/******************************************************************************/
#define vec_foreach_ptr_rev(v, var) \
    if ((v)->length > 0)                  \
        for (int (iter__) = (v)->length - 1;    \
             (iter__) >= 0 && (((var) = &(v)->data[(iter__)]), 1); --(iter__))

/******************************************************************************/
#define vec_foreach_ptr_rev_(v, var, iter) \
    if ((v)->length > 0)                  \
        for ((iter) = (v)->length - 1;    \
             (iter) >= 0 && (((var) = &(v)->data[(iter)]), 1); --(iter))

/******************************************************************************/
/** Call deinit_f__ == free for each element in vec. 
 *  It's can be useful for vec_str_t.
 * */
#define vec_deinit_with_ptr(v__) \
    do { \
        __typeof__((v__)->data[0]) var__; \
        vec_foreach((v__), (var__)) \
        { \
            vec_free(var__); \
        } \
        vec_deinit((v__)); \
    } while(0)
    
/******************************************************************************/
/** Call iter_f__ for each element in vec by value. */
#define vec_iter(v__, iter_f__) \
    do { \
        __typeof__((v__)->data[0]) var__; \
        vec_foreach((v__), (var__)) \
        { \
            iter_f__(var__); \
        } \
    } while(0)

/******************************************************************************/
/** Call iter_f__ for each element in vec by value. */
#define vec_map(v__, iter_f__, result__) \
    do { \
        __typeof__(vec_data(v__)) var__; \
        vec_foreach_ptr((v__), (var__)) \
        { \
            vec_push(result__, iter_f__(var__)); \
        } \
    } while(0)


/******************************************************************************/
/** Call iter_f__ for each element in vec by ptr. */
#define vec_iter_ptr(v__, iter_f__) \
    do { \
        __typeof__((v__)->data) var__; \
        vec_foreach_ptr((v__), (var__)) \
        { \
            iter_f__(var__); \
        } \
    } while(0)

/******************************************************************************/
/* Take owenership of vec data */
#define vec_data_detach(v) (vec_length(v) = 0, vec_capacity(v) = 0, vec_data(v))

/******************************************************************************/
#define vec_char_len(v) vec_length(v)

/******************************************************************************/
int vec_expand_(char **data, int *length, int *capacity, int memsz);
int vec_reserve_(char **data, int *length, int *capacity, int memsz, int n);
int vec_reserve_po2_(char **data, int *length, int *capacity, int memsz, int n);
int vec_compact_(char **data, int *length, int *capacity, int memsz);
int vec_insert_(char **data, int *length, int *capacity, int memsz, int idx);
void vec_splice_(char **data, int *length, int *capacity, int memsz, int start,
                 int count);
void vec_swapsplice_(char **data, int *length, int *capacity, int memsz,
                     int start, int count);
void vec_swap_(char **data, int *length, int *capacity, int memsz, int idx1,
               int idx2);

/******************************************************************************/
typedef vec_t(void *) vec_void_t;
typedef vec_t(char *) vec_str_t;
typedef vec_t(const char *) vec_cstr_t;
typedef vec_t(int) vec_int_t;
typedef vec_t(char) vec_char_t;
typedef vec_t(float) vec_float_t;
typedef vec_t(double) vec_double_t;
typedef vec_t(vec_str_t) vec_vstr_t;

#ifdef _VEC_H_EXPERIMENTAL_
/******************************************************************************/
/** Conver vec of strings into string
 * @param v - input vector of strings
 * @param str - output vector of chars (string)
 * @param sep - separator
 */
inline static int vec_str_to_vec_char(vec_str_t *v, vec_char_t *str, vec_char_t *sep)
{
    vec_char_t iter = VEC_INIT;
    int rv = 0;

    for(int i = 0; i < vec_length(v); i++)
    {
        iter.data = vec_at(v, i);
        iter.length = strlen(iter.data);
        iter.capacity = iter.length + 1;
        vec_extend(str, &iter, rv);

        if(rv)
        {
            vec_deinit(str);
            return (-1);
        }

        vec_extend(str, sep, rv);

        if(rv)
        {
            vec_deinit(str);
            return (-1);
        }

    }

    return (0);
}

inline static char *vec_str_to_raw_string(vec_str_t *v, const char *csep)
{
    vec_char_t str = VEC_INIT;
    vec_char_t sep = VEC_INIT;
    
    vec_data(&sep) = (char *) csep;
    vec_length(&sep) = strlen(vec_data(&sep));
    vec_capacity(&sep) = vec_length(&sep) + 1;

    if(vec_str_to_vec_char(v, &str, &sep))
    {
        return NULL;
    }

    return vec_data(&str);
}

/******************************************************************************/
inline static int vec_char_from_string_(vec_char_t *v, char *str)
{
    vec_deinit(v);
    size_t len = strlen(str);
    
    if(len == 0)
        return (0);
    
    if(NULL == (vec_data(v) = strdup(str)))
        return (-1);
    
    vec_length(v) = len + 1;
    vec_capacity(v) = len + 1;
    return (0);
}

inline static int vec_char_append_string(vec_char_t *v1, vec_char_t *v2)
{
    if(vec_resize(v1, vec_length(v1) + vec_length(v2) + 1))
        return (-1);

    memcpy(vec_data(v1) + vec_char_len(v1), vec_data(v2), vec_capacity(v1));
    vec_length(v1) = vec_capacity(v1) - 1;

    return (0);
}

#define ex_vec_char_from_str(v, str) vec_char_from_string_((v), (str))
#define ex_vec_char_from_cstr(v, str) (vec_deinit((v)), vec_data((v)) = str)
#endif /* EXPERIMENTAL_VEC */

#endif /* VEC_H_ */
