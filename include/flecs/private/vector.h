
/**
 * @file vector.h
 * @brief Vector datastructure.
 *
 * This is an implementation of a simple vector type. The vector is allocated in
 * a single block of memory, with the element count, and allocated number of
 * elements encoded in the block. As this vector is used for user-types it has
 * been designed to support alignments higher than 8 bytes. This makes the size
 * of the vector header variable in size. To reduce the overhead associated with
 * retrieving or computing this size, the functions are wrapped in macro calls
 * that compute the header size at compile time.
 *
 * The API provides a number of _t macro's, which accept a size and alignment.
 * These macro's are used when no compile-time type is available.
 *
 * The vector guarantees contiguous access to its elements. When an element is
 * removed from the vector, the last element is copied to the removed element.
 *
 * The API requires passing in the type of the vector. This type is used to test
 * whether the size of the provided type equals the size of the type with which
 * the vector was created. In release mode this check is not performed.
 *
 * When elements are added to the vector, it will automatically resize to the
 * next power of two. This can change the pointer of the vector, which is why
 * operations that can increase the vector size, accept a double pointer to the
 * vector.
 */

#ifndef FLECS_VECTOR_H
#define FLECS_VECTOR_H

#include "api_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Public, so we can do compile-time header size calculation */
struct ecs_vector_t {
    int32_t count;
    int32_t size;
    
#ifndef NDEBUG
    int64_t elem_size;
#endif
};

/* Compute the header size of the vector from size & alignment */
#define ECS_VECTOR_U(size, alignment) size, ECS_MAX(ECS_SIZEOF(ecs_vector_t), alignment)

/* Compute the header size of the vector from a provided compile-time type */
#define ECS_VECTOR_T(T) ECS_VECTOR_U(ECS_SIZEOF(T), ECS_ALIGNOF(T))

/* Utility macro's for creating vector on stack */
#ifndef NDEBUG
#define ECS_VECTOR_VALUE(T, elem_count)\
{\
    .elem_size = (int32_t)(ECS_SIZEOF(T)),\
    .count = elem_count,\
    .size = elem_count\
}
#else
#define ECS_VECTOR_VALUE(T, elem_count)\
{\
    .count = elem_count,\
    .size = elem_count\
}
#endif

#define ECS_VECTOR_DECL(name, T, elem_count)\
struct {\
    union {\
        ecs_vector_t vector;\
        uint64_t align;\
    } header;\
    T array[elem_count];\
} __##name##_value = {\
    .header.vector = ECS_VECTOR_VALUE(T, elem_count)\
};\
const ecs_vector_t *name = (ecs_vector_t*)&__##name##_value

