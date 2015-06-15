//HJM_Securities.cpp
//Routines to compute various security prices using HJM framework (via Simulation).
//Authors: Mark Broadie, Jatin Dewanwala
//Collaborator: Mikhail Smelyanskiy, Jike Chong, Intel

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>

#include "nr_routines.h"
#include "HJM.h"
#include "HJM_Securities.h"
#include "HJM_type.h"

#ifdef ENABLE_THREADS
#include <pthread.h>
#define MAX_THREAD 1024
#endif //ENABLE_THREADS

int NUM_TRIALS = DEFAULT_NUM_TRIALS;
int nThreads = 1;
int nSwaptions = 1;
int iN = 11;
FTYPE dYears = 5.5; 
int iFactors = 3; 
parm *swaptions;

void* worker(void *arg){
  int tid = *((int *)arg);
  FTYPE pdSwaptionPrice[2];

  int chunksize = nSwaptions / nThreads;
  int beg = tid * chunksize;
  int end = (tid == nThreads - 1 ? nSwaptions : (tid + 1) * chunksize);

  for(int i = beg; i < end; i++) {
		FTYPE ddelt = (FTYPE)(swaptions[i].dYears / swaptions[i].iN);
		int iSwapVectorLength = (int)(swaptions[i].iN - swaptions[i].dMaturity / ddelt + 0.5);
		FTYPE *ppdHJMPath = dmatrix(iN, iN * BLOCK_SIZE),
					*pdForward = dvector(iN),
					*ppdDrifts = dmatrix(iFactors, iN - 1),
					*pdTotalDrift = dvector(iN - 1);
		FTYPE *pdDiscountingRatePath = dvector(iN * BLOCK_SIZE),
					*pdPayoffDiscountFactors = dvector(iN * BLOCK_SIZE),
					*pdSwapRatePath = dvector(iSwapVectorLength * BLOCK_SIZE),
					*pdSwapDiscountFactors = dvector(iSwapVectorLength * BLOCK_SIZE),
					*pdSwapPayoffs = dvector(iSwapVectorLength);
		FTYPE *pdZ = dmatrix(iFactors, iN * BLOCK_SIZE),
					*randZ = dmatrix(iFactors, iN * BLOCK_SIZE);
		FTYPE *pdexpRes = dvector((iN - 1) * BLOCK_SIZE);

    int iSuccess = HJM_Swaption_Blocking(pdSwaptionPrice, swaptions[i].dStrike,
			swaptions[i].dCompounding, swaptions[i].dMaturity,
			swaptions[i].dTenor, swaptions[i].dPaymentInterval,
			swaptions[i].iN, swaptions[i].iFactors, swaptions[i].dYears,
			swaptions[i].pdYield, swaptions[i].ppdFactors,
			100, NUM_TRIALS, BLOCK_SIZE, 0,
			ppdHJMPath, pdForward, ppdDrifts, pdTotalDrift,
			pdDiscountingRatePath, pdPayoffDiscountFactors, pdSwapRatePath, pdSwapDiscountFactors, pdSwapPayoffs,
			pdZ, randZ, pdexpRes);
		assert(iSuccess == 1);

		free(ppdHJMPath);
		free(pdForward);
		free(ppdDrifts);
		free(pdTotalDrift);
		free(pdDiscountingRatePath);
		free(pdPayoffDiscountFactors);
		free(pdSwapRatePath);
		free(pdSwapDiscountFactors);
		free(pdSwapPayoffs);
		free(pdZ);
		free(randZ);
		free(pdexpRes);

    swaptions[i].dSimSwaptionMeanPrice = pdSwaptionPrice[0];
    swaptions[i].dSimSwaptionStdError = pdSwaptionPrice[1];
  }

  return NULL;    
}

void parseOpt(int argc, char** argv) {
  if(argc == 1)
  {
    fprintf(stderr," usage: \n\t-ns [number of swaptions (should be > number of threads]\n\t-sm [number of simulations]\n\t-nt [number of threads]\n"); 
    exit(1);
  }

  for (int j=1; j<argc; j++) {
    if (!strcmp("-sm", argv[j])) {NUM_TRIALS = atoi(argv[++j]);}
    else if (!strcmp("-nt", argv[j])) {nThreads = atoi(argv[++j]);} 
    else if (!strcmp("-ns", argv[j])) {nSwaptions = atoi(argv[++j]);} 
    else {
      fprintf(stderr," usage: \n\t-ns [number of swaptions (should be > number of threads]\n\t-sm [number of simulations]\n\t-nt [number of threads]\n"); 
    }
  }

  if(nSwaptions < nThreads) {
    nSwaptions = nThreads; 
  }
}

