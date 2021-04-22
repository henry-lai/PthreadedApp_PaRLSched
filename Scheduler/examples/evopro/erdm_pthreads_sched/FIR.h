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
	float filter(float inp, int size);
	inline int getCoefsSize() {
	  return coefs.size();
	}
	inline std::vector<float> getCoefs() {
	  return coefs;
	}
};

#endif /* ERDM_PC_FIR_H_ */
