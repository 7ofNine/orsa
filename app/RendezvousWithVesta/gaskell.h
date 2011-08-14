#ifndef _GASKELL_H_
#define _GASKELL_H_

#include <orsa/shape.h>
#include <orsa/unit.h>
#include <orsa/debug.h>

#include <string>

class GaskellPlateModel : public orsa::TriShape {
public:
    GaskellPlateModel() : TriShape() {
        
    }
public:
    bool read(const std::string & filename) {
        FILE * fp_model = fopen(filename.c_str(),"r");
        if (!fp_model) {
            ORSA_ERROR("can't open file %s",filename.c_str());
            return false;
        }
        //
        _vertex.clear();
        _face.clear();
        //
        char line[1024];
        size_t nv;
        if (fgets(line,1024,fp_model) != 0) {
            sscanf(line,"%zi",&nv);
        }
        // _vertex.resize(nv);
        size_t id;
        double x,y,z;
        size_t count_v=0;
        while (fgets(line,1024,fp_model) != 0) {
            ++count_v;
            if (count_v <= nv) {
                if (4 == sscanf(line,"%zi %lf %lf %lf",&id,&x,&y,&z)) {
                    _vertex.push_back(orsa::Vector(FromUnits(x,Unit::KM),
                                                   FromUnits(y,Unit::KM),
                                                   FromUnits(z,Unit::KM)));
                }
            }
            if (count_v == nv) break;
        }
        size_t nt;
        if (fgets(line,1024,fp_model) != 0) {
            sscanf(line,"%zi",&nt);
        }
        // _face.resize(nt);
        size_t i,j,k;
        while (fgets(line,1024,fp_model) != 0)  {
            if (4 == sscanf(line,"%zi %zi %zi %zi",&id,&i,&j,&k)) {
                // this is needed sometimes...
                // a flag fould be useful
                --i; --j; --k; // 1,2,3... to 0,1,2...
                if ((i != j) && (j != k)) {
                    _face.push_back(TriIndex(i,j,k));
                } else {
                    ORSA_ERROR("repeated indexes...");
                    return false;
                }
            } 
        }
        // post-read check: vertex numbering starting from 0?
        {
            bool start_from_zero = false;
            unsigned int q=0;
            while (q<_face.size()) {
                const TriIndex t = _face[q];
                if ((t.i() == 0) || (t.j() == 0) || (t.k() == 0)) {
                    start_from_zero = true;
                    break;
                }
                ++q;
            }
            if (!start_from_zero) {
                for (unsigned int p=0;p<_face.size();++p) {
                    const TriIndex t = _face[p];
                    _face[p] = TriIndex(t.i()-1,t.j()-1,t.k()-1);
                }
            }
        }
        fclose(fp_model);
    
        return true;
    }	
};

#endif // _GASKELL_H_
