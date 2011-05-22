/***********************************************************************

 HiSIM (Hiroshima University STARC IGFET Model)
 Copyright (C) 2011 Hiroshima University & STARC

 VERSION : HiSIM_2.5.1 
 FILE : hsm2noi.c

 date : 2011.04.07

 released by 
                Hiroshima University &
                Semiconductor Technology Academic Research Center (STARC)
***********************************************************************/

#include "ngspice.h"
#include <stdio.h>
#include <math.h>
#include "hsm2def.h"
#include "cktdefs.h"
#include "iferrmsg.h"
#include "noisedef.h"
#include "suffix.h"
#include "const.h"  /* jwan */

/*
 * HSM2noise (mode, operation, firstModel, ckt, data, OnDens)
 *    This routine names and evaluates all of the noise sources
 *    associated with MOSFET's.  It starts with the model *firstModel and
 *    traverses all of its insts.  It then proceeds to any other models
 *    on the linked list.  The total output noise density generated by
 *    all of the MOSFET's is summed with the variable "OnDens".
 */

extern void   NevalSrc();
extern double Nintegrate();

int HSM2noise (mode, operation, inModel, ckt, data, OnDens)
     int mode, operation;
     GENmodel *inModel;
     CKTcircuit *ckt;
     register Ndata *data;
     double *OnDens;
{
  register HSM2model *model = (HSM2model *)inModel;
  register HSM2instance *here;
  char name[N_MXVLNTH];
  double tempOnoise;
  double tempInoise;
  double noizDens[HSM2NSRCS];
  double lnNdens[HSM2NSRCS];
  register int i;
  double R = 0.0 , G = 0.0 ;
  double TTEMP = 0.0 ;

  /* define the names of the noise sources */
  static char * HSM2nNames[HSM2NSRCS] = {
    /* Note that we have to keep the order
       consistent with the index definitions 
       in hsm2defs.h */
    ".rd",              /* noise due to rd */
    ".rs",              /* noise due to rs */
    ".id",              /* noise due to id */
    ".1ovf",            /* flicker (1/f) noise */
    ".igs",             /* shot noise due to Igs */
    ".igd",             /* shot noise due to Igd */
    ".igb",             /* shot noise due to Igb */
    ".ign",             /* induced gate noise component at the drain node */
    ""                  /* total transistor noise */
  };
  
  for ( ;model != NULL; model = model->HSM2nextModel ) {
    for ( here = model->HSM2instances; here != NULL;
	  here = here->HSM2nextInstance ) {
      switch (operation) {
      case N_OPEN:
	/* see if we have to to produce a summary report */
	/* if so, name all the noise generators */
	  
	if (((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0) {
	  switch (mode) {
	  case N_DENS:
	    for ( i = 0; i < HSM2NSRCS; i++ ) { 
	      (void) sprintf(name, "onoise.%s%s", 
			     (char *)here->HSM2name, HSM2nNames[i]);
	      data->namelist = 
		(IFuid *) trealloc((char *) data->namelist,
				   (data->numPlots + 1) * sizeof(IFuid));
	      if (!data->namelist)
		return(E_NOMEM);
	      (*(SPfrontEnd->IFnewUid)) 
		(ckt, &(data->namelist[data->numPlots++]),
		 (IFuid) NULL, name, UID_OTHER, (GENERIC **) NULL);
	    }
	    break;
	  case INT_NOIZ:
	    for ( i = 0; i < HSM2NSRCS; i++ ) {
	      (void) sprintf(name, "onoise_total.%s%s", 
			     (char *)here->HSM2name, HSM2nNames[i]);
	      data->namelist = 
		(IFuid *) trealloc((char *) data->namelist,
				   (data->numPlots + 1) * sizeof(IFuid));
	      if (!data->namelist)
		return(E_NOMEM);
	      (*(SPfrontEnd->IFnewUid)) 
		(ckt, &(data->namelist[data->numPlots++]),
		 (IFuid) NULL, name, UID_OTHER, (GENERIC **) NULL);
	      
	      (void) sprintf(name, "inoise_total.%s%s", 
			     (char *)here->HSM2name, HSM2nNames[i]);
	      data->namelist = 
		(IFuid *) trealloc((char *) data->namelist,
				   (data->numPlots + 1) * sizeof(IFuid));
	      if (!data->namelist)
		return(E_NOMEM);
	      (*(SPfrontEnd->IFnewUid)) 
		(ckt, &(data->namelist[data->numPlots++]),
		 (IFuid) NULL, name, UID_OTHER, (GENERIC **)NULL);
	    }
	    break;
	  }
	}
	break;
      case N_CALC:
	switch (mode) {
	case N_DENS:

	  /* temperature */
	  TTEMP = ckt->CKTtemp ;
	  if ( here->HSM2_temp_Given ) TTEMP = here->HSM2_temp ;
	  if ( here->HSM2_dtemp_Given ) {
	    TTEMP = TTEMP + here->HSM2_dtemp ;
	  }

         /* rs/rd thermal noise */
         if ( model->HSM2_corsrd < 0 ) {
           NevalSrc(&noizDens[HSM2RDNOIZ], (double*) NULL,
                    ckt, N_GAIN,
                    here->HSM2dNodePrime, here->HSM2dNode,
                    (double) 0.0);
           noizDens[HSM2RDNOIZ] *= 4 * CONSTboltz * TTEMP * here->HSM2drainConductance ;
	   lnNdens[HSM2RDNOIZ] = log( MAX(noizDens[HSM2RDNOIZ],N_MINLOG) ) ;

           NevalSrc(&noizDens[HSM2RSNOIZ], (double*) NULL,
                    ckt, N_GAIN,
                    here->HSM2sNodePrime, here->HSM2sNode,
                    (double) 0.0);
           noizDens[HSM2RSNOIZ] *= 4 * CONSTboltz * TTEMP * here->HSM2sourceConductance ;
	   lnNdens[HSM2RSNOIZ] = log( MAX(noizDens[HSM2RSNOIZ],N_MINLOG) ) ;

           /*
           NevalSrc(&noizDens[HSM2RDNOIZ], &lnNdens[HSM2RDNOIZ],
		     ckt, THERMNOISE,
		     here->HSM2dNodePrime, here->HSM2dNode,
		     here->HSM2_weff / model->HSM2_rsh);
	    
	    NevalSrc(&noizDens[HSM2RSNOIZ], &lnNdens[HSM2RSNOIZ], 
		     ckt, THERMNOISE,
		     here->HSM2sNodePrime, here->HSM2sNode,
		     here->HSM2_weff / model->HSM2_rsh);
	    */
	  } else {
	    noizDens[HSM2RDNOIZ] = 0e0 ;
	    lnNdens[HSM2RDNOIZ] = N_MINLOG ;
	    noizDens[HSM2RSNOIZ] = 0e0 ;
	    lnNdens[HSM2RSNOIZ] = N_MINLOG ;
	  }

	  /* channel thermal noise */
	  switch( model->HSM2_noise ) {
	  case 1:
           /* HiSIM2 model */
         if ( model->HSM2_corsrd <= 0 || here->HSM2internalGd <= 0.0 ) {
           G = here->HSM2_noithrml ;
         } else {
           R = 0.0 , G = 0.0 ;
           if ( here->HSM2_noithrml > 0.0 ) R += 1.0 / here->HSM2_noithrml ;
           if ( here->HSM2internalGd > 0.0 ) R += 1.0 / here->HSM2internalGd ;
           if ( here->HSM2internalGs > 0.0 ) R += 1.0 / here->HSM2internalGs ;
           if ( R > 0.0 ) G = 1.0 / R ;
         }
           NevalSrc(&noizDens[HSM2IDNOIZ], (double*) NULL,
                    ckt, N_GAIN,
                    here->HSM2dNodePrime, here->HSM2sNodePrime,
                    (double) 0.0);
           noizDens[HSM2IDNOIZ] *= 4 * CONSTboltz * TTEMP * G ;
           lnNdens[HSM2IDNOIZ] = log( MAX(noizDens[HSM2IDNOIZ],N_MINLOG) );
           break;
         }

         /* flicker noise */
         NevalSrc(&noizDens[HSM2FLNOIZ], (double*) NULL,
                  ckt, N_GAIN,
                  here->HSM2dNodePrime, here->HSM2sNodePrime,
                  (double) 0.0);
         switch ( model->HSM2_noise ) {
         case 1:
           /* HiSIM model */
           noizDens[HSM2FLNOIZ] *= here->HSM2_noiflick / pow(data->freq, model->HSM2_falph) ;
           break;
         }
         lnNdens[HSM2FLNOIZ] = log(MAX(noizDens[HSM2FLNOIZ], N_MINLOG));

         /* shot noise */
	 NevalSrc(&noizDens[HSM2IGSNOIZ],
		  &lnNdens[HSM2IGSNOIZ], ckt, SHOTNOISE,
		  here->HSM2gNodePrime, here->HSM2sNodePrime,
		  here->HSM2_igs);
	 NevalSrc(&noizDens[HSM2IGDNOIZ],
		  &lnNdens[HSM2IGDNOIZ], ckt, SHOTNOISE,
		  here->HSM2gNodePrime, here->HSM2dNodePrime,
		  here->HSM2_igd);
	 NevalSrc(&noizDens[HSM2IGBNOIZ],
		  &lnNdens[HSM2IGBNOIZ], ckt, SHOTNOISE,
		  here->HSM2gNodePrime, here->HSM2bNodePrime,
		  here->HSM2_igb);

	  /* induced gate noise */
	  switch ( model->HSM2_noise ) {
	  case 1:
	    /* HiSIM model */
	    NevalSrc(&noizDens[HSM2IGNOIZ], (double*) NULL,
		     ckt, N_GAIN, 
		     here->HSM2dNodePrime, here->HSM2sNodePrime, 
		     (double) 0.0);
	    noizDens[HSM2IGNOIZ] *= here->HSM2_noiigate * here->HSM2_noicross * here->HSM2_noicross * data->freq * data->freq;
	    lnNdens[HSM2IGNOIZ] = log(MAX(noizDens[HSM2IGNOIZ], N_MINLOG));
	    break;
	  }

	  /* total */
	  noizDens[HSM2TOTNOIZ] = noizDens[HSM2RDNOIZ] + noizDens[HSM2RSNOIZ]
	    + noizDens[HSM2IDNOIZ] + noizDens[HSM2FLNOIZ]
	    + noizDens[HSM2IGSNOIZ] + noizDens[HSM2IGDNOIZ] + noizDens[HSM2IGBNOIZ]
	    + noizDens[HSM2IGNOIZ];
	  lnNdens[HSM2TOTNOIZ] = log(MAX(noizDens[HSM2TOTNOIZ], N_MINLOG));
/*       printf("f %e Sid %.16e Srd %.16e Srs %.16e Sflick %.16e Stherm %.16e Sign %.16e\n", */
/*              data->freq,noizDens[HSM2TOTNOIZ],noizDens[HSM2RDNOIZ],noizDens[HSM2RSNOIZ],noizDens[HSM2FLNOIZ],noizDens[HSM2IDNOIZ],noizDens[HSM2IGNOIZ]); */
	  
	  *OnDens += noizDens[HSM2TOTNOIZ];
	  
	  if ( data->delFreq == 0.0 ) {
	    /* if we haven't done any previous 
	       integration, we need to initialize our
	       "history" variables.
	    */
	    
	    for ( i = 0; i < HSM2NSRCS; i++ ) 
	      here->HSM2nVar[LNLSTDENS][i] = lnNdens[i];
	    
	    /* clear out our integration variables
	       if it's the first pass
	    */
	    if (data->freq == ((NOISEAN*) ckt->CKTcurJob)->NstartFreq) {
	      for (i = 0; i < HSM2NSRCS; i++) {
		here->HSM2nVar[OUTNOIZ][i] = 0.0;
		here->HSM2nVar[INNOIZ][i] = 0.0;
	      }
	    }
	  }
	  else {
	    /* data->delFreq != 0.0,
	       we have to integrate.
	    */
	    for ( i = 0; i < HSM2NSRCS; i++ ) {
	      if ( i != HSM2TOTNOIZ ) {
		tempOnoise = 
		  Nintegrate(noizDens[i], lnNdens[i],
			     here->HSM2nVar[LNLSTDENS][i], data);
		tempInoise = 
		  Nintegrate(noizDens[i] * data->GainSqInv, 
			     lnNdens[i] + data->lnGainInv,
			     here->HSM2nVar[LNLSTDENS][i] + data->lnGainInv,
			     data);
		here->HSM2nVar[LNLSTDENS][i] = lnNdens[i];
		data->outNoiz += tempOnoise;
		data->inNoise += tempInoise;
		if ( ((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0 ) {
		  here->HSM2nVar[OUTNOIZ][i] += tempOnoise;
		  here->HSM2nVar[OUTNOIZ][HSM2TOTNOIZ] += tempOnoise;
		  here->HSM2nVar[INNOIZ][i] += tempInoise;
		  here->HSM2nVar[INNOIZ][HSM2TOTNOIZ] += tempInoise;
		}
	      }
	    }
	  }
	  if ( data->prtSummary ) {
	    for (i = 0; i < HSM2NSRCS; i++) {
	      /* print a summary report */
	      data->outpVector[data->outNumber++] = noizDens[i];
	    }
	  }
	  break;
	case INT_NOIZ:
	  /* already calculated, just output */
	  if ( ((NOISEAN*)ckt->CKTcurJob)->NStpsSm != 0 ) {
	    for ( i = 0; i < HSM2NSRCS; i++ ) {
	      data->outpVector[data->outNumber++] = here->HSM2nVar[OUTNOIZ][i];
	      data->outpVector[data->outNumber++] = here->HSM2nVar[INNOIZ][i];
	    }
	  }
	  break;
	}
	break;
      case N_CLOSE:
	/* do nothing, the main calling routine will close */
	return (OK);
	break;   /* the plots */
      }       /* switch (operation) */
    }    /* for here */
  }    /* for model */
  
  return(OK);
}



