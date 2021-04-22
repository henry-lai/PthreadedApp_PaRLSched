/*
 * evopro Innovation Kft.
 *
 * sni
 *
 * File: LSOptimizer.cpp
 * <Description>
 * Created on: Sep 18, 2014
 *
 * Author: pechan
 * (C) 2014. evopro Innovation Kft.
 *
 */

#include <ctime>
#include <cstdlib>
#include <stdexcept>

#include "LSOptimizer.h"

//************************************************
//**********   class ArithmeticVector   **********
//************************************************

bool sniutils::ArithmeticVector::rngInitialized = false;

sniutils::ArithmeticVector sniutils::ArithmeticVector::operator+(const ArithmeticVector& rhs) {

	ArithmeticVector summation;
	unsigned int idx;

	idx = 0;
	while ((idx < size()) && (idx < rhs.size())) {
		summation.push_back(this->operator[](idx) + rhs[idx]);
		idx++;
	}

	return summation;
}

sniutils::ArithmeticVector sniutils::ArithmeticVector::operator-(const ArithmeticVector& rhs) {

	ArithmeticVector result;
	unsigned int idx;

	idx = 0;
	while ((idx < size()) && (idx < rhs.size())) {
		result.push_back(this->operator[](idx) - rhs[idx]);
		idx++;
	}

	return result;
}

sniutils::ArithmeticVector sniutils::ArithmeticVector::operator*(float rhs) {

	ArithmeticVector result;
	unsigned int idx;

	for (idx = 0; idx < size(); idx++) {
		result.push_back(rhs*(this->operator[](idx)));
	}

	return result;
}

void sniutils::ArithmeticVector::forceLimits(const ArithmeticVector& lowLims, const ArithmeticVector& highLims) {

	unsigned int idx;

	idx = 0;
	while ((idx < size()) && (idx < lowLims.size()) && (idx < highLims.size())) {

		if (this->operator[](idx) < lowLims[idx]) {
			this->operator[](idx) = lowLims[idx];
		}

		if (this->operator[](idx) > highLims[idx]) {
			this->operator[](idx) = highLims[idx];
		}

		idx++;
	}
}

sniutils::ArithmeticVector sniutils::ArithmeticVector::getRand(const ArithmeticVector& limit) {

	ArithmeticVector randVec;
	unsigned int idx;
	float temp;

	//initialize RNG if necessary
	if (rngInitialized == false) {

		srand((unsigned int) time(NULL));
		rngInitialized = true;
	}

	//generate random value for each element
	for (idx = 0; idx < limit.size(); idx++) {

		//generate random value between +/-1
		temp = 2.0f * (float (rand()))/(float (RAND_MAX)) - 1.0f;

		//scaling with limit and adding to output vector
		randVec.push_back(temp*limit[idx]);
	}

	return randVec;

}

//************************************************
//*************   class Optimizable   ************
//************************************************

sniutils::Optimizable::Optimizable(const ArithmeticVector& paramLowLimInit, const ArithmeticVector& paramHighLimInit) {

	//check if input vector sizes are OK
	if ((paramLowLimInit.size() != paramHighLimInit.size()) || (paramLowLimInit.size() == 0)) {
		throw invalid_argument("Vector sizes must equal and they must not be empty!");
	}

	//save number of parameters, copy limit vectors
	numParam = paramLowLimInit.size();
	paramLowLim = paramLowLimInit;
	paramHighLim = paramHighLimInit;
}

const sniutils::ArithmeticVector& sniutils::Optimizable::getLowLims() const {

	return paramLowLim;
}

const sniutils::ArithmeticVector& sniutils::Optimizable::getHighLims() const {

	return paramHighLim;
}

unsigned int sniutils::Optimizable::getNumParam() const {

	return numParam;
}

sniutils::Optimizable::~Optimizable() {
}

//************************************************
//*************   class LSOptimizer   ************
//************************************************

sniutils::LSOptimizer::LSOptimizer(const ArithmeticVector& parRandIvInit, float stopRhoLim, int stopItersLim, int consSuccLim, int consFailLim, float expFactor, float contFactor):
	expFactor(expFactor),
	contFactor(contFactor),
	consSuccLim(consSuccLim),
	consFailLim(consFailLim),
	stopRhoLim(stopRhoLim),
	stopItersLim(stopItersLim) {

	//check if parRandIvInit size is OK
	if (parRandIvInit.size() == 0) {
		throw invalid_argument("Parameter random interval vector must not be empty!");
	}

	//copy random interval vector and set number of parameters
	parRandIv = parRandIvInit;
	numParam = parRandIvInit.size();
}

