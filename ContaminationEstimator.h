/* The MIT License

   Copyright (c) 2009 Genome Research Ltd (GRL), 2010 Broad Institute

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

/* Contact: Fan Zhang <fanzhang@umich.edu> */
#ifndef CONTAMINATIONESTIMATOR_H_
#define CONTAMINATIONESTIMATOR_H_

#include <string>
#include <unordered_map>
//#include <tkDecls.h>
#include "MathVector.h"
#include "MathGenMin.h"
#include "SimplePileupViewer.h"
#include <limits>
#ifdef _OPENMP
#include "omp.h"
#endif

class ContaminationEstimator {
public:
    bool isPCFixed;
    bool isAlphaFixed;
    bool isAFknown;
    bool isHeter;
    bool verbose;
    int numPC;
    int numThread;
    int seed;
    double epsilon;
#define PCtype double

//#define PHRED(x)    pow(10.0,x/-10.0)
    static double Phred(double x) {
        return pow(10.0, x / -10.0);
    }

    class fullLLKFunc : public VectorFunc {
    public:
        double min_af;
        double max_af;
        double llk;
        ContaminationEstimator *ptr;
        std::vector<double> fixPC;
        std::vector<double> fixPC2;
        double fixAlpha;
        std::vector<double> globalPC;//best result holder
        std::vector<double> globalPC2;//best result holder
        double globalAlpha;//best result holder
        const char* Base;
        fullLLKFunc()
        {
            fullLLKFunc::Base = "actg";
            min_af=0.00005;
            max_af=0.99995;
            llk=0;
            ptr=nullptr;
            fixAlpha=0;
            std::cerr<<"Initializae from fullLLKFunc()"<<std::endl;

        }
        fullLLKFunc(int dim, ContaminationEstimator* contPtr):fixPC(dim,0.),fixPC2(dim,0.),globalPC(fixPC),globalPC2(fixPC2) {
            fullLLKFunc::Base = "actg";
            min_af = 0.00005;
            max_af = 0.99995;
            llk = 0.;
            ptr = contPtr;
            fixAlpha = 0.;
            globalAlpha =0.;
            std::cerr<<"Initialize from fullLLKFunc(int dim, ContaminationEstimator* contPtr)"<<std::endl;
        }

        ~fullLLKFunc() { };

        inline static double invLogit(double &x) {
            double e = exp(x);
            return e / (1. + e);
        };
        inline static double Logit(double &x) {

            return log(x / (1. - x));
        };
/*
        inline double computeMixLLKs(double tPC1, double tPC2) {
            double min_af(0.5 / ptr->NumIndividual), max_af((ptr->NumIndividual - 0.5) / ptr->NumIndividual);
            double sumLLK(0), GF0(0), GF1(0), GF2(0);
            std::string chr;
            uint32_t pos;
            size_t glIndex = 0;
            for (size_t i = 0; i != ptr->NumMarker; ++i) {
                //std::cerr << "Number " << i << "th marker out of " << ptr->NumMarker << " markers and " << ptr->NumIndividual << " individuals"<<std::endl;
                //std::cerr << "AF:" << ptr->AFs[i] << "\tUD:" << ptr->UD[i][0] << "\t" << ptr->UD[i][1] << "\tmeans:" << ptr->means[i] << std::endl;
                chr = ptr->PosVec[i].first;
                pos = ptr->PosVec[i].second;
                if (ptr->MarkerIndex[chr].find(pos) != ptr->MarkerIndex[chr].end())
                    glIndex = ptr->MarkerIndex[chr][pos];
                else
                    glIndex = ptr->GL.size() - 1;
                ptr->AFs[i] = ((ptr->UD[i][0] * tPC1 + ptr->UD[i][1] * tPC2) + ptr->means[i]) / 2.0;
                if (ptr->AFs[i] < min_af) ptr->AFs[i] = min_af;
                if (ptr->AFs[i] > max_af) ptr->AFs[i] = max_af;
                GF0 = (1 - ptr->AFs[i]) * (1 - ptr->AFs[i]);
                GF1 = 2 * (ptr->AFs[i]) * (1 - ptr->AFs[i]);
                GF2 = (ptr->AFs[i]) * (ptr->AFs[i]);
                sumLLK += log(Phred(ptr->GL[glIndex][0]) * GF0 + Phred(ptr->GL[glIndex][1]) * GF1 +
                              Phred(ptr->GL[glIndex][2]) * GF2);
                //std::cerr << "GL:" << ptr->GL[glIndex][0] << "\t" << ptr->GL[glIndex][1] << "\t" << ptr->GL[glIndex][2] << "\t" << chr << "\t" << pos << std::endl;
                //std::cerr << "AF:" << ptr->AFs[i] << "\tUD:" << ptr->UD[i][0] << "\t" << ptr->UD[i][1] << "\tmeans:" << ptr->means[i] << std::endl;
            }
            //std::cerr << "sumLLK:" << sumLLK << std::endl;
            return sumLLK;
        }
*/


