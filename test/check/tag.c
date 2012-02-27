/* Copyright 2011 Caleb Case
 *
 * This file is part of the Type Library.
 *
 * The Type Library is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * The Type Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the Type Library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <check.h>
#include <stdlib.h>

#include <ec/ec.h>
#include <ecx_stdlib.h>
#include <type.h>

const char integer[] = "integer";
struct integer {
    int i;
};

START_TEST(tag_basic)
{
    struct type_tag *tag = ecx_malloc(type_tag_size());

    type_tag_init(tag, NULL);

    struct integer int_impl = {
        .i = 0,
    };

    struct type_tag_impl tti = {
        .tag = tag,
        .type = integer,
        .impl = &int_impl,
    };

    type_tag_attach(&tti, NULL);

    struct type_tag_impl tti_received = {
        .tag = tag,
        .type = integer,
        .impl = NULL,
    };

    type_tag_acquire(&tti_received);
    struct integer *impl = tti_received.impl;

    impl->i++;
    fail_unless(int_impl.i == 1);

    impl = NULL;
    type_tag_with (tag, integer, impl) {
        impl->i++;
        fail_unless(int_impl.i == 2);
    }

    type_tag_release(&tti_received);
    type_tag_detach(&tti);

    type_tag_fini(tag);

    free(tag);
}
END_TEST

Suite *
tag_suite(void)
{
    Suite *s = suite_create("Type Tag");

    TCase *tc_tt = tcase_create("Type Tag");
    tcase_add_test(tc_tt, tag_basic);
    suite_add_tcase(s, tc_tt);

    return s;
}

int
main(void)
{
    int failed = 0;

    SRunner *sr = srunner_create(tag_suite());

    srunner_run_all(sr, CK_NORMAL);
    failed = srunner_ntests_failed(sr);

    srunner_free(sr);

    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
