#ifndef __ANALYSIS_SWITCH_H__
#define __ANALYSIS_SWITCH_H__

class AnalysisSwitch {
  public:
    bool metadata;
    bool object_count_size;
    bool function;
    bool promise;
    bool strictness;
    bool side_effect;
};

#endif /* __ANALYSIS_SWITCH_H__ */