#define ECS_VECTOR_IMPL(name, T, elems, elem_count)\
ecs_os_memcpy(__##name##_value.array, elems, sizeof(T) * elem_count)

#define ECS_VECTOR_STACK(name, T, elems, elem_count)\
ECS_VECTOR_DECL(name, T, elem_count);\
ECS_VECTOR_IMPL(name, T, elems, elem_count)

typedef struct ecs_vector_t ecs_vector_t;

typedef int (*ecs_comparator_t)(
    const void* p1,
    const void *p2);

/** Create new vector. */
FLECS_API
ecs_vector_t* _ecs_vector_new(
    ecs_size_t elem_size,
    int16_t offset,
    int32_t elem_count);

#define ecs_vector_new(T, elem_count) \
    _ecs_vector_new(ECS_VECTOR_T(T), elem_count)

#define ecs_vector_new_t(size, alignment, elem_count) \
    _ecs_vector_new(ECS_VECTOR_U(size, alignment), elem_count)    

/* Create new vector, initialize it with provided array */
FLECS_API
ecs_vector_t* _ecs_vector_from_array(
    ecs_size_t elem_size,
    int16_t offset,
    int32_t elem_count,
    void *array);

#define ecs_vector_from_array(T, elem_count, array)\
    _ecs_vector_from_array(ECS_VECTOR_T(T), elem_count, array)

/* Initialize vector with zero's */
FLECS_API
void _ecs_vector_zero(
    ecs_vector_t *vector,
    ecs_size_t elem_size,
    int16_t offset);

#define ecs_vector_zero(vector, T) \
    _ecs_vector_zero(vector, ECS_VECTOR_T(T))

/** Free vector */
FLECS_API
void ecs_vector_free(
    ecs_vector_t *vector);

/** Clear values in vector */
FLECS_API
void ecs_vector_clear(
    ecs_vector_t *vector);

/** Assert when the provided size does not match the vector type. */
FLECS_API
void ecs_vector_assert_size(
    ecs_vector_t* vector_inout,
    ecs_size_t elem_size);

/** Assert when the provided alignment does not match the vector type. */
FLECS_API
void ecs_vector_assert_alignment(
    ecs_vector_t* vector,
    ecs_size_t elem_alignment);    

/** Add element to vector. */
FLECS_API
void* _ecs_vector_add(
    ecs_vector_t **array_inout,
    ecs_size_t elem_size,
    int16_t offset);

#define ecs_vector_add(vector, T) \
    ((T*)_ecs_vector_add(vector, ECS_VECTOR_T(T)))

#define ecs_vector_add_t(vector, size, alignment) \
    _ecs_vector_add(vector, ECS_VECTOR_U(size, alignment))

/** Add n elements to the vector. */
FLECS_API
void* _ecs_vector_addn(
    ecs_vector_t **array_inout,
    ecs_size_t elem_size,
    int16_t offset,
    int32_t elem_count);

#define ecs_vector_addn(vector, T, elem_count) \
    ((T*)_ecs_vector_addn(vector, ECS_VECTOR_T(T), elem_count))

#define ecs_vector_addn_t(vector, size, alignment, elem_count) \
    _ecs_vector_addn(vector, ECS_VECTOR_U(size, alignment), elem_count)

/** Get element from vector. */
FLECS_API
void* _ecs_vector_get(
    const ecs_vector_t *vector,
    ecs_size_t elem_size,
    int16_t offset,
    int32_t index);

#define ecs_vector_get(vector, T, index) \
    ((T*)_ecs_vector_get(vector, ECS_VECTOR_T(T), index))

#define ecs_vector_get_t(vector, size, alignment, index) \
    _ecs_vector_get(vector, ECS_VECTOR_U(size, alignment), index)

/** Get last element from vector. */
FLECS_API
void* _ecs_vector_last(
    const ecs_vector_t *vector,
    ecs_size_t elem_size,
    int16_t offset);

#define ecs_vector_last(vector, T) \
    (T*)_ecs_vector_last(vector, ECS_VECTOR_T(T))

/** Set minimum size for vector. If the current size of the vector is larger, 
 * the function will have no side effects. */
FLECS_API
int32_t _ecs_vector_set_min_size(
    ecs_vector_t **array_inout,
    ecs_size_t elem_size,
    int16_t offset,
    int32_t elem_count);

#define ecs_vector_set_min_size(vector, T, size) \
    _ecs_vector_set_min_size(vector, ECS_VECTOR_T(T), size)

/** Set minimum count for vector. If the current count of the vector is larger, 
 * the function will have no side effects. */
FLECS_API
int32_t _ecs_vector_set_min_count(
    ecs_vector_t **vector_inout,
    ecs_size_t elem_size,
    int16_t offset,
    int32_t elem_count);

#define ecs_vector_set_min_count(vector, T, size) \
    _ecs_vector_set_min_count(vector, ECS_VECTOR_T(T), size)

/** Remove last element. This operation requires no swapping of values. */
FLECS_API
void ecs_vector_remove_last(
    ecs_vector_t *vector);

/** Remove last value, store last element in provided value. */
FLECS_API
bool _ecs_vector_pop(
    ecs_vector_t *vector,
    ecs_size_t elem_size,
    int16_t offset,
    void *value);

#define ecs_vector_pop(vector, T, value) \
    _ecs_vector_pop(vector, ECS_VECTOR_T(T), value)

/** Append element at specified index to another vector. */
FLECS_API
int32_t _ecs_vector_move_index(
    ecs_vector_t **dst,
    ecs_vector_t *src,
    ecs_size_t elem_size,
    int16_t offset,
    int32_t index);

#define ecs_vector_move_index(dst, src, T, index) \
    _ecs_vector_move_index(dst, src, ECS_VECTOR_T(T), index)

/** Remove element at specified index. Moves the last value to the index. */
FLECS_API
int32_t _ecs_vector_remove_index(
    ecs_vector_t *vector,
    ecs_size_t elem_size,
    int16_t offset,
    int32_t index);

#define ecs_vector_remove_index(vector, T, index) \
    _ecs_vector_remove_index(vector, ECS_VECTOR_T(T), index)

#define ecs_vector_remove_index_t(vector, size, alignment, index) \
    _ecs_vector_remove_index(vector, ECS_VECTOR_U(size, alignment), index)

/** Shrink vector to make the size match the count. */
FLECS_API
void _ecs_vector_reclaim(
    ecs_vector_t **vector,
    ecs_size_t elem_size,
    int16_t offset);

#define ecs_vector_reclaim(vector, T)\
    _ecs_vector_reclaim(vector, ECS_VECTOR_T(T))

/** Grow size of vector with provided number of elements. */
FLECS_API
int32_t _ecs_vector_grow(
    ecs_vector_t **vector,
    ecs_size_t elem_size,
    int16_t offset,
    int32_t elem_count);

#define ecs_vector_grow(vector, T, size) \
    _ecs_vector_grow(vector, ECS_VECTOR_T(T), size)

/** Set allocation size of vector. */
FLECS_API
int32_t _ecs_vector_set_size(
    ecs_vector_t **vector,
    ecs_size_t elem_size,
    int16_t offset,
    int32_t elem_count);

#define ecs_vector_set_size(vector, T, elem_count) \
    _ecs_vector_set_size(vector, ECS_VECTOR_T(T), elem_count)

#define ecs_vector_set_size_t(vector, size, alignment, elem_count) \
    _ecs_vector_set_size(vector, ECS_VECTOR_U(size, alignment), elem_count)

/** Set count of vector. If the size of the vector is smaller than the provided
 * count, the vector is resized. */
FLECS_API
int32_t _ecs_vector_set_count(
    ecs_vector_t **vector,
    ecs_size_t elem_size,
    int16_t offset,
    int32_t elem_count);

#define ecs_vector_set_count(vector, T, elem_count) \
    _ecs_vector_set_count(vector, ECS_VECTOR_T(T), elem_count)

#define ecs_vector_set_count_t(vector, size, alignment, elem_count) \
    _ecs_vector_set_count(vector, ECS_VECTOR_U(size, alignment), elem_count)

/** Return number of elements in vector. */
FLECS_API
int32_t ecs_vector_count(
    const ecs_vector_t *vector);

/** Return size of vector. */
FLECS_API
int32_t ecs_vector_size(
    const ecs_vector_t *vector);

/** Return first element of vector. */
FLECS_API
void* _ecs_vector_first(
    const ecs_vector_t *vector,
    ecs_size_t elem_size,
    int16_t offset);

#define ecs_vector_first(vector, T) \
    ((T*)_ecs_vector_first(vector, ECS_VECTOR_T(T)))

#define ecs_vector_first_t(vector, size, alignment) \
    _ecs_vector_first(vector, ECS_VECTOR_U(size, alignment))

/** Sort elements in vector. */
FLECS_API
void _ecs_vector_sort(
    ecs_vector_t *vector,
    ecs_size_t elem_size,
    int16_t offset,
    ecs_comparator_t compare_action);

#define ecs_vector_sort(vector, T, compare_action) \
    _ecs_vector_sort(vector, ECS_VECTOR_T(T), compare_action)

/** Return memory occupied by vector. */
FLECS_API
void _ecs_vector_memory(
    const ecs_vector_t *vector,
    ecs_size_t elem_size,
    int16_t offset,
    int32_t *allocd,
    int32_t *used);

#define ecs_vector_memory(vector, T, allocd, used) \
    _ecs_vector_memory(vector, ECS_VECTOR_T(T), allocd, used)

#define ecs_vector_memory_t(vector, size, alignment, allocd, used) \
    _ecs_vector_memory(vector, ECS_VECTOR_U(size, alignment), allocd, used)

/** Copy vectors */
FLECS_API
ecs_vector_t* _ecs_vector_copy(
    const ecs_vector_t *src,
    ecs_size_t elem_size,
    int16_t offset);

#define ecs_vector_copy(src, T) \
    _ecs_vector_copy(src, ECS_VECTOR_T(T))

#define ecs_vector_copy_t(src, size, alignment) \
    _ecs_vector_copy(src, ECS_VECTOR_U(size, alignment))

#ifndef FLECS_LEGACY
#define ecs_vector_each(vector, T, var, ...)\
    {\
        int var##_i, var##_count = ecs_vector_count(vector);\
        T* var##_array = ecs_vector_first(vector, T);\
        for (var##_i = 0; var##_i < var##_count; var##_i ++) {\
            T* var = &var##_array[var##_i];\
            __VA_ARGS__\
        }\
    }
#endif
#ifdef __cplusplus
}
#endif


