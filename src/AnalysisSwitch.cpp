#include "AnalysisSwitch.h"

std::ostream &operator<<(std::ostream &os,
                         const AnalysisSwitch &analysis_switch) {
    os << std::endl
       << "Metadata Analysis               : " << analysis_switch.metadata
       << std::endl
       << "Object Count Size Analysis      : "
       << analysis_switch.object_count_size << std::endl
       << "Promise Type Analysis           : " << analysis_switch.promise_type
       << std::endl
       << "Promise Evaluation Analysis     : "
       << analysis_switch.promise_evaluation << std::endl
       << "Promise Slot Mutation Analysis  : "
       << analysis_switch.promise_slot_mutation << std::endl
       << "Function Analysis               : " << analysis_switch.function
       << std::endl
       << "Strictness Analysis             : " << analysis_switch.strictness
       << std::endl
       << "Side Effect Analysis            : " << analysis_switch.side_effect
       << std::endl;

    return os;
}
