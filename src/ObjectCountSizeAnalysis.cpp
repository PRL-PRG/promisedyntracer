#include "ObjectCountSizeAnalysis.h"

ObjectCountSizeAnalysis::ObjectCountSizeAnalysis(
    const tracer_state_t &tracer_state, const std::string &output_dir)
    : tracer_state_(tracer_state), output_dir_(output_dir),
      object_counts_(std::vector<int>(MAX_NUM_SEXPTYPE)),
      object_sizes_(std::vector<unsigned long long int>(MAX_NUM_SEXPTYPE)) {}

void ObjectCountSizeAnalysis::vector_alloc(const type_gc_info_t &info) {
    ++object_counts_[info.type];
    object_sizes_[info.type] += info.length * info.bytes;
}

void ObjectCountSizeAnalysis::serialize() {
    std::ofstream fout(output_dir_ + "/object-count-size.csv", std::ios::trunc);

    fout << "object_type"
         << " , "
         << "count"
         << " , "
         << "size" << std::endl;

    for (int i = 0; i < MAX_NUM_SEXPTYPE; ++i) {
        if (object_counts_[i] == 0 && object_sizes_[i] == 0)
            continue;
        fout << sexp_type_to_string((sexp_type)i) << " , " << object_counts_[i]
             << " , " << object_sizes_[i] << std::endl;
    }

    fout.close();
}
