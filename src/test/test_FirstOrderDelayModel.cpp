#include <random>
#include <fstream>
#include <boost/multiprecision/cpp_bin_float.hpp> 
#include "FirstOrderDelayModel.h"

#include "gtest/gtest.h"

using namespace ska_mid_cbf_fodm_gen;


/** save_to_file
*
* Method: save_to_file
* Description	:
*       save FO poly into a cvs file - DEBUG only
*/
void save_to_file( const std::string& file_name, 
                   const std::vector<double>& fo_t_start, 
                   const std::vector<long double>& fo_poly, 
                   int num_fo_poly )
{
    std::ofstream myfile (file_name);
    if (myfile.is_open())
    {
        for (int i = 0; i < num_fo_poly; i++)
        {
            myfile << std::fixed << std::setprecision(12) << fo_t_start[i] << ",";
            myfile << std::fixed << std::setprecision(12) << fo_t_start[i+1]<< ","; 
            myfile << std::fixed << std::setprecision(16) << fo_poly[i*2] << ","; 
            myfile << std::fixed << std::setprecision(16) << fo_poly[i*2+1] << "\n"; 
        }

    }
    myfile.close();
}

long double polyval(const double* ho_poly, int num_ho_coeff, double t)
{
    using namespace boost::multiprecision;
    cpp_bin_float_50 y = ho_poly[0];
    for (int ii = 1; ii < num_ho_coeff; ii++) 
    {
        y *= t;
        y += ho_poly[ii];
    }
    return static_cast<long double>(y);
}


class FirstOrderDelayModelTest : public ::testing::Test
{
protected:    

    const int NUM_FO_POLYS = 1000;
    const int HO_POLY_LEN = 8;
    const int NUM_HO_COEFF = 6;
    const int NUM_LSQ_POINTS = 1000;
    const int NUM_TEST_POINTS = 2000;

    // TBD: what thresholds to use?
    const long double MAX_ABS_ERR = 1.0e-8;
    const long double MAX_CUMULATED_ERR = 1.0e-7;

    FirstOrderDelayModelTest()
    {
        fo_polys_.resize(NUM_FO_POLYS*2);
        t_fo_poly_.resize(NUM_FO_POLYS+1);
    }

    // Returns idx such that t_fo_poly_[idx] <= t <= t_fo_poly_[idx+1].
    // The FO poly to evaluate would be fo_polys_[idx*2] and fo_polys_[idx*2+1]
    int find_fo_poly_idx(double t, double fo_poly_interval)
    {
        // this assumes time is always positive
        return int( (t - t_fo_poly_[0]) / fo_poly_interval);
    }

    void lsq_fit_max_error_test_common(const double* ho_poly, double fo_poly_interval, bool dump_csv = false) {
        // Generate num_fo_polys FODMs from the start of HODM
        double hodm_t_start = ho_poly[0];
        double hodm_t_stop = ho_poly[1];
        for (int ii = 0 ; ii < NUM_FO_POLYS+1; ii++) 
        {
            t_fo_poly_[ii] = hodm_t_start + fo_poly_interval * ii;
        }
        
        // LSQ fit
        FirstOrderDelayModel test_model;
        bool result = test_model.process(hodm_t_start, hodm_t_stop, NUM_HO_COEFF, (ho_poly+2), NUM_LSQ_POINTS, NUM_FO_POLYS, t_fo_poly_, fo_polys_);
        EXPECT_TRUE(result);

        // Sample random points between within the range of generated FO polynomials
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(t_fo_poly_[0], t_fo_poly_[NUM_FO_POLYS]);

        long double max_abs_err = 0.0;   // Checks for max error of FO fit against HO poly at any single point 
        long double cumulated_err = 0.0; // Checks for fitting bias. Ideally should be close to 0.
        for (int ii = 0; ii < NUM_TEST_POINTS; ii++)
        {
            double t = dis(gen);
            int fo_idx = find_fo_poly_idx(t, fo_poly_interval);
            ASSERT_LT(fo_idx, t_fo_poly_.size());
            long double val_from_ho = polyval((ho_poly+2), NUM_HO_COEFF, t - hodm_t_start);
            long double val_from_fo = fo_polys_[fo_idx*2+1] + fo_polys_[fo_idx*2] * (t - t_fo_poly_[fo_idx]);
            long double diff = val_from_ho - val_from_fo;
            max_abs_err = abs(diff) > max_abs_err ? abs(diff) : max_abs_err;
            cumulated_err = cumulated_err + diff;
        }

        EXPECT_LT(max_abs_err, MAX_ABS_ERR);
        EXPECT_LT(abs(cumulated_err), MAX_CUMULATED_ERR);

        if (dump_csv)
        {
            std::string csv_file("FOTCP_SRC1_Receptor1_polX.csv");
            save_to_file(csv_file, t_fo_poly_, fo_polys_, NUM_FO_POLYS);
        }
    }

    std::vector<double> t_fo_poly_;
    std::vector<long double> fo_polys_;
};

