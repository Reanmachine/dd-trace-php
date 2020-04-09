#include "engine_hooks.h"

#include <php.h>

#include "clocks.h"
#include "configuration.h"
#include "ddtrace.h"

ZEND_EXTERN_MODULE_GLOBALS(ddtrace);

static zend_op_array *(*_prev_compile_file)(zend_file_handle *file_handle, int type TSRMLS_DC);

static void _compile_minit(void);
static void _compile_mshutdown(void);

void ddtrace_opcode_minit(void);
void ddtrace_opcode_mshutdown(void);

void ddtrace_engine_hooks_minit(void) {
    ddtrace_opcode_minit();
    _compile_minit();
}
void ddtrace_engine_hooks_mshutdown(void) {
    _compile_mshutdown();
    ddtrace_opcode_mshutdown();
}

static zend_op_array *_dd_compile_file(zend_file_handle *file_handle, int type TSRMLS_DC) {
    zend_op_array *res;
    ddtrace_monotonic_usec_t start = ddtrace_monotonic_usec();
    res = _prev_compile_file(file_handle, type TSRMLS_CC);
    DDTRACE_G(compile_time_microseconds) += (int64_t)(ddtrace_monotonic_usec() - start);
    return res;
}

static void _compile_minit(void) {
    if (get_dd_trace_measure_compile_time()) {
        _prev_compile_file = zend_compile_file;
        zend_compile_file = _dd_compile_file;
    }
}

static void _compile_mshutdown(void) {
    if (get_dd_trace_measure_compile_time() && zend_compile_file == _dd_compile_file) {
        zend_compile_file = _prev_compile_file;
    }
}

void ddtrace_compile_time_reset(TSRMLS_D) { DDTRACE_G(compile_time_microseconds) = 0; }

int64_t ddtrace_compile_time_get(TSRMLS_D) { return DDTRACE_G(compile_time_microseconds); }

extern inline void ddtrace_backup_error_handling(ddtrace_error_handling *eh, zend_error_handling_t mode TSRMLS_DC);
extern inline void ddtrace_restore_error_handling(ddtrace_error_handling *eh TSRMLS_DC);
#if PHP_VERSION_ID < 70000
extern inline void ddtrace_maybe_clear_exception(TSRMLS_D);
#else
extern inline void ddtrace_maybe_clear_exception(void);
#endif
