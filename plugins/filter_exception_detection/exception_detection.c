 /* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  Fluent Bit
 *  ==========
 *  Copyright (C) 2019      The Fluent Bit Authors
 *  Copyright (C) 2015-2018 Treasure Data Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <stdio.h>
#include <sys/types.h>

#include <fluent-bit/flb_info.h>
#include <fluent-bit/flb_mem.h>
#include <fluent-bit/flb_str.h>
#include <fluent-bit/flb_filter.h>
#include <fluent-bit/flb_utils.h>
#include <fluent-bit/flb_pack.h>
#include <fluent-bit/flb_log.h>
#include <fluent-bit/flb_time.h>
#include <msgpack.h>
#include "stdlib.h"

#include "exception_detection.h"

#define PLUGIN_NAME "filter_exception_detection"
#define SPLIT_DELIMITER '|'
#define KEY_DEPTH 10

/*load_field_key_list split @str into list of string representing the depth of a nested key.
The split is base on SPLIT_DELIMITER*/
static inline int load_field_key_list(char *str, struct mk_list *the_list,
                                      size_t * list_size)
{
    struct mk_list *split;
    struct mk_list *head = NULL;
    struct field_key *fk;
    struct flb_split_entry *entry;

    mk_list_init(the_list);
    *list_size = 0;
    if (str != NULL) {
        split = flb_utils_split(str, SPLIT_DELIMITER, KEY_DEPTH);
        if (mk_list_size(split) < 1) {
            return 0;
        }
        mk_list_foreach(head, split) {
            fk = flb_malloc(sizeof(struct field_key));
            if (!fk) {
                flb_error("[%s] Not enough memory!", PLUGIN_NAME);
                flb_utils_split_free(split);
                return -1;
            }

            entry = mk_list_entry(head, struct flb_split_entry, _head);

            fk->key = strndup(entry->value, entry->len);
            fk->key_len = entry->len;
            mk_list_add(&fk->_head, the_list);
            (*list_size)++;
        }

        flb_utils_split_free(split);
    }
    return 0;
}


static int cb_exception_detection_init(struct flb_filter_instance *f_ins,
                            struct flb_config *config, void *data)
{
    struct flb_filter_exception_detection_ctx *ctx;
    char *str;

    /* Create context */
    ctx = flb_malloc(sizeof(struct flb_filter_exception_detection_ctx));
    if (!ctx) {
        flb_error("[%s] Not enough memory!", PLUGIN_NAME);
        return -1;
    }

    /* print informational status */
    str = flb_filter_get_property("print_status", f_ins);
    if (str != NULL) {
        ctx->print_status = flb_utils_bool(str);
    }
    else {
        ctx->print_status = FLB_FALSE;
    }

    str = flb_filter_get_property("stream", f_ins);
    if (load_field_key_list(str, &ctx->log_fields, &ctx->log_fields_depth)) {
        return -1;
    }

    /* Set our context */
    flb_filter_set_context(f_ins, ctx);

    return 0;
}

static int cb_exception_detection_filter(void *data, size_t bytes,
                              char *tag, int tag_len,
                              void **out_buf, size_t * out_size,
                              struct flb_filter_instance *f_ins,
                              void *context, struct flb_config *config)
{
    msgpack_unpacked result;
    msgpack_object root;
    size_t off = 0;
    (void) f_ins;
    (void) config;
    msgpack_sbuffer tmp_sbuf;
    msgpack_packer tmp_pck;

    /* Create temporal msgpack buffer */
    msgpack_sbuffer_init(&tmp_sbuf);
    msgpack_packer_init(&tmp_pck, &tmp_sbuf, msgpack_sbuffer_write);

    // Records come in the format,
    //
    // [ TIMESTAMP, { K1:V1, K2:V2, ...} ],
    // [ TIMESTAMP, { K1:V1, K2:V2, ...} ]
    //
    // Example record,
    // [1123123, {"Mem.total"=>4050908, "Mem.used"=>476576, "Mem.free"=>3574332 } ]

    /* Iterate each item array and apply rules */
    msgpack_unpacked_init(&result);
    while (msgpack_unpack_next(&result, data, bytes, &off)) {
        root = result.data;
        if (root.type != MSGPACK_OBJECT_ARRAY) {
            continue;
        }

        printf("-------------before-------------------------\n");
        msgpack_object_print(stdout, root);
        memset(data, 0, off);
        printf("-------------after-------------------------\n");
        msgpack_object_print(stdout, root);
        printf("-------------end-------------------------\n");
        msgpack_pack_object(&tmp_pck, root);
    }
    msgpack_unpacked_destroy(&result);

    /* link new buffers */
    *out_buf = tmp_sbuf.data;
    *out_size = tmp_sbuf.size;

    return FLB_FILTER_MODIFIED;
}

static int cb_exception_detection_exit(void *data, struct flb_config *config)
{
    struct flb_filter_size_throttle_ctx *ctx = data;
    flb_free(ctx);
    return 0;
}

struct flb_filter_plugin filter_exception_detection_plugin = {
    .name = "exception_detection",
    .description = "Detect exception on multilines and united them in one line",
    .cb_init = cb_exception_detection_init,
    .cb_filter = cb_exception_detection_filter,
    .cb_exit = cb_exception_detection_exit,
    .flags = 0
};
