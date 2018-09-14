#ifndef _ORSA_INPUT_OUTPUT_MPC_ASTEROID_
#define _ORSA_INPUT_OUTPUT_MPC_ASTEROID_

#include <orsaInputOutput/file.h>

#include <orsaSolarSystem/orbit.h>

#include <vector>
#include <string>

namespace orsaInputOutput {
  
    class MPCAsteroidDataElement {
    public:
        orsa::Cache<orsaSolarSystem::OrbitWithEpoch> orbit;
        orsa::Cache<double> H;
        orsa::Cache<double> G;
        orsa::Cache<unsigned int> number;
        orsa::Cache<std::string> designation;
        orsa::Cache<int> U; // orbit uncertainty parameter http://www.minorplanetcenter.net/iau/info/UValue.html
    };
  
    typedef std::vector<MPCAsteroidDataElement> MPCAsteroidData;
  
    class MPCAsteroidFile : 
        public orsaInputOutput::InputFile <
        orsaInputOutput::CompressedFile,
        orsaInputOutput::MPCAsteroidData
        > {
    
    public:
        MPCAsteroidFile() : 
            orsaInputOutput::InputFile <
        orsaInputOutput::CompressedFile,
        orsaInputOutput::MPCAsteroidData
        > () { }
      
    protected:
        ~MPCAsteroidFile() { }
    
    public:
        bool goodLine(const char * line);
    
    public:
        bool processLine(const char * line);
        
    public:
        // TODO: use lists of objects to read, and exit as soon as all requested object are read
        orsa::Cache<unsigned int> select_number;
        orsa::Cache<std::string>  select_designation;
        orsa::Cache<bool>         select_NEO;
        orsa::Cache<int>          select_max_U;
    public:
        static const double NEO_max_q;
    };
    
}; // orsaInputOutput

#endif // _ORSA_INPUT_OUTPUT_MPC_ASTEROID_
