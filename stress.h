#ifndef STRESS
#define STRESS

#include <queue>
#include <string>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <math.h>
#include <thread>
#include <mutex>

static std::mutex stressTheoryMTX;

struct Tensor {
    double sigmaXX, sigmaYY, sigmaZZ, tauXY;
    Tensor() {this->addData();}
    ~Tensor() {}

    void addData();
};

struct YieldStress {
    double MSST_stress, DET_stress;
};

class mohrsCircle {
    protected:
        Tensor myTensor;
        double radius, center;
        std::queue<double> principalValues;
        std::string type;

        virtual void findRadius();
        virtual void findCenter();
        virtual void determinePrincipals();

    public:
        virtual void printInformation();
        virtual std::queue<double> getPrincipals() {return this->principalValues;}

        // Will implement during GUI stage
        // virtual void drawCircle();
};

class stressCircle : public mohrsCircle {
    public:
        stressCircle(Tensor directionalStresses);
        ~stressCircle() {}
};

class strainCircle : public mohrsCircle {
    public:
        strainCircle(Tensor directionalStrains);
        ~strainCircle() {}
};

YieldStress findStresses();

template <typename T>
double normalStress(T shape);

template <typename T>
double torsion(T shape);

template <typename T>
double hoopStress(T shape);

template <typename T>
double longStress(T shape);


#endif