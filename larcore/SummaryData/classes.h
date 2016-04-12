#include "art/Persistency/Common/Wrapper.h"

#include "larcore/SummaryData/RunData.h"
#include "larcore/SummaryData/POTSummary.h"

template class std::vector<sumdata::RunData>;
template class art::Wrapper<sumdata::RunData>;
template class std::vector<sumdata::POTSummary>;
template class art::Wrapper<sumdata::POTSummary>;

