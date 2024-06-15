#define min(a, b) (a) < (b) ? (a) : (b)
#define PARAMETER_NAME_LEN 64
#define PARAMETER_TYPE_LEN 16
//#define DEBUG_PARAMETERS

enum parameter_set {twix_config=0, twix_dicom=1, twix_meas=2, twix_measyaps=3, twix_phoenix=4, twix_spice=5};


#include "measyaps.c"
#include "parse_config_dicom_meas_protocols.c"


