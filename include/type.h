#ifndef TYPE_H
#define TYPE_H

#include <stdlib.h>

/*** Type Tag ***/

/* Exceptions */
extern const char TYPE_TAG_STILL_ATTACHED[];    /* Data: C String */
extern const char TYPE_TAG_ALREADY_ATTACHED[];  /* Data: C String */
extern const char TYPE_TAG_NOT_ATTACHED[];      /* Data: C String */
extern const char TYPE_TAG_IS_STATIC[];         /* Data: C String */
extern const char TYPE_TAG_STILL_ACQUIRED[];    /* Data: C String */
extern const char TYPE_TAG_NOT_ACQUIRED[];      /* Data: C String */
extern const char TYPE_TAG_MISMATCH[];          /* Data: C String */

/* Opaque type tag structure. */
struct type_tag;

/* A tag with its type and implementation. */
struct type_tag_impl {
    struct type_tag *tag;
    const char *type;
    void *impl;
};

/* Type Tag Interface */
struct type_tag_i;
struct type_tag_static_i;

typedef size_t (*type_tag_size_f)();
typedef size_t (*type_tag_align_f)();

typedef void (*type_tag_init_f)(struct type_tag *tag, const struct type_tag_static_i *hooks);
typedef void (*type_tag_fini_f)(struct type_tag *tag);

typedef void (*type_tag_attach_f)(struct type_tag_impl *tti, void (*impl_detach)(void *impl));
typedef void (*type_tag_detach_f)(struct type_tag_impl *tti);
typedef void (*type_tag_detach_all_f)(struct type_tag *tag);

typedef unsigned int (*type_tag_is_static_f)(struct type_tag *tag, const char *type);

typedef size_t (*type_tag_attachments_f)(struct type_tag *tag);
typedef unsigned int (*type_tag_has_a_f)(struct type_tag *tag, const char *type);

typedef void (*type_tag_acquire_f)(struct type_tag_impl *tti);
typedef void (*type_tag_release_f)(struct type_tag_impl *tti);

typedef size_t (*type_tag_acquisitions_f)(struct type_tag_impl *tti);
typedef int (*type_tag_for_each_f)(struct type_tag *tag, void *self, int (*action)( void *self, struct type_tag_impl *tti));

struct type_tag_i {
    type_tag_size_f size;
    type_tag_align_f align;

    type_tag_init_f init;
    type_tag_fini_f fini;

    type_tag_attach_f attach;
    type_tag_detach_f detach;
    type_tag_detach_all_f detach_all;

    type_tag_is_static_f is_static;

    type_tag_attachments_f attachments;
    type_tag_has_a_f has_a;

    type_tag_acquire_f acquire;
    type_tag_release_f release;

    type_tag_acquisitions_f acquisitions;
    type_tag_for_each_f for_each;
};

/* Static typing hooks. */
struct type_tag_static_i {
    type_tag_attachments_f attachments;
    type_tag_has_a_f has_a;

    type_tag_acquire_f acquire;
    type_tag_release_f release;

    type_tag_acquisitions_f acquisitions;
    type_tag_for_each_f for_each;
};

/* Size of the type tag struct. */
size_t
type_tag_size();

/* Required alignment of the type tag struct. */
size_t
type_tag_align();

/* Initialize the type tag and set static typing hooks. */
void
type_tag_init(
        struct type_tag *tag,
        const struct type_tag_static_i *hooks);

/* Finalize the type tag.
 *
 * Throws:
 *
 * TYPE_TAG_STILL_ATTACHED
 *  If any implementations are still attached.
 */
void
type_tag_fini(
        struct type_tag *tag);

/* Attach the given type implementation.
 *
 * Throws:
 *
 * TYPE_TAG_ALREADY_ATTACHED
 *  If an implementation for the given type is already attached.
 */
void
type_tag_attach(
        struct type_tag_impl *tti,
        void (*impl_detach)(void *impl));

/* Detach the given type and implementation.
 *
 * Throws:
 *
 * TYPE_TAG_NOT_ATTACHED
 *  If an implementation for the given type is NOT attached.
 *
 * TYPE_TAG_STILL_ACQUIRED
 *  If the existing implementation has outstanding acquisitions.
 *
 * TYPE_TAG_MISMATCH
 *  If an implementation is provided and does NOT match the currently
 *  attached implementation.
 */
void
type_tag_detach(
        struct type_tag_impl *tti);

/* Detach all the attached types and implementations.
 *
 * Throws:
 *
 * TYPE_TAG_IS_STATIC
 *  If an implementation for the given type is statically attached.
 *
 * TYPE_TAG_NOT_ATTACHED
 *  If an implementation for the given type is NOT attached.
 *
 * TYPE_TAG_STILL_ACQUIRED
 *  If the existing implementation has outstanding acquisitions.
 *
 * TYPE_TAG_MISMATCH
 *  If an implementation is provided and does NOT match the currently
 *  attached implementation.
 */
void
type_tag_detach_all(
        struct type_tag *tag);

/* Returns 1 if the type is statically attached, otherwise returns 0. */
unsigned int
type_tag_is_static(
        struct type_tag *tag,
        const char *type);

/* Returns the number of attached type implementations. */
size_t
type_tag_attachments(
        struct type_tag *tag);

/* Returns 1 if the type has an implementation attached, otherwise returns 0. */
unsigned int
type_tag_has_a(
        struct type_tag *tag,
        const char *type);

/* Acquire the type implementation.
 *
 * Requires that the tti->tag and tti->type be set. It will overwrite the
 * tti->impl with the acquired implementation.
 *
 * Throws:
 *
 * TYPE_TAG_NOT_ATTACHED
 *  If an implementation for the given type is NOT attached.
 *
 */