/** C++ wrapper for vector class. */
#ifdef __cplusplus
#ifndef FLECS_NO_CPP

#include <iostream>

namespace flecs {

template <typename T>
class vector_iterator
{
public:
    explicit vector_iterator(T* value, int index) {
        m_value = value;
        m_index = index;
    }

    bool operator!=(vector_iterator const& other) const
    {
        return m_index != other.m_index;
    }

    T const& operator*() const
    {
        return m_value[m_index];
    }

    vector_iterator& operator++()
    {
        ++m_index;
        return *this;
    }

private:
    T* m_value;
    int m_index;
};

template <typename T>
class vector {
public:
    explicit vector(ecs_vector_t *v) : m_vector( v ) { }

    vector(int32_t count = 0) : m_vector( nullptr ) { 
        if (count) {
            init(count);
        }
    }

    vector(std::initializer_list<T> elems) : m_vector( nullptr) {
        init(elems.size());
        *this = elems;
    }

    void operator=(std::initializer_list<T> elems) {
        for (auto elem : elems) {
            this->add(elem);
        }
    }

    T& operator[](size_t index) {
        return ecs_vector_get(m_vector, T, index)[0];
    }

    vector_iterator<T> begin() {
        return vector_iterator<T>(
            ecs_vector_first(m_vector, T), 0);
    }

    vector_iterator<T> end() {
        return vector_iterator<T>(
            ecs_vector_last(m_vector, T),
            ecs_vector_count(m_vector));
    }    

    void clear() {
        ecs_vector_clear(m_vector);
    }

    void add(T& value) {
        T* elem = ecs_vector_add(&m_vector, T);
        *elem = value;
    }

    void add(T&& value) {
        T* elem = ecs_vector_add(&m_vector, T);
        *elem = value;
    }    

    T& get(int32_t index) {
        return ecs_vector_get(m_vector, T, index);
    }

    T& first() {
        return ecs_vector_first(m_vector, T);
    }

    T& last() {
        return ecs_vector_last(m_vector, T);
    }

    int32_t count() {
        return ecs_vector_count(m_vector);
    }

    int32_t size() {
        return ecs_vector_size(m_vector);
    }

    ecs_vector_t *ptr() {
        return m_vector;
    }

    void ptr(ecs_vector_t *ptr) {
        m_vector = ptr;
    }

private:
    void init(int32_t count) {
        m_vector = ecs_vector_new(T, count);
    }

    ecs_vector_t *m_vector;
};

}

#endif
#endif

#endif