        inline char findAlt(std::vector<char> &tmpBase) {
            int a[4];
            int maxIndex(-1);
            for (int i = 0; i < tmpBase.size(); ++i) {
                if (tmpBase[i] == '.' || tmpBase[i] == ',') continue;
                if (tmpBase[i] == 'A' || tmpBase[i] == 'a') a[0]++;
                else if (tmpBase[i] == 'C' || tmpBase[i] == 'c') a[1]++;
                else if (tmpBase[i] == 'T' || tmpBase[i] == 't') a[2]++;
                else if (tmpBase[i] == 'G' || tmpBase[i] == 'g') a[3]++;
                maxIndex = 0;
            }
            if (maxIndex == -1) return 0;

            for (int j = 0; j < 4; ++j) {
                if (a[j] > a[maxIndex]) maxIndex = j;
            }
            return Base[maxIndex];
        }

        inline double getConditionalBaseLK(char base, int genotype, char altBase, bool is_error) {
            if (!is_error) {
                if (genotype == 0) {
                    if (base == '.' || base == ',') {
                        return 1;
                    }
                    else
                        return 0;
                }
                else if (genotype == 1) {
                    if (base == '.' || base == ',') {
                        return 0.5;
                    }
                    else if (toupper(base) == toupper(altBase)) {
                        return 0.5;
                    }
                    else
                        return 0;
                }
                else if (genotype == 2) {
                    if (toupper(base) == toupper(altBase)) {
                        return 1;
                    }
                    else
                        return 0;
                }
                else {
                    std::cerr << "genotype error!" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            else {
                if (genotype == 0) {
                    if (base == '.' || base == ',') {
                        return 0;
                    }
                    else if (toupper(base) == toupper(altBase)) {
                        return 1. / 3.;
                    }
                    else
                        return 2. / 3.;
                }
                else if (genotype == 1) {
                    if (base == '.' || base == ',') {
                        return 1. / 6.;
                    }
                    else if (toupper(base) == toupper(altBase)) {
                        return 1. / 6.;
                    }
                    else
                        return 2. / 3.;
                }
                else if (genotype == 2) {
                    if (base == '.' || base == ',') {
                        return 1. / 3.;
                    }
                    if (toupper(base) == toupper(altBase)) {
                        return 0;
                    }
                    else
                        return 2. / 3.;
                }
                else {
                    std::cerr << "genotype error!" << std::endl;
                    exit(EXIT_FAILURE);
                }

            }


        }

        void InitialGF(double AF, double *GF) const {
            if (AF < min_af) AF = min_af;
            if (AF > max_af) AF = max_af;
            GF[0] = (1 - AF) * (1 - AF);
            GF[1] = 2 * (AF) * (1 - AF);
            GF[2] = AF * AF;
        }

        inline double computeMixLLKs(const std::vector<double> & tPC1,const std::vector<double> & tPC2,const double alpha)
        {

            double sumLLK(0);
#ifdef _OPENMP
            omp_set_num_threads(ptr->numThread);
#pragma omp parallel for reduction (+:sumLLK)
#endif
            for (size_t i = 0; i < ptr->NumMarker; ++i) {
                double markerLK(0);
                double GF[3];
                double GF2[3];
                std::string chr = ptr->PosVec[i].first;
                int pos = ptr->PosVec[i].second;
                if (ptr->viewer.posIndex.find(chr) == ptr->viewer.posIndex.end()) {
                    continue;
                }
                else if (ptr->viewer.posIndex[chr].find(pos) == ptr->viewer.posIndex[chr].end()) {
                    continue;
                }
                if(ptr->isAFknown)
                {
                    ptr->AFs[i]=ptr->knownAF[chr][pos];
                }
                else
                {
                    ptr->AFs[i]=0.;
                    for (int k = 0; k <tPC1.size(); ++k) {
                        ptr->AFs[i]+=ptr->UD[i][k] * tPC1[k];
                    }
                    ptr->AFs[i] += ptr->means[i];
                    ptr->AFs[i] /= 2.0;

                    ptr->AF2s[i]=0.;
                    for (int k = 0; k <tPC2.size(); ++k) {
                        ptr->AF2s[i]+=ptr->UD[i][k] * tPC2[k];
                    }
                    ptr->AF2s[i] += ptr->means[i];
                    ptr->AF2s[i] /= 2.0;
                }


                InitialGF(ptr->AFs[i], GF);
                InitialGF(ptr->AF2s[i], GF2);
                std::vector<char>& tmpBase = ptr->viewer.GetBaseInfoAt(chr, pos);
                std::vector<char>& tmpQual = ptr->viewer.GetQualInfoAt(chr, pos);
                if (tmpBase.size() == 0) continue;

                char altBase = ptr->ChooseBed[chr][pos].second;

                for (int geno1 = 0; geno1 < 3; ++geno1)
                    for (int geno2 = 0; geno2 < 3; ++geno2) {
                        double baseLK(0);
                        for (int j = 0; j < tmpBase.size(); ++j) {
                            baseLK += log(( alpha * getConditionalBaseLK(tmpBase[j], geno1, altBase, 1) +
                                            (1. - alpha)* getConditionalBaseLK(tmpBase[j], geno2, altBase, 1)) *
                                          Phred(tmpQual[j] - 33)
                                          + ( alpha * getConditionalBaseLK(tmpBase[j], geno1, altBase, 0) +
                                             (1. - alpha) * getConditionalBaseLK(tmpBase[j], geno2, altBase, 0)) *
                                            (1 - Phred(tmpQual[j] - 33)));
//                            std::cerr <<i<<"th marker\t"<<tmpBase[j]<<"\t"<<tmpQual[j]<<"\t"<<altBase<<"\tlocalAlpha:"<<localAlpha<<"\tgeno1:"<<geno1<<"\tgeno2:"<<geno2
//                            <<"\tgetConditionalBaseLK1:"<<getConditionalBaseLK(tmpBase[j], geno1, altBase, 1)<<"\t"<< getConditionalBaseLK(tmpBase[j], geno2, altBase, 1)<<"\tPhred:"<<Phred(tmpQual[j] - 33)
//                            <<"\tgetConditionalBaseLK0:"<<getConditionalBaseLK(tmpBase[j], geno1, altBase, 0)<<"\t"<<getConditionalBaseLK(tmpBase[j], geno2, altBase, 0)<< std::endl;
                        }
//                        std::cerr<<"baseLK:"<<baseLK;
                        markerLK += exp(baseLK) * GF[geno1] * GF2[geno2];
//                        std::cerr<<"markerLK:"<<markerLK<<std::endl;
                    }
                if (markerLK > 0)
                    sumLLK += log(markerLK);
//                std::cerr << "sumLLK:" << sumLLK << std::endl;
            }
//            std::cerr << "sumLLK:" << sumLLK << std::endl;
            return sumLLK;
        }


        int initialize() {
            globalPC=fixPC=globalPC2=fixPC2=ptr->PC[1];//only intended smaple has pre defined PCs
            globalAlpha=fixAlpha = ptr->alpha;
            llk = (0 - computeMixLLKs(fixPC,fixPC2,fixAlpha));
            return 0;
        }

        virtual double Evaluate(Vector &v) {
            double smLLK = 0;
            if(!ptr->isHeter)
            {
                if(ptr->isPCFixed) {
                    double tmpAlpha = invLogit(v[0]);
                    smLLK = 0 - computeMixLLKs(fixPC, fixPC2, tmpAlpha);
                    if (smLLK < llk) {
                        llk = smLLK;
                        globalAlpha = tmpAlpha;
                    }
                }
                else if(ptr->isAlphaFixed) {
                    std::vector<double> tmpPC(ptr->numPC,0.);
                    for (int i = 0; i <ptr->numPC ; ++i) {
                        tmpPC[i]=v[i];
                    }
                    smLLK = 0 - computeMixLLKs(tmpPC, tmpPC, fixAlpha);
                    if (smLLK < llk) {
                        llk = smLLK;
                        globalPC = tmpPC;
                        globalPC2 = tmpPC;
                    }
                }
                else {
                    std::vector<double> tmpPC(ptr->numPC,0.);
                    for (int i = 0; i <ptr->numPC ; ++i) {
                        tmpPC[i]=v[i];
                    }
                    double tmpAlpha=invLogit(v[ptr->numPC]);
                    smLLK = 0 - computeMixLLKs(tmpPC, tmpPC, tmpAlpha);
                    if (smLLK < llk) {
                        llk = smLLK;
                        globalPC = tmpPC;
                        globalPC2 = tmpPC;
                        globalAlpha = tmpAlpha;
                    }
                }
            }
            else//contamination source from different population
            {
                if(ptr->isPCFixed) {//only fixed for intended sample
                    std::vector<double> tmpPC(ptr->numPC,0.);
                    for (int i = 0; i <ptr->numPC ; ++i) {
                        tmpPC[i]=v[i];
                        }
                    double tmpAlpha = invLogit(v[ptr->numPC]);
                    smLLK = 0 - computeMixLLKs(tmpPC, fixPC2, tmpAlpha);

                    if (smLLK < llk) {
                        llk = smLLK;
                        globalPC = tmpPC;
                        globalAlpha = tmpAlpha;
                    }
                }
                else if(ptr->isAlphaFixed) {
                    std::vector<double> tmpPC(ptr->numPC,0.);
                    std::vector<double> tmpPC2(ptr->numPC,0.);

                    for (int k = 0; k <v.Length(); ++k) {
                        if(k < ptr->numPC)
                            tmpPC[k]=v[k];
                        else if(k< ptr->numPC*2)
                            tmpPC2[k-(ptr->numPC)]=v[k];
                        else {
                            error("Simplex Vector dimension error!");
                            exit(EXIT_FAILURE);
                        }
                    }
                    smLLK = 0 - computeMixLLKs(tmpPC, tmpPC2, fixAlpha);
                    if (smLLK < llk) {
                        llk = smLLK;
                        globalPC = tmpPC;
                        globalPC2 = tmpPC2;
                    }
                }
                else {
                    std::vector<double> tmpPC(ptr->numPC,0.);
                    std::vector<double> tmpPC2(ptr->numPC,0.);
                    double tmpAlpha(0.);
                    for (int k = 0; k <v.Length(); ++k) {
                        if(k < ptr->numPC)
                            tmpPC[k]=v[k];
                        else if(k < ptr->numPC*2)
                            tmpPC2[k-(ptr->numPC)]=v[k];
                        else if(k == ptr->numPC*2)
                            tmpAlpha = invLogit(v[k]);
                        else{
                            error("Simplex Vector dimension error!");
                            exit(EXIT_FAILURE);
                        }
                    }
                    smLLK = (0 - computeMixLLKs(tmpPC, tmpPC2, tmpAlpha));
                    if (smLLK < llk) {
                        llk = smLLK;
                        globalPC = tmpPC;
                        globalPC2 = tmpPC2;
                        globalAlpha = tmpAlpha;
                    }
                }
            }
            if(ptr->verbose)
            std::cerr << "globalPC:" << globalPC[0] << "\tglobalPC:" << globalPC[1]
                      << "\tglobalPC2:" << globalPC2[0] << "\tglobalPC2:" << globalPC2[1]
                      << "\tglobalAlpha:" << globalAlpha << "\tllk:" << llk <<std::endl;
            return smLLK;
        }
    };

    SimplePileupViewer viewer;
    uint32_t NumMarker;
    fullLLKFunc fn;

    std::unordered_map<std::string, std::unordered_map<uint32_t, double> > knownAF;

    double alpha;//input alpha
    std::vector<std::vector<PCtype> > UD;//input UD
    std::vector<std::vector<PCtype> > PC;//input PC
    std::vector<PCtype> means;
    std::vector<double> AFs;
    std::vector<double> AF2s;

    typedef std::unordered_map<std::string, std::unordered_map<int, std::pair<char, char> > > BED;
    BED ChooseBed;
    std::vector<region_t> BedVec;
    std::vector<std::pair<std::string, int> > PosVec;

    ContaminationEstimator();

    ContaminationEstimator(int nPC, const char *bedFile, int nThread, double ep);

    /*Initialize from existed UD*/
    /*This assumes the markers are the same as the selected vcf*/
    /*ContaminationEstimator(const std::string &UDpath, const std::string &PCpath, const std::string &Mean,
                           const std::string &pileup, const std::string &GLpath, const std::string &Bed);
    */
    int ReadMatrixUD(const std::string &path);
    /*
    int ReadMatrixPC(const std::string &path);
    */
    /*Intersect marker sites*/
    /*
    int ReadMatrixGL(const std::string &path);
    */
    int ReadChooseBed(const std::string &path);

    int ReadMean(const std::string &path);

    int ReadAF(const std::string & path);

    int ReadBam(const char *bamFile, const char *faiFile, const char *bedFile);
    /*
    int CheckMarkerSetConsistency();

    int FormatMarkerIntersection();
    */
    /*Optimize*/
    int OptimizeLLK(const std::string &OutputPrefix);
    /*
    int RunMapping();

    int writeVcfFile(const std::string &path);

    int ReadPileup(const std::string &path);
    */
    ~ContaminationEstimator();
    /*
    int RunFromVCF(const std::string VcfSiteAFFile, const std::string CurrentMPU, const std::string ReadGroup,
                   const std::string Prefix);

    int RunFromSVDMatrix(const std::string UDpath, const std::string PCpath, const std::string Mean,
                         const std::string &MPUpath, const std::string &Bed, const std::string &Prefix,
                         const std::string &ReadGroup);
    */
    int ReadSVDMatrix(const std::string UDpath, const std::string Mean);
    /*
    int FromBamtoPileup();
     */
    bool OptimizeHomoFixedPC(AmoebaMinimizer &myMinimizer);

    bool OptimizeHomoFixedAlpha(AmoebaMinimizer &myMinimizer);

    bool OptimizeHomo(AmoebaMinimizer &myMinimizer);

    bool OptimizeHeterFixedPC(AmoebaMinimizer &myMinimizer);

    bool OptimizeHeterFixedAlpha(AmoebaMinimizer &myMinimizer);

    bool OptimizeHeter(AmoebaMinimizer &myMinimizer);
};

#endif /* CONTAMINATIONESTIMATOR_H_ */
