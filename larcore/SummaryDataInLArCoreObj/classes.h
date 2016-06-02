#include "canvas/Persistency/Common/Wrapper.h"

#include "larcoreobj/SummaryData/RunData.h"
#include "larcoreobj/SummaryData/POTSummary.h"

template class std::vector<sumdata::RunData>;
template class art::Wrapper<sumdata::RunData>;
template class std::vector<sumdata::POTSummary>;
template class art::Wrapper<sumdata::POTSummary>;

