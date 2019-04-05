#include <fluent-bit/flb_hash.h>


typedef struct{
    const char *pattern;
    const char *to_state;
} rule_t;

static const char const * start_state = "start_state";


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


static const char const *supported_languages[] = {
    "java"
};

struct flb_hash *get_rules(const char *** rules){
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

/*This function accepts array os trings representing the names of the programming languages
 we want to detect for exceptions.*/
struct flb_hash *get_rules_by_names(const char const ** rules_names){
    int size = 0;
    int i;
    struct flb_hash *all_language_rules_table = NULL;
    struct flb_hash *language_rules_table = NULL;
    if (rules_names){
        while(rules_names[size]){
            size++;
        }
    }

    if (size){
        all_language_rules_table = flb_hash_create(FLB_HASH_EVICT_NONE, size, size);
        if(all_language_rules_table == NULL) {
            return NULL;
        }

        for(i = 0; i < size; i++){

            //check if such rule allreade exists
            language_rules_table = NULL;
            flb_hash_get(all_language_rules_table, rules_names[i], strlen(rules_names[i]), &language_rules_table, sizeof(struct flb_hash *));
            if (language_rules_table){
                continue;
            }


            if(strcmp(rules_names[i], "java") == 0){
                language_rules_table = get_rules(JAVA_RULES);
                if(language_rules_table == NULL){
                    continue;
                }
                if (flb_hash_add(all_language_rules_table, "java", 4, language_rules_table, sizeof(struct flb_hash *)) < 0){
                    flb_hash_destroy(language_rules_table);
                }
            }
            //put here the other supported languadges
            else {
                //TODO: log unknown language
            }
        }
    }else{
        all_language_rules_table = get_rules_by_names(supported_languages);
    }

    return all_language_rules_table;
}

typedef struct {
    char * state;
    struct flb_hash *rules;
} exception_detector_t;

void exception_detector_init(exception_detector_t *ed){
    ed->state = start_state;
    ed->rules = NULL;
}

int exception_detector_set_languages_rules(exception_detector_t *ed, const char ** languages){
    if (ed == NULL || languages == NULL){
        return 0;
    }

    if (ed->rules != NULL){
        //TODO: delete previous rules. 
    }

    ed->rules = get_rules_by_names(languages);
    if (ed->rules == NULL){
        return 0;
    }

    return 1;
}

static int exception_detector_transition(exception_detector_t *ed, const char *line){
    struct mk_list *head;
    struct flb_hash_entry *entry;
    
    mk_list_foreach(head, &ht->windows->entries){

    }
}
