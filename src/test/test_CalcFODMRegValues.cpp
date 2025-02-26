#include <vector>
#include <cstdlib>
#include "csv.h"
#include "CalcFODMRegValues.h"

#include "gtest/gtest.h"

using namespace ska_mid_cbf_fodm_gen;

constexpr int INPUT_CSV_NUM_COL = 11;
constexpr int OUTPUT_CSV_NUM_COL = 8;

struct CsvInputs
{
    FoPoly fo_poly;
    uint32_t input_sample_rate;
    uint32_t output_sample_rate;
    double f_wb;
    double f_as;
    double f_ds;
    double f_scfo;
};

void parse_input_csv(
    const std::string& csv_file, 
    std::vector<CsvInputs>& inputs)
{
    io::CSVReader<INPUT_CSV_NUM_COL> in(csv_file);
    in.read_header(io::ignore_extra_column, "fo_delay_const", "fo_delay_linear",
        "fodm_start_t", "fodm_stop_t", "hodm_start_t", "input_sample_rate", 
        "output_sample_rate", "f_wb", "f_as", "f_ds", "f_scfo");
    inputs.clear();
    CsvInputs data;
    while(in.read_row(
        data.fo_poly.poly[1],
        data.fo_poly.poly[0],
        data.fo_poly.start_time_s,
        data.fo_poly.stop_time_s,
        data.fo_poly.ho_poly_start_time_s,
        data.input_sample_rate,
        data.output_sample_rate,
        data.f_wb,
        data.f_as,
        data.f_ds,
        data.f_scfo)) 
    {
        inputs.push_back(data);
    }

}

void parse_output_csv(
    const std::string& csv_file, 
    std::vector<FirstOrderDelayModelsRegisterSet>& func_output)
{
    io::CSVReader<OUTPUT_CSV_NUM_COL> in(csv_file);
    in.read_header(io::ignore_extra_column, "first_input_timestamp", "delay_constant",
        "phase_constant", "delay_linear", "phase_linear", "validity_period",
        "output_PPS", "first_output_timestamp");
    
    func_output.clear();
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
        func_output.push_back(data);
    }
}

void run_python_ref(const std::string& input_csv, const std::string& output_csv)
{
    // This generates an output csv file containing the values to compare against
    std::string cmd("python3 fodm_calc_ref.py " + input_csv + " " + output_csv);
    std::system(cmd.c_str());
}

TEST(CompareTest, Test1)
{
    std::string input_csv("fodm_test_input.csv");
    std::string output_csv("python_output_ref.csv");
    run_python_ref(input_csv, output_csv);

    std::vector<FirstOrderDelayModelsRegisterSet> python_output;
    parse_output_csv(output_csv, python_output);

    std::vector<CsvInputs> test_input;
    parse_input_csv(input_csv, test_input);

    ASSERT_EQ(test_input.size(), python_output.size());

    for (int ii = 0; ii < test_input.size(); ii++)
    {
        FirstOrderDelayModelsRegisterSet func_output = 
            CalcFodmRegValues(
                test_input[ii].fo_poly,
                test_input[ii].input_sample_rate,
                test_input[ii].output_sample_rate,
                test_input[ii].f_ds,
                test_input[ii].f_as,
                test_input[ii].f_wb,
                test_input[ii].f_scfo);
        
        EXPECT_EQ(func_output.first_input_timestamp, python_output[ii].first_input_timestamp);
        EXPECT_EQ(func_output.delay_constant, python_output[ii].delay_constant);
        EXPECT_EQ(func_output.phase_constant, python_output[ii].phase_constant);
        EXPECT_EQ(func_output.delay_linear, python_output[ii].delay_linear);
        EXPECT_EQ(func_output.phase_linear, python_output[ii].phase_linear);
        EXPECT_EQ(func_output.validity_period, python_output[ii].validity_period);
        EXPECT_EQ(func_output.output_PPS, python_output[ii].output_PPS);
        EXPECT_EQ(func_output.first_output_timestamp, python_output[ii].first_output_timestamp);
    }
}