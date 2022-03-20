#pragma once

DECLARE_LOG_CATEGORY_EXTERN(LogSCPEditor, Log, All);

#ifndef CONCAT
#define CONCAT(x, y) x##y
#define CONCAT2(x, y) CONCAT(x, y)
#endif
#define DEFER(x)										 \
	::FDefer CONCAT2(_defer_, __COUNTER__)([&]() { \
		x												 \
	})

class FDefer {
	typedef TFunction<void()> F;

	F Target;
public:

	explicit FDefer(F Target) : Target(MoveTemp(Target)) {}

	~FDefer() { Target(); }
};
