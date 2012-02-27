#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <ec/ec.h>
#include <ecx_stdio.h>
#include <ecx_stdlib.h>

#include <Judy.h>

#include "type.h"

/*** Type Tag ***/

const char TYPE_TAG_STILL_ATTACHED[]    = "Type Tag: Still Attached";
const char TYPE_TAG_ALREADY_ATTACHED[]  = "Type Tag: Already Attached";
const char TYPE_TAG_NOT_ATTACHED[]      = "Type Tag: Not Attached";
const char TYPE_TAG_IS_STATIC[]         = "Type Tag: Is Static";
const char TYPE_TAG_STILL_ACQUIRED[]    = "Type Tag: Still Acquired";
const char TYPE_TAG_NOT_ACQUIRED[]      = "Type Tag: Not Acquired";
const char TYPE_TAG_MISMATCH[]          = "Type Tag: Mismatch";

struct type_tag {
    struct type_tag_static_i hooks;     /* Hooks for static typing. */
    Pvoid_t type_to_impl;               /* Map from type to implementation. */
};

struct impl {
    void *impl;
    void (*impl_detach)(void *impl);
    size_t acquisitions;
};

size_t
type_tag_size()
{
    return sizeof (struct type_tag);
}

size_t
type_tag_align()
{
    return __alignof__ (struct type_tag);
}

void
type_tag_init(
        struct type_tag *tag,
        const struct type_tag_static_i *hooks)
{
    /* Copy hooks. */
    if (hooks != NULL) {
        tag->hooks = *hooks;
    }
    else {
        /* Set all static hooks to NULL. */
        struct type_tag_static_i null_hooks = {
            .attachments    = NULL,
            .has_a          = NULL,

            .acquire        = NULL,
            .release        = NULL,

            .acquisitions   = NULL,
            .for_each       = NULL,
        };
        tag->hooks = null_hooks;
    }

    /* Initialize map (just needs to be NULL). */
    tag->type_to_impl = NULL;
}

void
type_tag_fini(
        struct type_tag *tag)
{
    /* Ignore null tag. */
    if (tag == NULL) return;

    /* Check if types are attached. */
    Word_t count = 0;
    JLC(count, tag->type_to_impl, 0, -1);
    if (count != 0) {
        ec_throw_str_static(TYPE_TAG_STILL_ATTACHED, "Can't finalize, types still attached.");
    }

    /* Finalize map. */
    Word_t freed = 0;
    JLFA(freed, tag->type_to_impl);
}

void
type_tag_attach(
        struct type_tag_impl *tti,
        void (*impl_detach)(void *impl))
{
    Pvoid_t *PValue = NULL;

    struct type_tag *tag = tti->tag;
    const char *type = tti->type;

    /* Check for existing type implementation. */
    JLG(PValue, tag->type_to_impl, (Word_t)type);
    if (PValue != NULL ||
        (tag->hooks.has_a != NULL &&
         tag->hooks.has_a(tag, type))) {
        /* Otherwise fail, implementation already exists. */
        char *msg = NULL;
        ecx_asprintf(&msg,
                "Type implementation for '%s' already attached.", type);
        ec_throw_str(TYPE_TAG_ALREADY_ATTACHED) msg;
    }
    else {
        struct impl *impl= ecx_malloc(sizeof(struct impl));

        impl->impl = tti->impl;
        impl->impl_detach = impl_detach;
        impl->acquisitions = 0;

        /* Insert new mapping type -> impl. */
        JLI(PValue, tag->type_to_impl, (Word_t)type);
        *PValue = impl;
    }
}

void
type_tag_detach(
        struct type_tag_impl *tti)
{
    Pvoid_t *PValue = NULL;

    struct type_tag *tag = tti->tag;
    const char *type = tti->type;

    /* Get implementation. */
    JLG(PValue, tag->type_to_impl, (Word_t)type);
    if (PValue == NULL) {
        /* Is it a static type? */
        if (tag->hooks.has_a != NULL &&
            tag->hooks.has_a(tag, type)) {
            char *msg = NULL;
            ecx_asprintf(&msg,
                    "Type implementation for '%s' is static (and cannot be detached).", type);
            ec_throw_str(TYPE_TAG_IS_STATIC) msg;
        }
        else {
            /* Otherwise fail, implementation not attached. */
            char *msg = NULL;
            ecx_asprintf(&msg,
                    "Type implementation for '%s' not attached.", type);
            ec_throw_str(TYPE_TAG_NOT_ATTACHED) msg;
        }
    }

    struct impl *impl = *PValue;

