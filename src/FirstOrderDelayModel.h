#ifndef FirstOrderDelayModel_H
#define FirstOrderDelayModel_H

#include <string> 
#include <stdio.h>
#include <stdlib.h>
#include <vector>

namespace ska_mid_cbf_fodm_gen
{

class FirstOrderDelayModel
{
public:
    FirstOrderDelayModel();

    bool process( double ho_t_start,
                  double ho_t_stop,
                  int num_ho_coeff,
                  const double* ho_poly,
                  int num_fo_poly,
                  const std::vector<double>& fo_t_start,
                  std::vector<long double>& fo_poly);

    bool process(double ho_t_start, 
                 double ho_t_stop, 
                 int num_ho_coeff, 
                 const double* ho_poly,                                    
                 int num_lsq_points, 
                 int num_fo_poly, 
                 const std::vector<double>& fo_t_start, 
                 std::vector<long double>& fo_poly);

  private:

    long double  polyval(const double* ho_poly, int num_ho_coeff, double x);
};

};

#endif