float sniutils::LSOptimizer::doLS(Optimizable& funToOpt, const ArithmeticVector& parStart) {

		float rho;
		int consSucc, consFail, iterCnt;
		ArithmeticVector parCurr;
		ArithmeticVector parCand;
		ArithmeticVector parBias;
		ArithmeticVector parDev;
		ArithmeticVector parLowLim;
		ArithmeticVector parHighLim;
		float scoreCurr;
		float scoreCand;

		//checking if number of parameters match each other
		if ((funToOpt.getNumParam() != numParam) || (parStart.size() != numParam)) {
			throw invalid_argument("Number of parameters of funToOpt and size of parStart must match the expected value!");
		}

		// initializing variables
		rho = 1.0f;
		consSucc = 0;
		consFail = 0;
		iterCnt = 0;
		parCurr = parStart;
		parBias.resize(numParam, 0.0f);
		parCand.resize(numParam, 0.0f);
		parDev.resize(numParam, 0.0f);
		parLowLim = funToOpt.getLowLims();
		parHighLim = funToOpt.getHighLims();

		//calculate initial score
		parCurr.forceLimits(parLowLim, parHighLim);
		scoreCurr = funToOpt.getScore(parCurr);

		// local search iteration cycles
		while ((iterCnt < stopItersLim) && (rho > stopRhoLim)) {

			// generating a new random deviate between +/-(deviate*rho)
			parDev = ArithmeticVector::getRand(parRandIv*rho);

			// generating new candidate, ensuring that it is between limits
			parCand = parCurr + parBias + parDev;
			parCand.forceLimits(parLowLim, parHighLim);

			// calculating score of candidate
			scoreCand = funToOpt.getScore(parCand);

			if (scoreCand < scoreCurr) {

				// if candidate is better, updating variables accordingly
				scoreCurr = scoreCand;
				parCurr = parCand;
				parBias = parBias * 0.6f + parDev * 0.4f;
				consSucc++;
				consFail = 0;
			}
			else {

				// if this candidate is worser, checking opposite direction
				// generating new candidate, ensuring that it is between limits
				parCand = parCurr - (parBias + parDev);
				parCand.forceLimits(parLowLim, parHighLim);

				// calculating score of candidate
				scoreCand = funToOpt.getScore(parCand);

				if (scoreCand < scoreCurr) {

					// if this candidate is better, updating variables accordingly
					scoreCurr = scoreCand;
					parCurr = parCand;
					parBias = parBias * 0.6f - parDev * 0.4f;
					consSucc++;
					consFail = 0;
				}
				else {

					// if both candidates were worser, updating variables accordingly
					parBias = parBias * 0.5f;
					consSucc = 0;
					consFail++;
				}
			}

			// updating rho if necessary
			if (consSucc >= consSuccLim) {
				rho = expFactor * rho;
				consSucc = 0;
				consFail = 0;
			}

			if (consFail >= consFailLim) {
				rho = contFactor * rho;
				consSucc = 0;
				consFail = 0;
			}

			iterCnt++;
		}

	//assign results
	optScore = scoreCurr;
	optPar = parCurr;
	numIters = iterCnt;

	return optScore;
}

float sniutils::LSOptimizer::doRepeatedLS(Optimizable& funToOpt, const ArithmeticVector& parStart, unsigned int numRuns) {

	float optScoreCurr;
	ArithmeticVector optParCurr;
	unsigned int numItersTotal;
	unsigned int runIdx;

	//perform an LS run, save results
	doLS(funToOpt, parStart);
	optScoreCurr = optScore;
	optParCurr = optPar;
	numItersTotal = numIters;

	//perform further LS runs and look for best result
	for (runIdx = 1; runIdx < numRuns; runIdx++) {
		doLS(funToOpt, parStart);
		if (optScore < optScoreCurr) {
			optScoreCurr = optScore;
			optParCurr = optPar;
		}
		numItersTotal += numIters;
	}

	//save best result
	optScore = optScoreCurr;
	optPar = optParCurr;
	numIters = numItersTotal;

	return optScore;
}

float sniutils::LSOptimizer::getOptScore() {

	return optScore;
}

const sniutils::ArithmeticVector& sniutils::LSOptimizer::getOptPar() {

	return optPar;
}

int sniutils::LSOptimizer::getNumIters() {

	return numIters;
}
