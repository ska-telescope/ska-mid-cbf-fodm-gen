#include <vector>
#include "csv.h"

#include "CalcFODMRegValues.h"

using namespace ska_mid_cbf_fodm_gen;

constexpr int CSV_NUM_COL = 8;

void parse_csv(
    const std::string& csv_file, 
    std::vector<FirstOrderDelayModelsRegisterSet>& reg_set)
{
    io::CSVReader<CSV_NUM_COL> in(csv_file);
    in.read_header(io::ignore_extra_column, "first_input_timestamp", "delay_constant",
        "phase_constant", "delay_linear", "phase_linear", "validity_period",
        "output_PPS", "first_output_timestamp");
    
    reg_set.clear();
    FirstOrderDelayModelsRegisterSet data;
    while ( in.read_row(
        data.first_input_timestamp, 
        data.delay_constant, 
        data.phase_constant,
        data.delay_linear,
        data.phase_linear,
        data.validity_period,
        data.output_PPS,
        data.first_output_timestamp) )
    {
        reg_set.push_back(data);
    }
}