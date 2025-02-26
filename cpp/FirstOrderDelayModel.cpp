#include "FirstOrderDelayModel.h"

#include <math.h>
#include <fstream>
#include <iomanip>
#include <cassert> 

/** FirstOrderDelayModel CONSTRUCTOR
*
* Method: FirstOrderDelayModel
* Arguments   :
*    : param logger : The logger used to send logs to the TLS
*    : type logger : shared_ptr<MessageLogger>
*
*/
FirstOrderDelayModel::FirstOrderDelayModel() {}


/** process
*
* Method: process
* Input params:    
*       ho_t_start: high order poly start time [s]
*       ho_t_stop: high order poly start time [s]
*       num_ho_coeff: number of coeffecients in the high order poly
*       ho_poly: high order polynomial
*       num_lsq_points: number of points used for first order approximation
*       num_fo_poly: number of first order delay models
*       fo_t_start: pointer to an array of length num_fo_poly + 1, containing the start time stamps of the FOs
*                   to be derived AND the end time of the last FO as the last element. 
* Output params :
*       fo_poly: pointer to store first order polynomials (array length = num_fo_poly and unit = [ns/s , ns])
* Description	:
*       uses the Linear Least Square Fitting Algorithm to find a 
*       2nd order polynomial fit to a ho polynomial
*       [a, b] = [(X'X)^-1] * [X'Y]
*       X is a 2-by-num_lsq_points matrix, first column of X are 1s, and 2nd coloumn are the lsq point time stamps.
*       Y is a 1-by-num_lsq_points vector, they are values calcualted at the lsq points.
*/
void FirstOrderDelayModel::process(
    double ho_t_start, 
    double ho_t_stop, 
    int num_ho_coeff, 
    double *ho_poly,                                    
    int num_lsq_points, 
    int num_fo_poly, 
    double *fo_t_start, 
    double *fo_poly
)
{
    double t_s;
    double y_t;  

    double xtx[4] = {0};
    double xtx_inv[4] = {0};
    double xty[2] = {0};

    double det;
    double t_fitting_incr;
    cout.precision(15);
    int time_elapsed  = 0;
        
    assert (fo_t_start!=NULL);
    assert (fo_poly!=NULL);
    assert (ho_poly!=NULL);

    assert (num_lsq_points>=1);
    assert (num_ho_coeff>=2);
    assert (num_fo_poly>=1);

    for (int i = 0; i < num_fo_poly; i++)
    {   
        xtx[1] = 0;
        xtx[3] = 0;    
        xty[0] = 0;    
        xty[1] = 0;    
        
        if (fo_t_start[i+1] > ho_t_stop | fo_t_start[i] < ho_t_start | fo_t_start[i+1] < fo_t_start[i])
            LOG_ERROR_FIRST_ORDER_DELAY_MODEL("FirstOrderDelayModel::process() - Time stamp error");
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
}

/** save_to_file
*
* Method: save_to_file
* Description	:
*       save FO poly into a cvs file - DEBUG only
*/
void FirstOrderDelayModel::save_to_file(string file_name, double *fo_t_start, double *fo_poly, int num_fo_poly)
{
    ofstream myfile (file_name);
    LOG_DEBUG_FIRST_ORDER_DELAY_MODEL("write to file");
    if (myfile.is_open())
    {
        for (int i = 0; i < num_fo_poly; i++)
        {
            myfile << std::fixed << std::setprecision(10) << fo_t_start[i] << ",";    
            myfile << std::fixed << std::setprecision(10) << fo_t_start[i+1]<< ","; 
            myfile << std::fixed << std::setprecision(10) << fo_poly[i*2] << ","; 
            myfile << std::fixed << std::setprecision(10) << fo_poly[i*2+1] << "\n"; 
        }

    }
    myfile.close();
    LOG_DEBUG_FIRST_ORDER_DELAY_MODEL("write to file complete");
}

void FirstOrderDelayModel::log_message_(const char* file, int line, const std::string message )
{
    cout << "[" << file << ":" << line << "] " << message << endl;
}