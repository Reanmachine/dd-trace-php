--TEST--
Test that clocks return > 0
--FILE--
<?php
var_dump(ddtrace_monotonic_nsec() > 0);
var_dump(ddtrace_monotonic_usec() > 0);
var_dump(ddtrace_realtime_nsec() > 0);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
