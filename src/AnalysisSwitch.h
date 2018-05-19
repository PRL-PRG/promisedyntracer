#ifndef __ANALYSIS_SWITCH_H__
#define __ANALYSIS_SWITCH_H__

#include <iostream>

class AnalysisSwitch {
  public:
    bool metadata;
    bool object_count_size;
    bool function;
    bool promise_type;
    bool promise_slot_mutation;
    bool promise_evaluation;
    bool strictness;
    bool side_effect;

    friend std::ostream &operator<<(std::ostream &os,
                                    const AnalysisSwitch &analysis_switch);
};

#endif /* __ANALYSIS_SWITCH_H__ */
