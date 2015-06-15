#ifndef __TYPE__
#define __TYPE__
//#define DEBUG

#define FTYPE double
#define BLOCK_SIZE 16 // Blocking to allow better caching

#define RANDSEEDVAL 100
#define DEFAULT_NUM_TRIALS  102400

typedef struct
{
  int Id;
  FTYPE dSimSwaptionMeanPrice;
  FTYPE dSimSwaptionStdError;
  FTYPE dStrike;
  FTYPE dCompounding;
  FTYPE dMaturity;
  FTYPE dTenor;
  FTYPE dPaymentInterval;
  int iN;
  FTYPE dYears;
  int iFactors;
  FTYPE *pdYield;
  FTYPE *ppdFactors;
} parm;

#endif //__TYPE__