void
type_tag_acquire(
        struct type_tag_impl *tti);

/* Release a previously acquired type implementation.
 *
 * Requires that at least the tti->tag and tti->type are set.
 *
 * Throws:
 *
 * TYPE_TAG_NOT_ATTACHED
 *  If an implementation for the given type is NOT attached.
 *
 * TYPE_TAG_MISMATCH
 *  If an implementation is provided and does NOT match the currently
 *  attached implementation.
 *
 * TYPE_TAG_NOT_ACQUIRED
 *  If no outstanding acquisitions for the type remain.
 */
void
type_tag_release(
        struct type_tag_impl *tti);

/* Returns the number of outstanding acquisitions for the given type
 * implementation. This will fail if the type has no attached implementation.
 *
 * Requires that at least the tti->tag and tti->type are set.
 *
 * Throws:
 *
 * TYPE_TAG_NOT_ATTACHED
 *  If an implementation for the given type is NOT attached.
 */
size_t
type_tag_acquisitions(
        struct type_tag_impl *tti);

/* Calls action(...) for each type with an implementation attached. Returns the
 * same value as the last call to action(...). If action(...) returns non-zero,
 * then it terminates immediately returning the value from the action(...)
 * call. The for_each hook is guaranteed to be called first.
 */
int
type_tag_for_each(
        struct type_tag *tag,
        void *self,
        int (*action)(
            void *self,
            struct type_tag_impl *tti));

/* The type tag interface provided by this library. */
extern struct type_tag_i TYPE_TAG_I;

/* A utility macro for acquiring and releasing an implementation.
 *
 * In the event that an exception is thrown, the implementation will be released.
 */
#define type_tag_with(tag_, type_, impl_) \
    for (struct type_tag_impl type_tag_with_impl_, \
         *type_tag_with_once_ = NULL; \
         type_tag_with_once_ == NULL && ( \
             type_tag_with_impl_.tag = tag_, \
             type_tag_with_impl_.type = type_, \
             type_tag_with_impl_.impl = NULL, \
             type_tag_acquire(&type_tag_with_impl_), \
             impl_ = type_tag_with_impl_.impl, \
             1); \
         type_tag_with_once_ = (void *)1) \
        ec_with (&type_tag_with_impl_, (ec_unwind_f)type_tag_release) \

/*** Global ***/

/* Exceptions */
extern const char TYPE_STILL_ATTACHED[];    /* Data: C String */
extern const char TYPE_ALREADY_ATTACHED[];  /* Data: C String */
extern const char TYPE_NOT_ATTACHED[];      /* Data: C String */
extern const char TYPE_STILL_ACQUIRED[];    /* Data: C String */
extern const char TYPE_NOT_ACQUIRED[];      /* Data: C String */
extern const char TYPE_INVALID_ARG[];       /* Data: C String */
extern const char TYPE_MISMATCH[];          /* Data: C String */

/* Generic type tagged structure. */
struct type_tagged {
    void *data;
    struct type_tag *tag;
};

/* Attach the type tag to the data. Requires at least tagged->data to be
 * non-NULL. If tagged->tag and tag_detach are NULL, then an empty type tag
 * will be allocated automatically.
 *
 * Throws:
 *
 * TYPE_ALREADY_ATTACHED
 *  If a type tag is already attached.
 *
 * TYPE_INVALID_ARG
 *  If tagged->tag is NULL, but tag_detach is NOT NULL.
 */
void
type_attach(
        struct type_tagged *tagged,
        void (*tag_detach)(struct type_tag *tag));

/* Detach the type tag from the data.
 *
 * Throws:
 *
 * TYPE_NOT_ATTACHED
 *  If a type tag is NOT attached.
 *
 * TYPE_STILL_ACQUIRED
 *  If the type tag has outstanding acquisitions.
 *
 * TYPE_MISMATCH
 *  If a type tag is provided and does not match the existing type tag.
 */
void
type_detach(
        struct type_tagged *tagged);

/* Returns true(1) if the data has an associated tag and false(0) otherwise. */
unsigned int
type_has_a(
        void *data);

/* Returns the number of outstanding acquisitions for the data's tag.
 *
 * Throws:
 *
 * TYPE_NOT_ATTACHED
 *  If a type tag is NOT attached.
 */
size_t
type_acquisitions(
        void *data);

/* Acquires the type tag attached to the data. Requires tagged->data to be
 * non-NULL.
 *
 * Throws:
 *
 * TYPE_NOT_ATTACHED
 *  If a type tag is NOT attached.
 */
void
type_acquire(
        struct type_tagged *tagged);

/* Releases the type tag attached to the data. Requires at least tagged->data
 * to be non-NULL.
 *
 * Throws:
 *
 * TYPE_NOT_ATTACHED
 *  If a type tag is NOT attached.
 *
 * TYPE_MISMATCH
 *  If a type tag is provided and does not match the existing type tag.
 *
 * TYPE_NOT_ACQUIRED
 *  If the type tag has NO acquisitions.
 */
void
type_release(
        struct type_tagged *tagged);

/* A utility macro for acquiring and releasing a tag.
 *
 * In the event that an exception is thrown, the tag will be released.
 */
#define type_with(data_, tag_) \
    for (struct type_tagged type_with_tagged_, \
         *type_with_tag_once_ = NULL; \
         type_with_tag_once_ == NULL && ( \
             type_with_tagged_.data = data_, \
             type_with_tagged_.tag = NULL, \
             type_acquire(&type_with_tagged_), \
             tag_ = type_with_tagged_.tag, \
             1); \
         type_with_tag_once_ = (void *)1) \
        ec_with (&type_with_tagged_, (ec_unwind_f)type_release) \

#endif /* TYPE_H */
