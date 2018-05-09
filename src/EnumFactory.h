#ifndef __ENUM_FACTORY_H__
#define __ENUM_FACTORY_H__

// Obtained from https://stackoverflow.com/a/202511

// expansion macro for enum value definition
#define ENUM_VALUE(name, assign) name assign,

// expansion macro for enum to string conversion
#define ENUM_CASE(name, assign)                                                \
    case name:                                                                 \
        return #name;

// expansion macro for string to enum conversion
#define ENUM_STRCMP(name, assign)                                              \
    if (string == #name)                                                       \
        return name;

/// declare the access function and define enum values
#define DECLARE_ENUM(EnumType, ENUM_DEF, enum_to_string, string_to_enum)       \
    enum EnumType { ENUM_DEF(ENUM_VALUE) };                                    \
    std::string enum_to_string(EnumType dummy);                              \
    EnumType string_to_enum(const std::string &string);

/// define the access function names
#define DEFINE_ENUM(EnumType, ENUM_DEF, enum_to_string, string_to_enum)        \
    std::string enum_to_string(EnumType value) {                             \
        switch (value) {                                                       \
            ENUM_DEF(ENUM_CASE)                                                \
            default:                                                           \
                return ""; /* handle input error */                            \
        }                                                                      \
    }                                                                          \
    EnumType string_to_enum(const std::string &string) {                     \
        ENUM_DEF(ENUM_STRCMP)                                                  \
        return (EnumType)0; /* handle input error */                           \
    }

#endif /* __ENUM_FACTORY_H__ */
