/***
 * test_CompareCalcFodmRegisterValues.cpp
 * 
 * The unit test driver for the CalcFodmRegisterValues free function.
 * The test input is a CSV file. Each row specifies a FODM, 
 * sampling rates, and the frequency shifts. The test driver runs
 * the inputs through the reference python code (based on FW notebook)
 * and the function, and compares the final values that would be written 
 * to the FODM register. The values are expected to be equal.
 * 
 ***/
#include <vector>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <random>
#include "CalcFodmRegisterValues.h"
#include "csv.h"

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
        data.fo_poly.start_time_ms,
        data.fo_poly.stop_time_ms,
        data.fo_poly.ho_poly_start_time_ms,
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
    std::vector<FirstOrderDelayModelRegisterValues>& func_output)
{
    io::CSVReader<OUTPUT_CSV_NUM_COL> in(csv_file);
    in.read_header(io::ignore_extra_column, "first_input_timestamp", "delay_constant",
        "phase_constant", "delay_linear", "phase_linear", "validity_period",
        "output_PPS", "first_output_timestamp");
    
    func_output.clear();
    FirstOrderDelayModelRegisterValues data;
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

// Generate more rows to test a more extensive range of parameters
// The test_input should already contain several rows of input parameters.
// The new rows are added to test_input.
// Additionally, the new CSV is written to the file input_csv_added_params.
void generate_more_rows(
    int rows_to_generate,
    const std::string& input_csv_added_params, 
    std::vector<CsvInputs>& test_input)
{
    // Random generators
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> k_val_distr(1, 2222);
    std::uniform_real_distribution<> delay_const_distr(double(-400000.0), double(400000.0)); // ns
    std::uniform_real_distribution<> delay_linear_distr(double(-10.0), double(10.0)); // ns / s
    std::uniform_int_distribution<> freq_slice_idx_distr(1, 9);
    std::uniform_int_distribution<> ho_poly_start_time_s_distr(720000000, 990000000);
    std::uniform_int_distribution<> nth_fodm_distr(0, 999);

    // TODO: using 1/128 interval is hitting the limit of double precision.
    //std::array<double, 2> fodm_interval_choices_ms = { 10.0, 100.0/128.0 };
    std::array<double, 1> fodm_interval_choices_ms = { 10.0 };
    const uint32_t output_sample_rate = 220200960;

    int original_size = test_input.size();

    // Re-use the frequency shifts in the existing rows
    int jj = 0;
    int kk = 0;
    for (int ii = 0; ii < rows_to_generate; ii++, jj++, kk++)
    {
        if (jj >= original_size) { jj = 0; }
        if (kk >= fodm_interval_choices_ms.size()) { kk = 0; }

        CsvInputs input;
        int fsi = freq_slice_idx_distr(gen);

        input.fo_poly.poly[1] = delay_const_distr(gen);
        input.fo_poly.poly[0] = delay_linear_distr(gen);
        // generate HODM start time with whole seconds
        input.fo_poly.ho_poly_start_time_ms = floor(ho_poly_start_time_s_distr(gen) * 1000.0); 
        input.fo_poly.start_time_ms = input.fo_poly.ho_poly_start_time_ms + fodm_interval_choices_ms[kk] * nth_fodm_distr(gen);
        input.fo_poly.stop_time_ms = input.fo_poly.start_time_ms + fodm_interval_choices_ms[kk];
        input.input_sample_rate = 220000000 + k_val_distr(gen) * 100;
        double f_ds = round(-9.0 * fsi * double(input.input_sample_rate) / 10); // the number should be an integer really...rounding just to be safe
        double f_scfo = round(9.0 * fsi * (double(input.input_sample_rate) - double(output_sample_rate)) / 10);
        input.output_sample_rate = output_sample_rate;
        input.f_wb = test_input[jj].f_wb;
        input.f_as = test_input[jj].f_as;
        input.f_ds = f_ds;
        input.f_scfo = f_scfo;

        test_input.push_back(input);
    }

    // Write to a new input file
    std::ofstream ofs(input_csv_added_params);
    ofs << "fo_delay_const,fo_delay_linear,fodm_start_t,fodm_stop_t,hodm_start_t,input_sample_rate,output_sample_rate,f_wb,f_as,f_ds,f_scfo" << std::endl;
    for (int ii = 0; ii < test_input.size(); ii++)
    {
        CsvInputs& row = test_input[ii];
        ofs << std::setprecision(10) << row.fo_poly.poly[1] << "," << row.fo_poly.poly[0] << "," 
            << std::setprecision(16) << row.fo_poly.start_time_ms << "," << row.fo_poly.stop_time_ms << "," 
            << row.fo_poly.ho_poly_start_time_ms << "," << row.input_sample_rate << "," << row.output_sample_rate << ","
            << row.f_wb << "," << row.f_as << "," << row.f_ds << "," << row.f_scfo << std::endl;
    }
}

void run_python_ref(const std::string& input_csv, const std::string& output_csv)
{
    // This generates an output csv file containing the values to compare against
    std::string cmd("python3 fodm_calc_ref.py " + input_csv + " " + output_csv);
    std::system(cmd.c_str());
}

// GTest test function
TEST(CalcFodmRegisterValuesTest, PythonCompare)
{
    std::string input_csv("fodm_test_input.csv");
    std::string input_csv_added_params("fodm_test_input_added_params.csv");
    
    std::vector<CsvInputs> test_input;
    parse_input_csv(input_csv, test_input);
    int ROWS_TO_GENERATE = 0; // TODO: something is still off with the generated input rows. Set to non-zero when it's ready.
    generate_more_rows(ROWS_TO_GENERATE, input_csv_added_params, test_input);

    // run the CSV input through the python code
    std::string output_csv("python_output_ref.csv");
    run_python_ref(input_csv_added_params, output_csv);

    std::vector<FirstOrderDelayModelRegisterValues> python_output;
    parse_output_csv(output_csv, python_output);

    // run the CSV input through the C++ code
    test_input.clear();
    parse_input_csv(input_csv_added_params, test_input);
    ASSERT_EQ(test_input.size(), python_output.size());

    for (int ii = 0; ii < test_input.size(); ii++)
    {
        FirstOrderDelayModelRegisterValues func_output = 
            CalcFodmRegisterValues(
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