    /* Outstanding acquisitions? */
    if (impl->acquisitions != 0) {
        char *msg = NULL;

        /* Choose correct numbering. */
        const char *acq = NULL;
        const char acq1[] = "acquisition remains";
        const char acq2[] = "acquisitions remain";
        acq = impl->acquisitions == 1 ? acq1 : acq2;

        ecx_asprintf(&msg, "Can't detach because %zi %s.",
                impl->acquisitions, acq);
        ec_throw_str(TYPE_TAG_STILL_ACQUIRED) msg;
    }

    /* Provided impl pointer doesn't match attached. */
    if (tti->impl != NULL && tti->impl != impl->impl) {
        ec_throw_str_static(TYPE_TAG_MISMATCH,
                "Implementation provided doesn't match currently attached.");
    }

    /* Remove type -> impl mapping. */
    int status = 0;
    JLD(status, tag->type_to_impl, (Word_t)type);

    /* Call detach callback. */
    if (impl->impl_detach != NULL) {
        impl->impl_detach(impl->impl);
    }

    /* Free the implementation. */
    free(impl);

    /* If all the tags are removed, then free the mapping. */
    if (type_tag_attachments(tag) == 0) {
        free(tag->type_to_impl);
        tag->type_to_impl = NULL;
    }
}

void
type_tag_detach_all(
        struct type_tag *tag)
{
    Pvoid_t *PValue = NULL;
    Word_t Index = 0;

    const char *type = NULL;
    struct impl *impl = NULL;

    /* Get first type implementation. */
    JLF(PValue, tag->type_to_impl, Index);

    while (PValue != NULL) {
        type = (const char *)Index;
        impl = *PValue;

        /* Detach type implementation. */
        struct type_tag_impl tti = {
            .tag = tag,
            .type = type,
            .impl = impl->impl,
        };
        type_tag_detach(&tti);

        /* Get next type implementation. */
        JLN(PValue, tag->type_to_impl, Index);
    }
}

unsigned int
type_tag_is_static(
        struct type_tag *tag,
        const char *type)
{
    if (tag->hooks.has_a != NULL &&
        tag->hooks.has_a(tag, type)) {
        return 1;
    }

    return 0;
}

size_t
type_tag_attachments(
        struct type_tag *tag)
{
    size_t attachments = 0;

    /* Get dynamic type count. */
    JLC(attachments, tag->type_to_impl, 0, -1);

    /* Add static types. */
    if (tag->hooks.attachments != NULL) {
        attachments += tag->hooks.attachments(tag);
    }

    return attachments;
}

unsigned int
type_tag_has_a(
        struct type_tag *tag,
        const char *type)
{
    /* Is it a static type? */
    if (tag->hooks.has_a != NULL &&
        tag->hooks.has_a(tag, type)) {
        return 1;
    }

    /* Check dynamic types. */
    Pvoid_t *PValue = NULL;
    JLG(PValue, tag->type_to_impl, (Word_t)type);

    if (PValue != NULL) {
        return 1;
    }

    return 0;
}

void
type_tag_acquire(
        struct type_tag_impl *tti)
{
    struct type_tag *tag = tti->tag;
    const char *type = tti->type;

    /* Look for static types first. */
    if (tag->hooks.has_a != NULL &&
        tag->hooks.acquire != NULL &&
        tag->hooks.has_a(tag, type) != 0) {
        return tag->hooks.acquire(tti);
    }

    /* Look for dynamic types. */
    Pvoid_t *PValue = NULL;
    JLG(PValue, tag->type_to_impl, (Word_t)type);
    if (PValue == NULL) {
        char *msg = NULL;
        ecx_asprintf(&msg,
                "Type implementation for '%s' not attached.", type);
        ec_throw_str(TYPE_TAG_NOT_ATTACHED) msg;
    }

    struct impl *impl = *PValue;
    impl->acquisitions++;

    tti->impl = impl->impl;
}

void
type_tag_release(
        struct type_tag_impl *tti)
{
    struct type_tag *tag = tti->tag;
    const char *type = tti->type;

    /* Look for static types first. */
    if (tag->hooks.has_a != NULL &&
        tag->hooks.release != NULL &&
        tag->hooks.has_a(tag, type) != 0) {
        tag->hooks.release(tti);
        return;
    }

    /* Look for dynamic types. */
    Pvoid_t *PValue = NULL;
    JLG(PValue, tag->type_to_impl, (Word_t)type);
    if (PValue == NULL) {
        char *msg = NULL;
        ecx_asprintf(&msg,
                "Type implementation for '%s' not attached.", type);
        ec_throw_str(TYPE_TAG_NOT_ATTACHED) msg;
    }

