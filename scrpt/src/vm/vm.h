#pragma once

// Design
// * Stack based
// * Direct call and indirect call
// * String table for static strings
// * Strings are immutable
// * Function table for run time arrity checks and ip lookup
// * Stack pointer, frame pointer, ip
// * Stack has args, return ip, return frame pointer, and locals
// * Differentiate between floats and integers transparently
// * Callstack generation for runtime errors
// * Debug information