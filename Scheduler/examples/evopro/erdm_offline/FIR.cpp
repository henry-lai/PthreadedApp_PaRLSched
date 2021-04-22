#include <cstdio>
#include "FIR.h"

#include <iostream>

FIR::FIR()
	: filterBufferPointer(0) {}

FIR::FIR(const float* coefs_, unsigned len_)
	: coefs(coefs_, coefs_ + len_),
	  samples(len_, 0),
	  filterBufferPointer(0) {}

FIR::FIR(const std::vector<float>& coefs_)
	: coefs(coefs_),
	  samples(coefs_.size(), 0),
	  filterBufferPointer(0) {}

float FIR::filter(float inp) {
	float sum = 0;
	unsigned idx_inp;
	unsigned idx_coefs = 0;
	const unsigned len = coefs.size();
	if (filterBufferPointer == 0) {
		filterBufferPointer = len;
	}
	filterBufferPointer--;
	samples[filterBufferPointer] = inp;

	float coef;
	float sample;
	std::vector<float>& samples_local = samples;
	std::vector<float>& coefs_local = coefs;
	unsigned fbp = filterBufferPointer;

	for (idx_coefs = 0; idx_coefs < len; ++idx_coefs) {
		idx_inp = (fbp + idx_coefs) % len;
		sample = samples_local[idx_inp];
		coef = coefs_local[idx_coefs];
		sum += coef * sample;
	}
/*



	float* coefs_local = coefs;
	float* samples_local = samples;
	unsigned fbp_local = filterBufferPointer;
	unsigned len_local = len;
	unsigned idx_coefs;
	[[ rpr::pipeline, stream(idx_coefs, idx_inp, coef, sample, res) ]]
	for (idx_coefs = 0; idx_coefs < len_local; ++idx_coefs) {
		[[ rpr::kernel, rpr::in(idx_coefs, fbp_local, len_local), rpr::out(idx_inp), rpr::keep(idx_coefs) ]] {
			idx_inp = (fbp_local + idx_coefs) % len_local;
		}
		[[ rpr::kernel, rpr::in(idx_coefs), rpr::out(coef, sample) ]] {
			coef = coefs_local[idx_coefs];
			sample = samples_local[idx_inp];
		}
		[[ rpr::kernel, rpr::in(coef, sample), rpr::reduce(add, res) ]] {
			res = sample * coef;
		}
	}
	return res;*/

/*	for (idx_coefs = 0; idx_coefs < len; ++idx_coefs) {
		idx_inp = (filterBufferPointer + idx_coefs) % len;
		sum += coefs[idx_coefs] * samples[idx_inp];
	}*/
	return sum;
}

