#ifndef C_VEC_H_
#define C_VEC_H_
/******************************************************************************/
#include <stddef.h>

/******************************************************************************/
/* Static (const) vector */
#define c_vec_t(type) struct {      \
    type *p;                        \
    size_t count;                   \
}

/******************************************************************************/
/* Access to const vec */
#define c_vec_data(v) ((v)->p)
#define c_vec_length(v) ((v)->count)
#define c_vec_last(v) (((v)->p) + ((v)->count))
#define c_vec_first(v) c_vec_data(v)

#define c_vec_is_end(v, value) (c_vec_last(v) == value)

#define c_vec_at(v, i) ((v)->p[(i)])
#define c_vec_at_ptr(v, i) (&(v)->p[(i)])
#define c_vec_foreach_ptr(v, var)    \
    if (c_vec_length(v) > 0)                \
        for (int iter__ = 0;                    \
             (iter__) < c_vec_length(v) && (((var) = c_vec_at_ptr(v, iter__)), 1); ++iter__)

#define c_vec_foreach(v, var)    \
    if (c_vec_length(v) > 0)                \
        for (int iter__ = 0;                    \
             (iter__) < c_vec_length(v) && (((var) = c_vec_at(v, iter__)), 1); ++iter__)

#define c_vec_find_by_str_field(v, val, iter, field)         \
    do                                                       \
    {                                                        \
        int found = 0;                                       \
        c_vec_foreach_ptr(v, iter)                           \
        {\
            if(strcmp(iter-> field, (val)) == 0) { found = 1; break; }\
        }\
        if (!found) { iter = c_vec_last(v); } \
    } while (0)

#define c_vec_find_by_int_field(v, val, iter, field)         \
    do                                                       \
    {                                                        \
        int found = 0;                                       \
        c_vec_foreach_ptr(v, iter)                           \
        {\
            if( ((iter)-> field) == (val) ) { found = 1; break; }\
        }\
        if (!found) { iter = c_vec_last(v); } \
    } while (0)

/******************************************************************************/
/** 'Cast-assign' */
#define c_vec_from(_args, _element_type) { .p = (_element_type *) _args, .count = sizeof(*_args) / sizeof(_element_type), }
#define c_vec_from_range(_args, _element_type, _start, _end) { .p = ((_element_type *) _args ) + _start, .count = _end - _start, }
#define c_vec_from_start(_args, _element_type, _start) { .p = ((_element_type *) _args ) + _start, .count =  (sizeof(*_args) / sizeof(_element_type)) - _start, }
#define c_vec_from_single(_args, _element_type) { .p = ((_element_type *) _args ), .count =  1, }
#define c_vec_from_empty(_element_type) { .p = ((_element_type *) NULL), .count =  0, }

#define c_vec_from_static(_args) { .p = _args, .count = sizeof(_args) / sizeof(_args[0]), }

#define c_vec_data_t(v) __typeof__(*c_vec_data(v))
#define c_vec_iter_t(v) __typeof__(*c_vec_data(v)) *
#define pc_vec_iter_t(v) __typeof__(*c_vec_data(&v)) *

#define c_vec_diff(iter2, iter1) (iter2 - iter1)


/* Find first value and truncate vec len to it position. */
#define c_vec_truncate_by_int(v, val, iter, field)                \
    do {                                                          \
        c_vec_find_by_int_field(v, val, iter, field);             \
        c_vec_length(v) = c_vec_diff(iter, c_vec_first(v)); \
    } while(0)


typedef c_vec_t(char) c_vec_char_t;
typedef c_vec_t(char*) c_vec_string_t;
typedef c_vec_t(const char*) c_vec_cstring_t;

#endif /* C_VEC_H_ */