FTYPE* getFactors() {
  // initialize input dataset
  FTYPE* factors = dmatrix(iFactors, iN - 1);
  //the three rows store vol data for the three factors
  factors[0]= .01;
  factors[1]= .01;
  factors[2]= .01;
  factors[3]= .01;
  factors[4]= .01;
  factors[5]= .01;
  factors[6]= .01;
  factors[7]= .01;
  factors[8]= .01;
  factors[9]= .01;

  factors[(iN - 1)]= .009048;
  factors[(iN - 1) + 1]= .008187;
  factors[(iN - 1) + 2]= .007408;
  factors[(iN - 1) + 3]= .006703;
  factors[(iN - 1) + 4]= .006065;
  factors[(iN - 1) + 5]= .005488;
  factors[(iN - 1) + 6]= .004966;
  factors[(iN - 1) + 7]= .004493;
  factors[(iN - 1) + 8]= .004066;
  factors[(iN - 1) + 9]= .003679;

  factors[(iN - 1) * 2]= .001000;
  factors[(iN - 1) * 2 + 1]= .000750;
  factors[(iN - 1) * 2 + 2]= .000500;
  factors[(iN - 1) * 2 + 3]= .000250;
  factors[(iN - 1) * 2 + 4]= .000000;
  factors[(iN - 1) * 2 + 5]= -.000250;
  factors[(iN - 1) * 2 + 6]= -.000500;
  factors[(iN - 1) * 2 + 7]= -.000750;
  factors[(iN - 1) * 2 + 8]= -.001000;
  factors[(iN - 1) * 2 + 9]= -.001250;

	return factors;
}

void initSwaption(FTYPE* factors) {
  // setting up multiple swaptions
	int i, j, k;
  swaptions = (parm *)malloc(sizeof(parm)*nSwaptions);

  for (i = 0; i < nSwaptions; i++) {
    swaptions[i].Id = i;
    swaptions[i].iN = iN;
    swaptions[i].iFactors = iFactors;
    swaptions[i].dYears = dYears;

    swaptions[i].dStrike = (double)i / (double)nSwaptions;
    swaptions[i].dCompounding = 0;
    swaptions[i].dMaturity = 1;
    swaptions[i].dTenor = 2.0;
    swaptions[i].dPaymentInterval = 1.0;

    swaptions[i].pdYield = dvector(iN);
    swaptions[i].pdYield[0] = 0.1;
    for(j = 1; j <= swaptions[i].iN; j++) swaptions[i].pdYield[j] = swaptions[i].pdYield[j - 1] + 0.005;

    swaptions[i].ppdFactors = dmatrix(swaptions[i].iFactors, swaptions[i].iN - 1);
    for(k = 0; k < swaptions[i].iFactors; k++)
      for(j = 0; j < swaptions[i].iN - 1; j++)
        swaptions[i].ppdFactors[k * (iN - 1) + j] = factors[k * (iN - 1) + j];
  }

}
//Please note: Whenever we type-cast to (int), we add 0.5 to ensure that the value is rounded to the correct number. 
//For instance, if X/Y = 0.999 then (int) (X/Y) will equal 0 and not 1 (as (int) rounds down).
//Adding 0.5 ensures that this does not happen. Therefore we use (int) (X/Y + 0.5); instead of (int) (X/Y);
int main(int argc, char *argv[])
{
  int i, j;
  FTYPE *factors = NULL;

#ifdef PARSEC_VERSION
  printf("PARSEC Benchmark Suite\n");
  fflush(NULL);
#endif //PARSEC_VERSION

	parseOpt(argc, argv);
  printf("Number of Simulations: %d,  Number of threads: %d Number of swaptions: %d\n", NUM_TRIALS, nThreads, nSwaptions);
#ifdef ENABLE_THREADS
  pthread_t* threads;
  pthread_attr_t pthread_custom_attr;

  if ((nThreads < 1) || (nThreads > MAX_THREAD)) {
    fprintf(stderr,"Number of threads must be between 1 and %d.\n", MAX_THREAD);
    exit(1);
  }
  threads = (pthread_t*)malloc(nThreads * sizeof(pthread_t));
  pthread_attr_init(&pthread_custom_attr);

  if ((nThreads < 1) || (nThreads > MAX_THREAD)) {
    fprintf(stderr,"Number of threads must be between 1 and %d.\n", MAX_THREAD);
    exit(1);
  }
#else
  if (nThreads != 1) {
    fprintf(stderr,"Number of threads must be 1 (serial version)\n");
    exit(1);
  }
#endif

	factors = getFactors();
	initSwaption(factors);

  // Calling the Swaption Pricing Routine
#ifdef ENABLE_THREADS
  int threadIDs[nThreads];
  for (i = 0; i < nThreads; i++) {
    threadIDs[i] = i;
    pthread_create(&threads[i], &pthread_custom_attr, worker, &threadIDs[i]);
  }
  for (i = 0; i < nThreads; i++) {
    pthread_join(threads[i], NULL);
  }

  free(threads);
#else
  int threadID = 0;
  worker(&threadID);
#endif //ENABLE_THREADS

  for (i = 0; i < nSwaptions; i++) {
    fprintf(stderr,"Swaption%d: [SwaptionPrice: %.10lf StdError: %.10lf] \n", 
        i, swaptions[i].dSimSwaptionMeanPrice, swaptions[i].dSimSwaptionStdError);
  }

  for (i = 0; i < nSwaptions; i++) {
    free_dvector(swaptions[i].pdYield);
    free_dmatrix(swaptions[i].ppdFactors);
  }

  free(swaptions);

  return 0;
}
