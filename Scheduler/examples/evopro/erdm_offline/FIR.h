#ifndef ERDM_PC_FIR_H_
#define ERDM_PC_FIR_H_

#include <vector>
#include <cstddef> // size_t

class FIR {
private:
	std::vector<float> coefs;
	std::vector<float> samples;
	unsigned filterBufferPointer;
public:
	FIR();
	FIR(const std::vector<float>& coefs_);
	FIR(const float* coefs_, unsigned len_);
	float filter(float inp);
};

#endif /* ERDM_PC_FIR_H_ */
