// clang-format off
#include <php.h>
#include "compatibility.h"

#include "env_config.h"
// clang-format on

#include <SAPI.h>
#include <Zend/zend_types.h>
#include <php.h>
#include <stdint.h>

#include "coms_curl.h"

char *ddtrace_strdup(const char *c);
#define EQUALS(stra, stra_len, literal_strb) \
    (stra_len == (sizeof(literal_strb) - 1) && memcmp(stra, literal_strb, sizeof(literal_strb) - 1) == 0)

static inline char *get_local_env_or_sapi_env_cased(char *name TSRMLS_DC) {
    char *env = NULL, *tmp = getenv(name);
    if (tmp) {
        env = ddtrace_strdup(tmp);
    } else {
        // reading sapi_getenv from within writer thread can and will lead to undefined behaviour
        if (ddtrace_in_writer_thread()) {
            return NULL;
        }

        tmp = sapi_getenv(name, strlen(name) TSRMLS_CC);
        if (tmp) {
            // convert PHP memory to pure C memory since this could be used in non request contexts too
            // currently we're not using permanent C memory anywhere, while this could be applied here
            // it seems more practical to simply use "C memory" instead of having 3rd way to free and allocate memory
            env = ddtrace_strdup(tmp);
            efree(tmp);
        }
    }

    return env;
}

static inline char *get_local_env_or_sapi_env(char *name TSRMLS_DC) {
    char *env = get_local_env_or_sapi_env_cased(name TSRMLS_C);

    if (!env) {
        char *envname = ddtrace_strdup(name);
        char *cptr = envname;
        size_t envname_length = strlen(name);
        while (*cptr && (size_t)(cptr - envname) < envname_length) {
            *cptr = tolower((unsigned char)*cptr);
            cptr++;
        }
        env = get_local_env_or_sapi_env_cased(envname);
        // fprintf(stderr, "aa %s: %s\n", envname, env);

        free(envname);
    }

    return env;
}

BOOL_T ddtrace_get_bool_config(char *name, BOOL_T def TSRMLS_DC) {
    char *env = get_local_env_or_sapi_env(name TSRMLS_CC);
    if (!env) {
        return def;
    }

    size_t len = strlen(env);
    if (len > sizeof("false")) {
        free(env);
        return def;
    }

    zend_str_tolower(env, len);

    zend_bool rv = def;
    if (EQUALS(env, len, "1") || EQUALS(env, len, "true")) {
        rv = 1;
    } else if (EQUALS(env, len, "0") || EQUALS(env, len, "false")) {
        rv = 0;
    }

    free(env);
    return rv;
}

int64_t ddtrace_get_int_config(char *name, int64_t def TSRMLS_DC) {
    char *env = get_local_env_or_sapi_env(name TSRMLS_CC);
    if (!env) {
        return def;
    }

    char *endptr = env;

    long long result = strtoll(env, &endptr, 10);

    if (endptr == env) {
        free(env);

        return def;
    }
    free(env);

    return result;
}

uint32_t ddtrace_get_uint32_config(char *name, uint32_t def TSRMLS_DC) {
    int64_t value = ddtrace_get_int_config(name, def TSRMLS_CC);
    if (value < 0 || value > UINT32_MAX) {
        value = def;
    }
    return value;
}

char *ddtrace_get_c_string_config(char *name TSRMLS_DC) {
    char *env = get_local_env_or_sapi_env(name TSRMLS_CC);
    if (!env) {
        return NULL;
    } else {
        return env;
    }
}

char *ddtrace_get_c_string_config_with_default(char *name, const char *def TSRMLS_DC) {
    char *env = get_local_env_or_sapi_env(name TSRMLS_CC);
    if (!env) {
        if (def) {
            return ddtrace_strdup(def);
        } else {
            return NULL;
        }
    } else {
        return env;
    }
}

#if !defined(__clang__) && (__GNUC__ >= 7)
// disable checks since some GCC trigger false positives
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif  //! defined(__clang__) && __GNUC__ >= 7

char *ddtrace_strdup(const char *c) {
    size_t len = strlen(c);
    char *dup = malloc(len + 1);

    if (dup != NULL) {
        strncpy(dup, c, len + 1);
    }
    return dup;
}

#if !defined(__clang__) && __GNUC__ >= 7
#pragma GCC diagnostic pop
#endif  //! defined(__clang__) && __GNUC__ >= 7
