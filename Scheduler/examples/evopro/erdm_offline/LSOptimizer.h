/*
 * evopro Innovation Kft.
 *
 * sni
 *
 * File: LSOptimizer.h
 * <Description>
 * Created on: Sep 18, 2014
 *
 * Author: pechan
 * (C) 2014. evopro Innovation Kft.
 *
 */

#ifndef ERDM_PC_LSOPTIMIZER_H_
#define ERDM_PC_LSOPTIMIZER_H_

#include <vector>

using namespace std;

namespace sniutils {

//class extending stl vector<float> class, adding element-wise arithmetic operations
class ArithmeticVector: public vector<float> {

private:

	static bool rngInitialized;

public:

	//constructors
	ArithmeticVector() : vector<float>() {};
	ArithmeticVector(const float* begin, const float* end) : vector<float>(begin, end) {};

	//element-wise addition of vectors
	//rhs: right hand side operand
	//return: summation vector
	ArithmeticVector operator+(const ArithmeticVector& rhs);

	//element-wise subtraction of vectors
	//rhs: right hand side operand
	//return: result vector
	ArithmeticVector operator-(const ArithmeticVector& rhs);

	//scalar (element-wise) multiplication
	//rhs: right hand side operand
	//return: result vector
	ArithmeticVector operator*(float rhs);

	//ensures that each vector element is between the given limits (if not, the value will be changed to limit)
	//lowLims: specifies the lower bound for each vector element
	//hihgLims: specifies the upper bound for each vector element
	void forceLimits(const ArithmeticVector& lowLims, const ArithmeticVector& highLims);

	//generates a random vector of equal length than the input, each element will be generated in the interval -limit[i]...+limit[i]
	//limit: vector with the interval limits
	//return: random output vector
	static ArithmeticVector getRand(const ArithmeticVector& limit);
};

//absract class representing a function that can be optimized along a parameter vector
class Optimizable {

private:

	unsigned int numParam;
	ArithmeticVector paramLowLim;
	ArithmeticVector paramHighLim;

public:

	//constructor
	//paramLowLimInit: vector of lower bounds of parameters
	//paramHighLimInit: vector of upper bounds of parameters
	//throws invalid_argument exception if vector sizes are not equal or they are empty
	Optimizable(const ArithmeticVector& paramLowLimInit, const ArithmeticVector& paramHighLimInit);

	//return: vector of lower bounds of parameters
	const ArithmeticVector& getLowLims() const;

	//return: vector of upper bound of parameters
	const ArithmeticVector& getHighLims() const;

	//return: number of parameters of the function
	unsigned int getNumParam() const;

	//function to be optimized
	//paramVec: vector containing parameter values
	//return: value of the function with the given parameters
	virtual float getScore(const ArithmeticVector& paramVec) = 0;

	virtual ~Optimizable();
};

//class implementing Solis-Wets local search algorithm
class LSOptimizer {

private:

	//local search parameters
	ArithmeticVector parRandIv;
	unsigned int numParam;
	float expFactor;
	float contFactor;
	int consSuccLim;
	int consFailLim;
	float stopRhoLim;
	int stopItersLim;

	//local search results
	float optScore;
	ArithmeticVector optPar;
	int numIters;

public:

	//constructor
	//parRandIvInit: each vector element specifies the interval (+/-) for the corresponding parameter which will be used when generating random deviation
	//before scaling with current rho value, vector size must be equal to the number of parameters
	//stopRhoLim: stop condition based on rho - when rho becomes lower than this value, the search is to be stopped
	//stopItersLim: stop condition based on number of iterations - when number of iterations reaches this value, the search is to be stopped
	//consSuccLim: number of consecutive successes after which rho expansion is to be performed
	//consFailLim: number of consecutive failures after which rho contraction is to be performed
	//expFactor: rho expansion factor
	//contFactor: rho contraction factor
	//throws invalid_argument exception if parRandIvInit is empty
	LSOptimizer(const ArithmeticVector& parRandIvInit, float stopRhoLim = 0.05f, int stopItersLim = 300, int consSuccLim = 40, int consFailLim = 40, float expFactor = 2.0f, float contFactor = 0.5f);

	//performs local search starting at the specified parameter vector with the parameters set
	//funToOpt: Optimizable object implementing the function to be optimized
	//parStart: initial value of the parameters (starting point)
	//return: the final score (value of funToOpt at the optimal parameters)
	//thorws invalid_argument exception if the number of parameters for funToOpt or size of parStart does not equal the size of constructor parameter parRandIvInit
	float doLS(Optimizable& funToOpt, const ArithmeticVector& parStart);

	//performs the specified number of local search runs starting always at the specified parameter vector with the parameters set
	//funToOpt: Optimizable object implementing the function to be optimized
	//parStart: initial value of the parameters (starting point)
	//numRuns: number of LS runs (one run will be performed even if set to 0)
	//return: the final score (value of funToOpt at the optimal parameters)
	//thorws invalid_argument exception if the number of parameters for funToOpt or size of parStart does not equal the size of constructor parameter parRandIvInit
	float doRepeatedLS(Optimizable& funToOpt, const ArithmeticVector& parStart, unsigned int numRuns);

	//return: the final (optimal) score (if doLS or doRepeatedLS has been called previously)
	float getOptScore();

	//return: the vector containing the optimal parameters (if doLS or doRepeatedLS has been called previously)
	const ArithmeticVector& getOptPar();

	//return: the number of iterations during the last LS or the total number of iterations during the last repeated LS search (if doLS or doRepeatedLS has been called previously)
	int getNumIters();
};

}

#endif
