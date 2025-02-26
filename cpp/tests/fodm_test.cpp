#include <gtest/gtest.h>
#include "FirstOrderDelayModel.h"

#define NUM_FO_POINTS 100
#define NUM_COEFF 7
#define MAX_FO_POLY 4010
#define NUM_FO_DMODEL 1000

class FirstOrderDelayModelTest : public ::testing::Test {
public:
  FirstOrderDelayModel test_model;
};

// Demonstrate some basic assertions.
TEST_F(FirstOrderDelayModelTest, BasicSuccess) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}

// Demonstrate some basic assertions.
TEST_F(FirstOrderDelayModelTest, BasicFailure) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "hello");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}

/**
 * Test that runs the FO delay model processing and saves the
 * output to a CSV file.
 */
TEST_F (FirstOrderDelayModelTest, FODMTest) {
  double ho_poly[10] = {27, 37, 0.0000000000000016765247099798, -0.0000000000000777454450817839, 0.0000000000013703522293644218, -0.0000000010937608165606331000, 0.0000767171471221868880000000, 1.2203389596567424000000000000, -28854.6047841830330000000000000000};
  double * fo_poly = new double[2 * MAX_FO_POLY];
  double * t_fo_poly = new double[2 * MAX_FO_POLY];
  double t_start = ho_poly[0], t_stop = ho_poly[1];
  
  for (int i = 0; i < NUM_FO_DMODEL + 1; i ++) {
    t_fo_poly[i] = i * 0.01 + t_start;
  }
      
  test_model.process(
    t_start,
    t_stop,
    NUM_COEFF,
    (ho_poly + 2),
    NUM_FO_POINTS,
    NUM_FO_DMODEL,
    t_fo_poly,
    fo_poly
  );
  
  test_model.save_to_file("FOTCP_SRC1_Receptor1_polX.csv", t_fo_poly, fo_poly, NUM_FO_DMODEL);
  
  std::cout << fo_poly[1] << std::endl;
}