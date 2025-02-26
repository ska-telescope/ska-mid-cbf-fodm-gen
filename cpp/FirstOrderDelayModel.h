#ifndef FirstOrderDelayModel_H
#define FirstOrderDelayModel_H

#include <string.h> 
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

#define LOG_DEBUG_FIRST_ORDER_DELAY_MODEL( msg ) \
  log_message_(__FILE__, __LINE__, msg);
#define LOG_INFO_FIRST_ORDER_DELAY_MODEL( msg ) \
  log_message_(__FILE__, __LINE__, msg);
#define LOG_WARN_FIRST_ORDER_DELAY_MODEL( msg ) \
  log_message_(__FILE__, __LINE__, msg);
#define LOG_ERROR_FIRST_ORDER_DELAY_MODEL( msg ) \
  log_message_(__FILE__, __LINE__, msg);
#define LOG_FATAL_FIRST_ORDER_DELAY_MODEL( msg ) \
  log_message_(__FILE__, __LINE__, msg);

class FirstOrderDelayModel
{
public:
    FirstOrderDelayModel();
    void process(double ho_t_start, 
                 double ho_t_stop, 
                 int num_ho_coeff, 
                 double *ho_poly,                                    
                 int num_lsq_points, 
                 int num_fo_poly, 
                 double *fo_t_start, 
                 double *fo_poly);
    void save_to_file(string file_name, double *fo_tms, double *fo_poly, int num_FO_DModel);

  private:

  /** Private function log_message_
   * @brief Used by logging macro. Log to Tango Log System if MessageLogger
   *        is provided. Otherwise logs to stdout.
   *  
   * @param file file name
   * @param line line number
   * @param severity severity
   * @param message log message
   */
  void log_message_( const char* file, 
    int line,
    const std::string message );
};

#endif