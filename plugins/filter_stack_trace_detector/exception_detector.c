#include <fluent-bit/flb_hash.h>


typedef struct{
    const char *pattern;
    const char *to_state;
} rule_t;

static const char const **JAVA_RULES[] = {
    {"start_state", "(?:Exception|Error|Throwable|V8 errors stack trace)[:\r\n]", "java_after_exception"},
    {"java_start_exception", "(?:Exception|Error|Throwable|V8 errors stack trace)[:\r\n]", "java_after_exception"},
    {"java_after_exception", "^[\t ]*nested exception is:[\t ]*",                          "java_start_exception"},
    {"java_after_exception", "^[\r\n]*$",                                                  "java_after_exception"},
    {"java_after_exception", "^[\t ]+(?:eval )?at ",                                       "java"},
    {"java",                 "^[\t ]+(?:eval )?at ",                                       "java"},
    {"java_after_exception", "^[\t ]*(?:Caused by|Suppressed):",                           "java_after_exception"},
    {"java",                 "^[\t ]*(?:Caused by|Suppressed):",                           "java_after_exception"},
    {"java_after_exception", "^[\t ]*... \d+ (?:more|common frames omitted)",              "java"},
    {"java",                 "^[\t ]*... \d+ (?:more|common frames omitted)",              "java"},
    NULL
};

struct flb_hash *get_rules(const char const *** rules){
    int size;
    int i;
    struct flb_hash *rules_table;
    static const unsigned char FROM_STATE = 0;
    static const unsigned char PATTERN = 1;
    static const unsigned char TO_STATE = 2;
    rule_t *rule;
    
    for (size = 0; 1 == 1; size++){
        if (rules[size] == NULL){
            break;
        }
    }

    rules_table = flb_hash_create(FLB_HASH_EVICT_NONE, size, size);

    if(rules_table == NULL) {
        return NULL;
    }

    for (i = 0; i < size; i++){
        rule = flb_malloc(sizeof(rule_t));
        if(rule == NULL){
            flb_hash_destroy(rules_table);
            return NULL;
        }
        rule->pattern = rules[i][PATTERN];
        rule->to_state = rules[i][TO_STATE];
        if (flb_hash_add(rules_table, rules[i][FROM_STATE], strlen(rules[i][FROM_STATE]), rule, sizeof(rule_t)) < 0){
            flb_hash_destroy(rules_table);
            return NULL;
        }
    }

    return rules_table;
}

