#include "FirstOrderDelayModel.h"

#include <math.h>
#include <fstream>
#include <iomanip>
#include <cassert> 
#include <boost/multiprecision/cpp_bin_float.hpp> 
using namespace boost::multiprecision;

namespace ska_mid_cbf_fodm_gen
{

/** FirstOrderDelayModel CONSTRUCTOR
*
* Method: FirstOrderDelayModel
*
*/
FirstOrderDelayModel::FirstOrderDelayModel() 
{
    // no-op
}

/** 
* Generates first order polynomials from a high order polynomial. This method
* evaluates the HO polynomial at each start and stop time of each first order
* polynomial, and fits a straight line between the two points.
*
* Input params:    
*       ho_t_start: high order poly start time [s]
*       ho_t_stop: high order poly start time [s]
*       num_ho_coeff: number of coeffecients in the high order poly
*       ho_poly: high order polynomial
*       num_fo_poly: number of first order delay models
*       fo_t_start: pointer to an array of length num_fo_poly + 1, containing the start time stamps of the FOs
*                   to be derived AND the end time of the last FO as the last element. 
*
* Output params :
*       fo_poly: pointer to store first order polynomials (array length = num_fo_poly and unit = [ns/s , ns])
*
* Returns :
*       false if there is any unexpected HO and FO polynomial time parameter, true otherwise.
*/
bool FirstOrderDelayModel::process( double ho_t_start,
                                    double ho_t_stop,
                                    int num_ho_coeff,
                                    const double* ho_poly,
                                    int num_fo_poly,
                                    const std::vector<double>& fo_t_start,
                                    std::vector<long double>& fo_poly)
{
    fo_poly.resize(num_fo_poly * 2); // 2 coefficients for each FO poly. 
    bool time_inputs_ok = true;

    for (int ii = 0; ii < num_fo_poly; ii++)
    {
        if (fo_t_start[ii+1] > ho_t_stop | fo_t_start[ii] < ho_t_start | fo_t_start[ii+1] < fo_t_start[ii]) 
        {
            time_inputs_ok = false;
        }
        double t1 = fo_t_start[ii] - ho_t_start;
        double t2 = fo_t_start[ii+1] - ho_t_start;
        long double y1 = polyval(ho_poly, num_ho_coeff, t1);
        long double y2 = polyval(ho_poly, num_ho_coeff, t2);
        long double m = (y2 - y1) / (t2 - t1);
        fo_poly[ii*2] = m;
        fo_poly[ii*2 + 1] = y1;
    }

    return time_inputs_ok;
}

/**
 * Evaluate the polynomial at x using Horner's method
 * 
 * Input Params:
 *   ho_poly - polynomial to evaluate. Highest degree coefficient first.
 *   num_ho_coeff - number of coefficients in the polynomial
 *   x - evaluate the polynomial at this point
 * 
 * Returns:
 *   the evaluated value
 */
long double FirstOrderDelayModel::polyval(const double* ho_poly, int num_ho_coeff, double x)
{
    cpp_bin_float_50 y = ho_poly[0];
    for (int ii = 1; ii < num_ho_coeff; ii++) 
    {
        y *= x;
        y += ho_poly[ii];
    }
    return static_cast<long double>(y);
}


/** process
*  Description:
*       uses the Linear Least Square Fitting Algorithm to find a 
*       2nd order polynomial fit to a ho polynomial
*       [a, b] = [(X'X)^-1] * [X'Y]
*       X is a 2-by-num_lsq_points matrix, first column of X are 1s, and 2nd coloumn are the lsq point time stamps.
*       Y is a 1-by-num_lsq_points vector, they are values calcualted at the lsq points.
*
* Input params:    
*       ho_t_start: high order poly start time [s]
*       ho_t_stop: high order poly start time [s]
*       num_ho_coeff: number of coeffecients in the high order poly
*       ho_poly: high order polynomial
*       num_lsq_points: number of points used for first order approximation
*       num_fo_poly: number of first order delay models
*       fo_t_start: pointer to an array of length num_fo_poly + 1, containing the start time stamps of the FOs
*                   to be derived AND the end time of the last FO as the last element. 
*
* Output params :
*       fo_poly: pointer to store first order polynomials (array length = num_fo_poly and unit = [ns/s , ns])
*
* Returns :
*       false if there is any unexpected HO and FO polynomial time parameter, true otherwise.
*/
bool FirstOrderDelayModel::process(double ho_t_start, 
                                   double ho_t_stop, 
                                   int num_ho_coeff, 
                                   const double* ho_poly,
                                   int num_lsq_points, 
                                   int num_fo_poly, 
                                   const std::vector<double>& fo_t_start, 
                                   std::vector<long double>& fo_poly)
{
    double t_s;
    double y_t;  
    long double xtx[4] = {0};
    long double xtx_inv[4] = {0};
    long double xty[2] = {0};
    long double det;
    double t_fitting_incr;
    int time_elapsed  = 0;
        
    assert (ho_poly!=NULL);
    assert (num_lsq_points>=1);
    assert (num_ho_coeff>=2);
    assert (num_fo_poly>=1);

    bool time_inputs_ok = true;

    fo_poly.resize(num_fo_poly * 2); // 2 coefficients for each FO poly. 

    for (int i = 0; i < num_fo_poly; i++)
    {   
        xtx[1] = 0;
        xtx[3] = 0;    
        xty[0] = 0;    
        xty[1] = 0;    
        
        if (fo_t_start[i+1] > ho_t_stop | fo_t_start[i] < ho_t_start | fo_t_start[i+1] < fo_t_start[i])
        {            
            time_inputs_ok = false;
        }

        t_fitting_incr = (fo_t_start[i+1] -  fo_t_start[i])/num_lsq_points;
        for (int j = 0; j <= num_lsq_points; j++) 
        {           
            t_s = fo_t_start[i] + j*t_fitting_incr - ho_t_start;
            //evaluate the y value at the corresponding time sample
            y_t = ho_poly[0];
            for	(int k = 1; k < num_ho_coeff; k++){
                y_t = y_t*t_s + ho_poly[k];
            }
            t_s = j*t_fitting_incr;
            xtx[1] += t_s;
            xtx[3] += t_s * t_s;
            xty[0] += y_t;
            xty[1] += t_s * y_t;
        }
        xtx[0] = num_lsq_points+1;
        xtx[2] = xtx[1];       
        det = xtx[0] * xtx[3] - xtx[1] * xtx[2];
        xtx_inv[0] = xtx[3] / det;
        xtx_inv[3] = xtx[0] / det;
        xtx_inv[1] = xtx[1] / det * -1.0;
        xtx_inv[2] = xtx[2] / det * -1.0;	
        
        fo_poly[2*i] = xtx_inv[2]*xty[0] + xtx_inv[3]*xty[1];//coeff_icient of 2nd order polynomial
        fo_poly[2*i+1] = xtx_inv[0]*xty[0] + xtx_inv[1]*xty[1];//coeff_icient of 2nd order polynomial
    }

    return time_inputs_ok;
}

};