    struct impl *impl = *PValue;

    /* Provided impl pointer doesn't match attached. */
    if (tti->impl != NULL && tti->impl != impl->impl) {
        ec_throw_str_static(TYPE_TAG_MISMATCH,
                "Implementation provided doesn't match currently attached.");
    }

    if (impl->acquisitions == 0) {
        ec_throw_str_static(TYPE_TAG_NOT_ACQUIRED, "No outstanding acquisitions to release.");
    }

    tti->impl = NULL;
    impl->acquisitions--;
}

size_t
type_tag_acquisitions(
        struct type_tag_impl *tti)
{
    struct type_tag *tag = tti->tag;
    const char *type = tti->type;

    /* Is it a static type? */
    if (tag->hooks.has_a != NULL &&
        tag->hooks.acquisitions != NULL &&
        tag->hooks.has_a(tag, type)) {
        return tag->hooks.acquisitions(tti);
    }

    /* Check dynamic types. */
    Pvoid_t *PValue = NULL;
    JLG(PValue, tag->type_to_impl, (Word_t)type);
    if (PValue == NULL) {
        char *msg = NULL;
        ecx_asprintf(&msg,
                "Type implementation for '%s' not attached.", type);
        ec_throw_str(TYPE_TAG_NOT_ATTACHED) msg;
    }

    struct impl *impl = *PValue;
    return impl->acquisitions;
}

int
type_tag_for_each(
        struct type_tag *tag,
        void *self,
        int (*action)(
            void *self,
            struct type_tag_impl *tti))
{
    int status = 0;

    /* First loop over static types. */
    if (tag->hooks.for_each != NULL) {
        status = tag->hooks.for_each(tag, self, action);
        if (status != 0) return status;
    }

    /* Loop over dynamic types. */
    Pvoid_t *PValue = NULL;
    Word_t Index = 0;

    const char *type = NULL;
    struct impl *impl = NULL;

    /* Get first type implementation. */
    JLF(PValue, tag->type_to_impl, Index);

    while (PValue != NULL) {
        type = (const char *)Index;
        impl = *PValue;

        struct type_tag_impl tti = {
            .tag = tag,
            .type = type,
            .impl = impl,
        };

        /* Call action. */
        status = action(self, &tti);
        if (status != 0) return status;

        /* Get next type implementation. */
        JLN(PValue, tag->type_to_impl, Index);
    }

    return status;
}

struct type_tag_i TYPE_TAG_I = {
    .size           = type_tag_size,
    .align          = type_tag_align,

    .init           = type_tag_init,
    .fini           = type_tag_fini,

    .attach         = type_tag_attach,
    .detach         = type_tag_detach,
    .detach_all     = type_tag_detach_all,

    .is_static      = type_tag_is_static,

    .attachments    = type_tag_attachments,
    .has_a          = type_tag_has_a,

    .acquire        = type_tag_acquire,
    .release        = type_tag_release,

    .acquisitions   = type_tag_acquisitions,
    .for_each       = type_tag_for_each,
};

/*** Global ***/

const char TYPE_STILL_ATTACHED[]    = "Type: Still Attached";
const char TYPE_ALREADY_ATTACHED[]  = "Type: Already Attached";
const char TYPE_NOT_ATTACHED[]      = "Type: Not Attached";
const char TYPE_STILL_ACQUIRED[]    = "Type: Still Acquired";
const char TYPE_NOT_ACQUIRED[]      = "Type: Not Acquired";
const char TYPE_INVALID_ARG[]       = "Type: Invalid Arg";
const char TYPE_MISMATCH[]          = "Type: Mismatch";

struct data_tag {
    struct type_tag *tag;
    void (*tag_detach)(struct type_tag *tag);
    size_t acquisitions;
};

/* Global per-thread map from data to type tag. */
__thread Pvoid_t data_to_dtag = NULL;

static void
free_tag(struct type_tag *tag)
{
    type_tag_fini(tag);
    free(tag);
}

