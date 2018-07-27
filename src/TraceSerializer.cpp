#include "TraceSerializer.h"

const std::string TraceSerializer::OPCODE_FUNCTION_BEGIN = "fnb";
const std::string TraceSerializer::OPCODE_FUNCTION_FINISH = "fnf";
const std::string TraceSerializer::OPCODE_ARGUMENT_PROMISE_ASSOCIATE = "apa";
const std::string TraceSerializer::OPCODE_PROMISE_CREATE = "prc";
const std::string TraceSerializer::OPCODE_PROMISE_BEGIN = "prb";
const std::string TraceSerializer::OPCODE_PROMISE_FINISH = "prf";
const std::string TraceSerializer::OPCODE_PROMISE_VALUE_LOOKUP = "pvl";
const std::string TraceSerializer::OPCODE_PROMISE_EXPRESSION_LOOKUP = "pel";
const std::string TraceSerializer::OPCODE_PROMISE_ENVIRONMENT_LOOKUP = "prl";
const std::string TraceSerializer::OPCODE_PROMISE_VALUE_ASSIGN = "pva";
const std::string TraceSerializer::OPCODE_PROMISE_EXPRESSION_ASSIGN = "pea";
const std::string TraceSerializer::OPCODE_PROMISE_ENVIRONMENT_ASSIGN = "pra";
const std::string TraceSerializer::OPCODE_ENVIRONMENT_CREATE = "enc";
const std::string TraceSerializer::OPCODE_ENVIRONMENT_ASSIGN = "ena";
const std::string TraceSerializer::OPCODE_ENVIRONMENT_REMOVE = "enr";
const std::string TraceSerializer::OPCODE_ENVIRONMENT_DEFINE = "end";
const std::string TraceSerializer::OPCODE_ENVIRONMENT_LOOKUP = "enl";