// Test with a HODM generated from MATLAB
TEST_F(FirstOrderDelayModelTest, LsqFitMaxErrorTest)
{
    double ho_poly[HO_POLY_LEN] = {
        1.0000000000000E+01,3.0000000000000E+01,4.513184775273619937E-17,3.016563864250689452E-14,
        1.077965332504251907E-09,-7.680455181115336256E-05,-1.216193871021531203E+00,28887.4980 };
    double fo_poly_interval = 1.0 / 128;

    lsq_fit_max_error_test_common(ho_poly, fo_poly_interval);
}

// Test with a HODM that starts at t = 0.
TEST_F(FirstOrderDelayModelTest, HodmStart0MaxErrorTest)
{
    double ho_poly[HO_POLY_LEN] = {
        0.0000000000000E+00,3.0000000000000E+01,4.513184775273619937E-17,3.016563864250689452E-14,
        1.077965332504251907E-09,-7.680455181115336256E-05,-1.216193871021531203E+00,28887.4980 };
    double fo_poly_interval = 1.0 / 128;

    lsq_fit_max_error_test_common(ho_poly, fo_poly_interval);
}

// Test with a HODM with only a constant coefficient
TEST_F(FirstOrderDelayModelTest, ConstHodmLsqFitMaxErrorTest)
{
    double ho_poly[HO_POLY_LEN] = {
        1.0000000000000E+01,3.0000000000000E+01,0.0000000000000E+00,0.0000000000000E+00,
        0.0000000000000E+00,0.0000000000000E+00,0.0000000000000E+00,28887.4980 };
    double fo_poly_interval = 1.0 / 128;

    lsq_fit_max_error_test_common(ho_poly, fo_poly_interval);
}

// Test with a HODM with only constant and linear coefficients
TEST_F(FirstOrderDelayModelTest, LinearHodmLsqFitMaxErrorTest)
{
    double ho_poly[HO_POLY_LEN] = {
        0.0000000000000E+00,3.0000000000000E+01,0.0000000000000E+00,0.0000000000000E+00,
        0.0000000000000E+00,0.0000000000000E+00,1.8070000000000E+00,-55910.224908};
    double fo_poly_interval = 1.0 / 128;

    lsq_fit_max_error_test_common(ho_poly, fo_poly_interval);
}

// Test generating FODMs beyond the HODM duration
TEST_F(FirstOrderDelayModelTest, BeyondDurationTest)
{
    // 10s HODM, try to generate 1000 FODMs * 0.02s interval = 20s total
    double ho_poly[HO_POLY_LEN] = {
        0.0000000000000E+00,1.0000000000000E+01,4.513184775273619937E-17,3.016563864250689452E-14,
        1.077965332504251907E-09,-7.680455181115336256E-05,-1.216193871021531203E+00,28887.4980 };
    double fo_poly_interval = 0.02;

    double hodm_t_start = ho_poly[0];
    double hodm_t_stop = ho_poly[1];
    for (int ii = 0 ; ii < NUM_FO_POLYS+1; ii++) 
    {
        t_fo_poly_[ii] = hodm_t_start + fo_poly_interval * ii;
    }
    
    FirstOrderDelayModel test_model;
    bool result = test_model.process(hodm_t_start, hodm_t_stop, NUM_HO_COEFF, (ho_poly+2), NUM_LSQ_POINTS, NUM_FO_POLYS, t_fo_poly_, fo_polys_);
    EXPECT_FALSE(result);
}


TEST_F(FirstOrderDelayModelTest, BeforeStartTimeTest)
{
    // HODM starts at t = 50. Request to generate FODMs from t < 50.
    double ho_poly[HO_POLY_LEN] = {
        5.0000000000000E+01,6.0000000000000E+01,4.513184775273619937E-17,3.016563864250689452E-14,
        1.077965332504251907E-09,-7.680455181115336256E-05,-1.216193871021531203E+00,28887.4980 };
    double fo_poly_interval = 0.01;

    double hodm_t_start = ho_poly[0];
    double hodm_t_stop = ho_poly[1];
    double fodm_start = 45.0; // FODM starts at t = 45
    for (int ii = 0 ; ii < NUM_FO_POLYS+1; ii++) 
    {
        t_fo_poly_[ii] = fodm_start + fo_poly_interval * ii;
    }
    
    FirstOrderDelayModel test_model;
    bool result = test_model.process(hodm_t_start, hodm_t_stop, NUM_HO_COEFF, (ho_poly+2), NUM_LSQ_POINTS, NUM_FO_POLYS, t_fo_poly_, fo_polys_);
    EXPECT_FALSE(result);
}

// This test additionally dumps the FODMs to a CSV file, the MATLAB/fit.m can read
// the CSV file, compute and plot the errors vs HODM.
TEST_F(FirstOrderDelayModelTest, DumpToFileTest)
{
    double ho_poly[HO_POLY_LEN] = {27.0, 37.0, -0.0000000000000777454450817839, 0.0000000000013703522293644218,-0.0000000010937608165606331000, 0.0000767171471221868880000000, 1.2203389596567424000000000000, -28854.6047841830330000000000000000};
    double fo_poly_interval = 0.01;

    lsq_fit_max_error_test_common(ho_poly, fo_poly_interval, true);
}