void
type_attach(
        struct type_tagged *tagged,
        void (*tag_detach)(struct type_tag *tag))
{
    void *data = tagged->data;
    struct type_tag *tag = tagged->tag;

    /* Look for tag. */
    PWord_t PValue = NULL;
    JLG(PValue, data_to_dtag, (Word_t)data);

    if (PValue != NULL) {
        ec_throw_str_static(TYPE_ALREADY_ATTACHED, "Data already has a tag attached.");
    }

    /* If no tag is provided, create one. */
    if (tag == NULL) {
        if (tag_detach != NULL) {
            ec_throw_str_static(TYPE_INVALID_ARG, "Tag is NULL, but tag_detach is not.");
        }

        tag = ecx_malloc(type_tag_size());
        type_tag_init(tag, NULL);

        tagged->tag = tag;
        tag_detach = free_tag;
    }

    /* Create internal data tag. */
    struct data_tag *dtag = ecx_malloc(sizeof(struct data_tag));
    dtag->tag = tag;
    dtag->tag_detach = tag_detach;
    dtag->acquisitions = 0;

    /* Insert mapping from data to type tag. */
    PValue = NULL;
    JLI(PValue, data_to_dtag, (Word_t)data);

    *(void **)PValue = dtag;
}

void
type_detach(
        struct type_tagged *tagged)
{
    void *data = tagged->data;
    struct type_tag *tag = tagged->tag;

    /* Get tag. */
    PWord_t PValue = NULL;
    JLG(PValue, data_to_dtag, (Word_t)tagged->data);

    if (PValue == NULL) {
        ec_throw_str_static(TYPE_NOT_ATTACHED, "Data does not have a tag to detach.");
    }

    struct data_tag *dtag = (void *)*PValue;

    /* Outstanding acquisitions? */
    if (dtag->acquisitions != 0) {
        char *msg = NULL;

        /* Choose correct numbering. */
        const char *acq = NULL;
        const char acq1[] = "acquisition remains";
        const char acq2[] = "acquisitions remain";
        acq = dtag->acquisitions == 1 ? acq1 : acq2;

        ecx_asprintf(&msg, "Can't detach because %zi %s.",
                dtag->acquisitions, acq);
        ec_throw_str(TYPE_STILL_ACQUIRED) msg;
    }

    /* Provided tag doesn't match attached. */
    if (tag != NULL &&
        tag != dtag->tag) {
        ec_throw_str_static(TYPE_MISMATCH,
                "Tag provided doesn't match currently attached.");
    }

    /* Remove data to tag mapping. */
    int status = 0;
    JLD(status, data_to_dtag, (Word_t)data);

    /* Call the tag detach callback. */
    if (dtag->tag_detach != NULL) {
        dtag->tag_detach(dtag->tag);
    }

    dtag->tag = NULL;
    dtag->tag_detach = NULL;
    dtag->acquisitions = 0;

    free(dtag);
    dtag = NULL;
}

unsigned int
type_has_a(
        void *data)
{
    /* Look for type tag. */
    PWord_t PValue = NULL;
    JLG(PValue, data_to_dtag, (Word_t)data);

    if (PValue != NULL) {
        return 1;
    }

    return 0;
}

size_t
type_acquisitions(
        void *data)
{
    /* Look for type tag. */
    Pvoid_t *PValue = NULL;
    JLG(PValue, data_to_dtag, (Word_t)data);

    if (PValue == NULL) {
        ec_throw_str_static(TYPE_NOT_ATTACHED, "Data does not have a tag attached.");
    }

    struct data_tag *dtag = (void *)*PValue;
    return dtag->acquisitions;
}

void
type_acquire(
        struct type_tagged *tagged)
{
    void *data = tagged->data;

    /* Get data tag. */
    PWord_t PValue = NULL;
    JLG(PValue, data_to_dtag, (Word_t)data);

    if (PValue == NULL) {
        ec_throw_str_static(TYPE_NOT_ATTACHED, "Data does not have a tag attached.");
    }

    struct data_tag *dtag = (void *)*PValue;
    dtag->acquisitions++;

    tagged->tag = dtag->tag;
}

void
type_release(
        struct type_tagged *tagged)
{
    void *data = tagged->data;
    struct type_tag *tag = tagged->tag;

    /* Get data tag. */
    PWord_t PValue = NULL;
    JLG(PValue, data_to_dtag, (Word_t)data);

    if (PValue == NULL) {
        ec_throw_str_static(TYPE_NOT_ATTACHED, "Data does not have a tag attached.");
    }

    struct data_tag *dtag = (void *)*PValue;

    if (tag != NULL &&
        tag != dtag->tag) {
        ec_throw_str_static(TYPE_MISMATCH,
                "Provided tag doesn't match currently attached.");
    }

    if (dtag->acquisitions == 0) {
        ec_throw_str_static(TYPE_NOT_ACQUIRED, "No outstanding acquisitions to release.");
    }

    dtag->acquisitions--;
}

