#ifndef PROMISE_DYNTRACER_PROMISE_USE_H
#define PROMISE_DYNTRACER_PROMISE_USE_H

#include <string>

enum class PromiseUse {
    ValueLookup = 0,
    ValueAssign,
    EnvironmentLookup,
    EnvironmentAssign,
    ExpressionLookup,
    ExpressionAssign,
    Force,
    Count
};

inline std::string to_string(const PromiseUse &use) {
    switch (use) {
        case PromiseUse::ValueLookup:
            return "VL";
        case PromiseUse::ValueAssign:
            return "VA";
        case PromiseUse::EnvironmentLookup:
            return "RL";
        case PromiseUse::EnvironmentAssign:
            return "RA";
        case PromiseUse::ExpressionLookup:
            return "EL";
        case PromiseUse::ExpressionAssign:
            return "EA";
        case PromiseUse::Force:
            return "FR";
        case PromiseUse::Count:
            return "COUNT";
    }

    return "PromiseUse::UNDEFINED";
}

#endif /* PROMISE_DYNTRACER_PROMISE_USE_H */
