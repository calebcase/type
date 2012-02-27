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

START_TEST(data_basic)
{
    char data[] = "data";

    struct integer int_impl = {
        .i = 0,
    };

    struct type_tagged tagged = {
        .data = data,
        .tag = NULL,
    };

    type_attach(&tagged, NULL);

    struct type_tag_impl tti = {
        .tag = tagged.tag,
        .type = integer,
        .impl = &int_impl,
    };

    type_tag_attach(&tti, NULL);

    struct type_tagged tagged_received = {
        .data = data,
        .tag = NULL,
    };

    type_acquire(&tagged_received);

    struct type_tag_impl tti_received = {
        .tag = tagged_received.tag,
        .type = integer,
        .impl = NULL,
    };

    type_tag_acquire(&tti_received);

    struct integer *impl = tti_received.impl;

    impl->i++;
    fail_unless(int_impl.i == 1);

    struct type_tag *tag = NULL;
    impl = NULL;
    type_with (data, tag) {
        type_tag_with (tag, integer, impl) {
            impl->i++;
            fail_unless(int_impl.i == 2);
        }
    }

    type_tag_release(&tti_received);
    type_tag_detach(&tti);

    type_release(&tagged_received);
    type_detach(&tagged_received);
}
END_TEST

Suite *
data_suite(void)
{
    Suite *s = suite_create("Data");

    TCase *tc_d = tcase_create("Data");
    tcase_add_test(tc_d, data_basic);
    /* tcase_add_test(tc_d, data_iterator); */
    suite_add_tcase(s, tc_d);

    return s;
}

int
main(void)
{
    int failed = 0;

    SRunner *sr = srunner_create(data_suite());

    srunner_run_all(sr, CK_NORMAL);
    failed = srunner_ntests_failed(sr);

    srunner_free(sr);

    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
