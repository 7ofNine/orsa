#include <stdlib.h>

#include <cstdio>
#include <iostream>
#include <string>

#include <gmpxx.h>
#include <cmath>

#include "grain.h"

#include <gsl/gsl_rng.h>

using namespace std;

int main (int argc, char ** argv) {
    
    if (argc != 11) {
        // only part of the parameters are arguments
        cout << "Usage: " << argv[0] << " baseName samplesPerBin a_AU_min a_AU_max e_min e_max i_DEG_min i_DEG_max H_min H_max" << endl;
        exit(0);
    }

    // use pid as seed for rng
    const int pid = getpid();
    cout << "process ID: " << pid << endl;
    gsl_rng * rnd = gsl_rng_alloc(gsl_rng_gfsr4);
    gsl_rng_set(rnd,pid);
    
    
    const std::string baseName = argv[1];
    const unsigned int samplesPerBin = atoi(argv[2]);
    const int z_a_min = lrint(atof(argv[3])/grain_a_AU);
    const int z_a_max = lrint(atof(argv[4])/grain_a_AU);
    const int z_e_min = lrint(atof(argv[5])/grain_e);
    const int z_e_max = lrint(atof(argv[6])/grain_e);
    const int z_i_min = lrint(atof(argv[7])/grain_i_DEG);
    const int z_i_max = lrint(atof(argv[8])/grain_i_DEG);
    const int z_H_min = lrint(atof(argv[9])/grain_H);
    const int z_H_max = lrint(atof(argv[10])/grain_H);
    
    const int z_a_delta = lrint(0.05/grain_a_AU);
    const int z_e_delta = lrint(0.05/grain_e);
    const int z_i_delta = lrint(5.00/grain_i_DEG);
    const int z_H_delta = lrint(1.00/grain_H);
    
    // the other angles all go from 0 to 360 deg in steps of 30 deg, 12 iter factor each
    
    // other args, constant for now
    const double JD = 2455198.0; // epoch of orbits
    
    mpz_class numJobs = 1;
    numJobs *= (z_a_max-z_a_min)/z_a_delta;
    numJobs *= (z_e_max-z_e_min)/z_e_delta;
    numJobs *= (z_i_max-z_i_min)/z_i_delta;
    cout << "maximum expected iterations: " << numJobs << endl;
    
    // should check directly for bins with 0 NEOs
    
    if (numJobs < 1) {
        cout << "numJobs must be positive" << endl;
        exit(0);
    }
    
    if (numJobs > 9999) {
        cout << "numJobs is too big, are you sure of what you're doing?" << endl;
        exit(0);
    }
    
    gmp_printf("Submission of %Zi job(s)\n"
               "proceed? [y/n] ",
               numJobs.get_mpz_t());
    
    const std::string yes = "y";
    
    std::string yesno;
    
    cin >> yesno;
    
    // cout << "yesno: " << yesno << endl;
    
    if (yesno == yes) {
        
        {
            int waitSec = 5;
            while (waitSec > 0) {
                cout << "Starting in " << waitSec << " seconds..." << endl;
                sleep(1);
                --waitSec;
            }
        }
        
        char cmd[1024];
        FILE * fp;
        for (int z_a=z_a_min; z_a<z_a_max; z_a+=z_a_delta) {
            for (int z_e=z_e_min; z_e<z_e_max; z_e+=z_e_delta) {
                for (int z_i=z_i_min; z_i<z_i_max; z_i+=z_i_delta) {

                    fp = fopen("grid.dat","w");
                    fprintf(fp,"%.2f %i   %i %i %i   %i %i %i   %i %i %i   %i %i %i   %i %i %i   %i %i %i   %i %i %i\n",
                            JD,
                            samplesPerBin,
                            z_a,z_a+z_a_delta,z_a_delta,
                            z_e,z_e+z_e_delta,z_e_delta,
                            z_i,z_i+z_i_delta,z_i_delta,
                            0,360000,30000,
                            0,360000,30000,
                            0,360000,30000,
                            z_H_min,z_H_max,z_H_delta); // all range in H
                    fclose(fp);
                    //
                    fp = fopen("randomSeed.dat","w");
                    fprintf(fp,"%i\n",gsl_rng_uniform_int(rnd,1000000000));
                    fclose(fp);
                    //
                    snprintf(cmd,1024,"./SurveyReviewWorkGenerator %s_%i_%i-%i_%i-%i_%i-%i_%i-%i",
                             baseName.c_str(),
                             samplesPerBin,
                             z_a,z_a+z_a_delta,
                             z_e,z_e+z_e_delta,
                             z_i,z_i+z_i_delta,
                             z_H_min,z_H_max);
                    cout << "executing: [" << cmd << "]" << endl;
                    system(cmd);
                }
            }
        }
    }
    
    gsl_rng_free(rnd); 
    
    return 0;
}