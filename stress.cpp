#include "stress.h"

void Tensor::addData() {
    // User input to enter data to tenosr as well as instructions as to how to do it.
    std::cout << "Enter normal stress values in order of xyz seperated by spaces: " << std::endl;
    std::cin >> this->sigmaXX >> this->sigmaYY >> this->sigmaZZ;
    std::cout << "\nEnter shear stress values for the XY direction: " << std::endl;
    std::cin >> this->tauXY;
}

void mohrsCircle::findCenter() {
    // Finds the cetner by taking an average of the x and y stresses.
    this->center = (myTensor.sigmaXX + myTensor.sigmaYY)/2;
}

void mohrsCircle::findRadius() {
    // Uses pythagorean thm to find the radius.
    this->radius = sqrt(pow(this->myTensor.tauXY, 2) + pow(this->myTensor.sigmaXX - this->center, 2));
}

void mohrsCircle::determinePrincipals() {
    // Adds sigma 1 and 2 into the queue
    this->principalValues.push(this->center + this->radius);
    this->principalValues.push(this->center - this->radius);
    // Adds sigma 3 to queue
    this->principalValues.push(this->myTensor.sigmaZZ);
    // Adds tau max to the queue
    this->principalValues.push(this->radius);
}

void mohrsCircle::printInformation() {
    std::queue tempValues = principalValues; // Creates a temporary queue that can be emptied during for loop.

    std::cout << "Principal " << this->type << ", in order from I-III and max shear, is: " << std::endl;
    while (tempValues.empty() == false) {
        // Prints values then removes it from the temp queue
        std::cout << tempValues.front() << std::endl;
        tempValues.pop();
    }
}

stressCircle::stressCircle(Tensor directionalStresses) {
    this->type = "stresses"; // Label for printInformation
    this->myTensor = directionalStresses; // Stress tensor based on user input

    // Determines important data for the circle
    this->findCenter();
    this->findRadius();
    this->determinePrincipals();
}

strainCircle::strainCircle(Tensor directionalStrains) {
    this->type = "strains"; // Label for printInformation
    this->myTensor = directionalStrains; // Strain tensor based on user input

    // Determines important data for the circle
    this->findCenter();
    this->findRadius();
    this->determinePrincipals();
}

YieldStress findStresses() {
    double s1, s2, s3;
    std::vector<double> principalStresses = {s1, s2, s3};

    YieldStress thisYield; // Declares struct that will be returned. Will have multiple threads writing to it.

    // Takes user input for the principle stresses. This may change as a GUI is added
    std::cout << "Enter principal stresses from I-III seperated with spaces: " << std::endl;
    std::cin >> s1 >> s2 >> s3;


    /*
        On the threads:
        Threads here are in order to complete both calculations at the same time, and with the use of a mutex variable
        avoid any odd behaviors on the shared structure. As structures are not thread proof, they will not be able to 
        read/write at the same time.

        On the lambda functions:
        The lambda functions simply apply the formulas for MSST and DET. Then they write the data to the structure to be
        returned by the function. It doesn't make sense to create a seperate functions for these theories as we most 
        likely will not use them outside of this one case. This can change with the implimentation of a GUI, however
        it most likely won't unless there are other functionalities implemented later.
    */

    std::thread t1 ([&] () {
        stressTheoryMTX.lock();
        // Calculates the yield based on the DET formula.
        thisYield.DET_stress = sqrt(pow(principalStresses[0], 2) + pow(principalStresses[1], 2) + 
                pow(principalStresses[2], 2) - (principalStresses[0] * principalStresses[1]) -
                (principalStresses[0] * principalStresses[1]) - (principalStresses[0] * principalStresses[1]));

        stressTheoryMTX.unlock();}
    );

    std::thread t2 ([&] () {
        std::vector<double> possibleYield;
        double yield;
    
        // Main equations of the MSST method (ie. sigma1-sigma2=sigmaYield)
        possibleYield.emplace_back(abs(principalStresses[0] - principalStresses[1]));
        possibleYield.emplace_back(abs(principalStresses[1] - principalStresses[2]));
        possibleYield.emplace_back(abs(principalStresses[2] - principalStresses[0]));

        stressTheoryMTX.lock();
        // Helps avoid possible issues in defining the limiting case.
        yield = possibleYield[0];

        // Determines the limiting case for yielding by determining if the current value is smaller than the current.
        for (auto stress : possibleYield) {
            if (stress < yield) {
                yield = stress;
            }
        }
        
        thisYield.MSST_stress = yield;
        stressTheoryMTX.unlock();}
    );

    // Joins the threads so that there is no odd behavior in continuous thread use.
    t1.join(), t2.join();
    
    return thisYield;
}

// Normal stress of a shape given an applied load.
template <typename T>
double normalStress(T shape) {
    double area = shape.area();
    double load;

    std::cout << "Enter applied load: " << std::endl;
    std::cin >> load;

    double stress = load/area;
    return stress;
}

// Torsional stress of a shape for a given torque.
template <typename T>
double torsion(T shape) {
    double radius;
    double appliedTorque;

    if (shape.name == "Rod") {
        radius = shape.getRadius();
    }
    else if (shape.name == "Pipe") {
        radius = shape.getMeanRadius();
    }

    std::cout << "Enter applied torque: " << std::endl;
    std::cin >> appliedTorque;

    return (appliedTorque * radius) / shape.pmInertia();
}

/*
    Determines the hoop stress of a pressure vessel. Dtermined by the pressure applied to the interior times the
    radius divided by the thickness of the wall. Also catches if the shape cannot be considered a pressure vessel
    i.e. it is a solid shape.
*/
template <typename T>
double hoopStress(T shape) {
    double pressure;
    double thickness;

    if (shape.name == "Pipe") {
        std::cout << "Enter pressure: " << std::endl;
        std::cin >> pressure;

        thickness = shape.getThickness();

        return (pressure * shape.getRadius()) / thickness;
    }
    else {
        std::cout << "ERROR: Pressure cannot be determined for this shape." << std::endl;
        return 0;
    }
}

/*
    Longitudinal/axial stress of a pressure vessel. Determined by taking the hoop stress and deviding it by 
    two (by definition the axial stress is half the hoop stress). Checks to see if the shape can be considered a 
    pressure vessel (currently only pipes fit this definition and this is because they may function the same as
    a cylinder if needed). 
*/
template <typename T>
double longStress(T shape) {
    double stress;
    double pressure;
    double thickness;

    if (shape.name == "Pipe") {
        std::cout << "Enter pressure: " << std::endl;
        std::cin >> pressure;

        thickness = shape.getThickness();

        return (pressure * shape.getRadius()) / (2 * thickness);
    }
    else {
        std::cout << "ERROR: Pressure cannot be determined for this shape." << std::endl;
        return 0;
    }
}