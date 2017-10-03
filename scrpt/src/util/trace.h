#ifndef TRACE_H
#define TRACE_H

void TraceMessage(const char* message);

// TODO: This should be converted to varargs for perf reasons
#define __Trace(level, message) \
{ \
    std::ostringstream os; \
    os.precision(3); \
    os << std::fixed << "[" #level "] " << COMPONENTNAME ":" << __LINE__ << " " << message; \
    TraceMessage(os.str().c_str()); \
}

#define TraceVerbose(message) __Trace(Verbose, message)
#define TraceInfo(message) __Trace(Info, message)
#define TraceWarning(message) __Trace(Warning, message)
#define TraceError(message) __Trace(Error, message)

#ifdef _DEBUG
#define AssertFail(message) { TraceError(message); __debugbreak(); }
#define AssertNotNull(value) { if ((value) == nullptr) AssertFail("Variable is null: " #value); }
#define Assert(condition, message) { if (!(condition)) AssertFail(message); }
#else
#define AssertFail(message)
#define AssertNotNull(value)
#define Assert(condition, message)
#endif

#endif
