#include "grain.h"

#include <orsa/debug.h>
#include <orsa/print.h>
#include <orsa/statistic.h>

#include <orsaSolarSystem/datetime.h>
#include <orsaSolarSystem/print.h>

// SQLite3
#include "sqlite3.h"

int main(int argc, char ** argv) {
    
    if (argc < 3) {
        ORSA_DEBUG("Usage: %s <sqlite-merged-db> <sqlite-result-db(s)>",argv[0]);
        exit(0);
    }
    
    orsa::Debug::instance()->initTimer();
    
    ORSA_DEBUG("process ID: %i",getpid());
    
    // needed to work with SQLite database
    sqlite3     * db;
    char        * zErr;
    int           rc;
    std::string   sql;
    
    {
        // open database
        rc = sqlite3_open(argv[1],&db);
        //
        if (rc) {
            fprintf(stderr,"Can't open db: %s\n",sqlite3_errmsg(db));
            sqlite3_close(db);
            exit(0);
        }
    }
    
    {
        // get list of tables, to see if some results have been obtained already
        // if no table is present, create it
        char **result;
        int nrows, ncols;
        sql = "SELECT name FROM sqlite_master";
        rc = sqlite3_get_table(db,sql.c_str(),&result,&nrows,&ncols,&zErr);
        //
        if (rc != SQLITE_OK) {
            if (zErr != NULL) {
                fprintf(stderr,"SQL error: %s\n",zErr);
                sqlite3_free(zErr);
                exit(0); 
            }
        }
        // ORSA_DEBUG("nrows: %i  ncols: %i",nrows, ncols);
        //
        /* for (int i=0; i<nrows; ++i) {
           for (int j=0; j<ncols; ++j) {
           // i=0 is the header
           const int index = (i+1)*ncols+j;
           ORSA_DEBUG("result[%i] = %s",index, result[index]);
           }
           }
        */
        //
        bool createTable=true;
        //
        if (nrows==1) {
            createTable=false;
        }
        //
        sqlite3_free_table(result);
        
        if (createTable) {
            // create results table
            sql = "CREATE TABLE grid(z_a_min INTEGER, z_a_max INTEGER, z_e_min INTEGER, z_e_max INTEGER, z_i_min INTEGER, z_i_max INTEGER, z_node_min INTEGER, z_node_max INTEGER, z_peri_min INTEGER, z_peri_max INTEGER, z_M_min INTEGER, z_M_max INTEGER, z_H INTEGER, N_NEO INTEGER, N_PHO INTEGER, NEO_in_field INTEGER, PHO_in_field INTEGER, eta_NEO REAL, sigma_eta_NEO REAL, eta_PHO REAL, sigma_eta_PHO REAL)";
            rc = sqlite3_exec(db,sql.c_str(),NULL,NULL,&zErr);
            //
            if (rc != SQLITE_OK) {
                if (zErr != NULL) {
                    fprintf(stderr,"SQL error: %s\n",zErr);
                    sqlite3_free(zErr);
                    exit(0); 
                }
            }
        }
    }
    
    {
        // integrity_check
        
        int nrows, ncols;
        char * * sql_result;
        rc = sqlite3_get_table(db,"pragma integrity_check",&sql_result,&nrows,&ncols,&zErr);
        //
        if (rc != SQLITE_OK) {
            if (zErr != NULL) {
                ORSA_DEBUG("SQL error: %s\n",zErr); 
                sqlite3_free(zErr);
                sqlite3_close(db);
                exit(0);
            }
        }
        //
        bool integrity_check_passed=false;
        if (nrows==1) {
            if (ncols==1) {
                if (sql_result[1]==std::string("ok")) {
                    integrity_check_passed=true;
                }
            }
        }
        if (!integrity_check_passed) {
            ORSA_DEBUG("SQLite problem: integrity_check failed\n"); 
            sqlite3_close(db);
            exit(0); 
        }    
    }
    
    sqlite3 * dbf;
    for (int fileID=2; fileID<argc; ++fileID) {
        
        ORSA_DEBUG("merging db file [%s]",argv[fileID]);
        
        // open dbf
        // rc = sqlite3_open(argv[1],&dbf);
        rc = sqlite3_open_v2(argv[fileID],&dbf,SQLITE_OPEN_READONLY,NULL);
        if (rc) {
            ORSA_DEBUG("Can't open db: %s\n",sqlite3_errmsg(dbf));
            sqlite3_close(dbf);
            continue;
        }
        
        {
            // integrity_check
            
            int nrows, ncols;
            char * * sql_result;
            rc = sqlite3_get_table(dbf,"pragma integrity_check",&sql_result,&nrows,&ncols,&zErr);
            //
            if (rc != SQLITE_OK) {
                if (zErr != NULL) {
                    ORSA_DEBUG("SQL error: %s\n",zErr); 
                    sqlite3_free(zErr);
                    sqlite3_close(dbf);
                    continue;
                }
            }
            //
            bool integrity_check_passed=false;
            if (nrows==1) {
                if (ncols==1) {
                    if (sql_result[1]==std::string("ok")) {
                        integrity_check_passed=true;
                    }
                }
            }
            if (!integrity_check_passed) {
                ORSA_DEBUG("SQLite problem: integrity_check failed\n"); 
                sqlite3_close(dbf);
                continue; 
            }    
        }
        
        {
            
            char * * sql_result_dbf;
            int nrows_dbf, ncols_dbf;
            char sql_line[1024];
            sprintf(sql_line,"SELECT * FROM grid where eta_PHO>0");
            rc = sqlite3_get_table(dbf,sql_line,&sql_result_dbf,&nrows_dbf,&ncols_dbf,&zErr);
            if (rc != SQLITE_OK) {
                if (zErr != NULL) {
                    ORSA_DEBUG("SQL error: %s\n",zErr);
                    sqlite3_free(zErr);
                    sqlite3_close(dbf);
                    continue;
                }
            }
            
            for (int row=1; row<=nrows_dbf; ++row) {
                const int z_a_min          = atoi(sql_result_dbf[row*ncols_dbf+0]);
                const int z_a_max          = atoi(sql_result_dbf[row*ncols_dbf+1]);
                const int z_e_min          = atoi(sql_result_dbf[row*ncols_dbf+2]);
                const int z_e_max          = atoi(sql_result_dbf[row*ncols_dbf+3]);
                const int z_i_min          = atoi(sql_result_dbf[row*ncols_dbf+4]);
                const int z_i_max          = atoi(sql_result_dbf[row*ncols_dbf+5]);
                const int z_node_min       = atoi(sql_result_dbf[row*ncols_dbf+6]);
                const int z_node_max       = atoi(sql_result_dbf[row*ncols_dbf+7]);
                const int z_peri_min       = atoi(sql_result_dbf[row*ncols_dbf+8]);
                const int z_peri_max       = atoi(sql_result_dbf[row*ncols_dbf+9]);
                const int z_M_min          = atoi(sql_result_dbf[row*ncols_dbf+10]);
                const int z_M_max          = atoi(sql_result_dbf[row*ncols_dbf+11]);
                const int z_H              = atoi(sql_result_dbf[row*ncols_dbf+12]);
                const int N_NEO            = atoi(sql_result_dbf[row*ncols_dbf+13]);
                const int N_PHO            = atoi(sql_result_dbf[row*ncols_dbf+14]);
                const int NEO_in_field     = atoi(sql_result_dbf[row*ncols_dbf+15]);
                const int PHO_in_field     = atoi(sql_result_dbf[row*ncols_dbf+16]);
                const double eta_NEO       = atof(sql_result_dbf[row*ncols_dbf+17]);
                const double sigma_eta_NEO = atof(sql_result_dbf[row*ncols_dbf+18]);
                const double eta_PHO       = atof(sql_result_dbf[row*ncols_dbf+19]);
                const double sigma_eta_PHO = atof(sql_result_dbf[row*ncols_dbf+20]);
                
                char * * sql_result_db;
                int nrows_db, ncols_db;
                sprintf(sql_line,"SELECT * FROM grid WHERE z_a_min=%i and z_a_max=%i and z_e_min=%i and z_e_max=%i and z_i_min=%i and z_i_max=%i and z_node_min=%i and z_node_max=%i and z_peri_min=%i and z_peri_max=%i and z_M_min=%i and z_M_max=%i and z_H=%i",
                        z_a_min,z_a_max,
                        z_e_min,z_e_max,
                        z_i_min,z_i_max,
                        z_node_min,z_node_max,
                        z_peri_min,z_peri_max,
                        z_M_min,z_M_max,
                        z_H);
                rc = sqlite3_get_table(db,sql_line,&sql_result_db,&nrows_db,&ncols_db,&zErr);
                if (rc != SQLITE_OK) {
                    if (zErr != NULL) {
                        ORSA_DEBUG("SQL error: %s\n",zErr);
                        sqlite3_free(zErr);
                        sqlite3_close(db);
                        exit(0);
                    }
                }
                if (nrows_db==0) {
                    // insert
                    sprintf(sql_line,
                            "INSERT INTO grid VALUES(%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%g,%g,%g,%g)",
                            z_a_min,z_a_max,
                            z_e_min,z_e_max,
                            z_i_min,z_i_max,
                            z_node_min,z_node_max,
                            z_peri_min,z_peri_max,
                            z_M_min,z_M_max,
                            z_H,
                            N_NEO,
                            N_PHO,
                            NEO_in_field,
                            PHO_in_field,
                            eta_NEO,
                            sigma_eta_NEO,
                            eta_PHO,
                            sigma_eta_PHO);  
                    rc = sqlite3_exec(db,sql_line,NULL,NULL,&zErr);
                    if (rc != SQLITE_OK) {
                        if (zErr != NULL) {
                            ORSA_DEBUG("SQL error: %s\n",zErr);
                            sqlite3_free(zErr);
                            sqlite3_close(db);
                            exit(0);
                        }
                    }
                } else if (nrows_db==1) {
                    // update
                    /* const int N_NEO_db            = atoi(sql_result_db[ncols_db+13]);
                       const int N_PHO_db            = atoi(sql_result_db[ncols_db+14]);
                       const int NEO_in_field_db     = atoi(sql_result_db[ncols_db+15]);
                       const int PHO_in_field_db     = atoi(sql_result_db[ncols_db+16]);
                    */
                    const double eta_NEO_db       = atof(sql_result_db[ncols_db+17]);
                    const double sigma_eta_NEO_db = atof(sql_result_db[ncols_db+18]);
                    const double eta_PHO_db       = atof(sql_result_db[ncols_db+19]);
                    const double sigma_eta_PHO_db = atof(sql_result_db[ncols_db+20]);
                    //
                    const double       new_eta_NEO = 1.0 - (1.0-eta_NEO_db)*(1.0-eta_NEO);
                    const double new_sigma_eta_NEO = sqrt(orsa::square((1.0-eta_NEO)*sigma_eta_NEO_db)+
                                                          orsa::square((1.0-eta_NEO_db)*sigma_eta_NEO));
                    const double       new_eta_PHO = 1.0 - (1.0-eta_PHO_db)*(1.0-eta_PHO);
                    const double new_sigma_eta_PHO = sqrt(orsa::square((1.0-eta_PHO)*sigma_eta_PHO_db)+
                                                          orsa::square((1.0-eta_PHO_db)*sigma_eta_PHO));

                    sprintf(sql_line,"UPDATE grid SET eta_NEO=%g,sigma_eta_NEO=%g,eta_PHO=%g,sigma_eta_PHO=%g WHERE z_a_min=%i and z_a_max=%i and z_e_min=%i and z_e_max=%i and z_i_min=%i and z_i_max=%i and z_node_min=%i and z_node_max=%i and z_peri_min=%i and z_peri_max=%i and z_M_min=%i and z_M_max=%i and z_H=%i",
                            new_eta_NEO,
                            new_sigma_eta_NEO,
                            new_eta_PHO,
                            new_sigma_eta_PHO,
                            z_a_min,z_a_max,
                            z_e_min,z_e_max,
                            z_i_min,z_i_max,
                            z_node_min,z_node_max,
                            z_peri_min,z_peri_max,
                            z_M_min,z_M_max,
                            z_H);
                    rc = sqlite3_get_table(db,sql_line,&sql_result_db,&nrows_db,&ncols_db,&zErr);
                    if (rc != SQLITE_OK) {
                        if (zErr != NULL) {
                            ORSA_DEBUG("SQL error: %s\n",zErr);
                            sqlite3_free(zErr);
                            sqlite3_close(db);
                            exit(0);
                        }
                    }
                } else if (nrows_db>1) {
                    ORSA_DEBUG("SQLite problem: selected %i rows instead of 1\n",nrows_db); 
                    sqlite3_close(db);
                    exit(0); 
                } 
            }
            
            sqlite3_free_table(sql_result_dbf);
        }
    
        // close dbf
        sqlite3_close(dbf);
    
    } // loop on files
    
    sqlite3_close(db);
    
    return 0;
}

