#include "ArgumentPromiseState.h"

const int ArgumentPromiseState::ENL = 0;
const int ArgumentPromiseState::ENA = 1;
const int ArgumentPromiseState::EXL = 2;
const int ArgumentPromiseState::EXA = 3;
const int ArgumentPromiseState::VAL = 4;
const int ArgumentPromiseState::VAA = 5;

const std::vector<std::string> ArgumentPromiseState::SLOT_NAMES{
    "ENL", "ENA", "EXL", "EXA", "VAL", "VAA"};
