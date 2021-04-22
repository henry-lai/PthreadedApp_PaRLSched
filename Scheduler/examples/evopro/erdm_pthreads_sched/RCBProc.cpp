#include <cmath>
#include <sstream>

#include "Utils.h"
#include "RCBProc.h"
#include "TaskQueue.hxx"

using namespace sniutils;

#ifdef SCHEDULER
#include "Scheduler.h"
#endif

RCBProc::RCBProc(unsigned char addr, int initialTimestamp, int temp)
	: address(addr),
	  temperature(temp),
	  bp(FIR(bandpass_coefs, filterLength)),
	  hil(FIR(hilbert_coefs, filterLength)),
	  t0(initialTimestamp) {}

RCBProc::RCBProc() :address(0), temperature(0) {}

void RCBProc::setSize(unsigned int size) {
	rawData = std::vector<float>(size, 0);
	filteredData = std::vector<float>(size, 0);
}

void RCBProc::loadData(float data, int pos) {
	rawData[pos] = data;
}

//#ifdef SCHEDULER
int Context::task(int jobID, int threadID) {
  int data_chunk_size = rcb.rawData.size() / m_threads;
  int bp_size = rcb.bp.getCoefsSize();
  const std::vector<float> &bp_coefs = rcb.bp.getCoefs();
  int hil_size = rcb.hil.getCoefsSize();
  const std::vector<float> &hil_coefs = rcb.hil.getCoefs();
  int raw_size = rcb.rawData.size();
  for (int i=threadID*data_chunk_size; i<(threadID+1)*data_chunk_size; i++)
    if (i<raw_size) {
      float filt_sum = 0, sum_bp = 0, sum_hil = 0;
      for (unsigned int idx_raw = i; idx_raw > i - bp_size && idx_raw >=0; idx_raw--)
        sum_bp = bp_coefs[i-idx_raw] * rcb.rawData[idx_raw];
      for (unsigned int idx_raw = i; idx_raw > i - hil_size && idx_raw >=0; idx_raw--)
        sum_hil = hil_coefs[i-idx_raw] * rcb.rawData[idx_raw];
      rcb.filteredData[i] = sqrtf(sum_bp*sum_bp + sum_hil*sum_hil);
    }
  return THREAD_RUNNING;
}
//#endif

/*
int Context::task(int jobID, int threadID) {
  int data_chunk_size = rcb.rawData.size() / m_threads;
  float fbp, fh;

  double startTime1= Utils::getTimeSec();
int bp_size = 52; //rcb.bp.getCoefsSize();
int hil_size = 52; //rcb.hil.getCoefsSize();
  int raw_size = rcb.rawData.size();
  for (int i=threadID*data_chunk_size; i<(threadID+1)*data_chunk_size; i++)
    if (i<raw_size) {
fbp = rcb.bp.filter(rcb.rawData[i], bp_size);
fh = rcb.hil.filter(rcb.rawData[i], hil_size);
      rcb.filteredData[i] = sqrtf(fbp*fbp + fh*fh);
    }
  double endTime= Utils::getTimeSec();
  fprintf (stderr, "Task %d runtime is %f\n", threadID, endTime-startTime1);
  return THREAD_RUNNING;
}
*/

void RCBProc::filterData() {
  extern Context context;
  if (!context.threadsCreated()) {
    context.createThreads(context.getNumThreads());
    context.setThreadsCreated(true);
  }
  context.startThreads();
  context.waitForAllThreads();
  //float fbp, fh;
  //for (unsigned i = 0; i < rawData.size(); ++i) {
  //fbp = bp.filter(rawData[i]);
  // fh = hil.filter(rawData[i]);
  //filteredData[i] = sqrtf(fbp*fbp + fh*fh);
  //}
}

void RCBProc::triggerAndDecimateData() {
	int pulseStart, length;
	float f;
	unsigned on = 0;
	for (unsigned i = 0; i < filteredData.size(); ++i) {
		f = filteredData[i];
		f = (f < 0)?(-f):f; // abs
		if (f > (thr - on * hys)) {
			if (!on) {
				pulseStart = i + t0;
				length = 0;
			}
			on = 1;
		}
		else if(on) {
			decimatedData.push_back(tDecimatedData());
			tDecimatedData& dec = decimatedData.back();
			decimate(dec.samples, pulseStart, length);
			dec.pl = round(length/6.25);
			dec.Ts = round(pulseStart/6.25);
			on = 0;
		}
		++length;
	}
}

void RCBProc::decimate(std::vector<int>& out, int start, int len) {
	static const int newlen = 64;
	int i;
	float step = 1.0*(len-1)/(newlen-1);
	float pos, w1, w2, f;
	int p1, p2;
	out.clear();
	// Generate /newlen/ values
	for (i = 0; i < newlen; ++i) {
		pos = start + i * step;
		p1 = floor(pos);
		p2 = ceil(pos);
		if (p1 == p2)
			f = filteredData[p1];
		else {
			w1 = p2-pos;
			w2 = pos-p1;
			f = filteredData[p1]*w1 + filteredData[p2]*w2;
		}
		out.push_back(massFactor*f);
	}
}

const std::string& RCBProc::getJson() {
	std::stringstream ss;
	int l = decimatedData.size();
	int i, j;
	int axleNo = 0;
	ss << "  {\"Add\":\"" << sniutils::Utils::lwHexEncode(address) << "\",\"ID\":" << (int)address << ",\"NumE\":" << l << ",\"E\":\n   [\n";
	for (i = 0; i < l; ++i) {
		tDecimatedData& data = decimatedData[i];
		ss << "    {\"Ax\":" << axleNo++ << ",\"Off\":0,\"Pl\":" << data.pl << ",\"Samp\":[";
		for (j = 0; j < 63; ++j) ss << data.samples[j] << ",";
		ss << data.samples.back() << "],\"Stat\":0,\"T\":" << temperature << ",\"Ts\":" << data.Ts << "}";
		if (i != l-1) ss << ",";
		ss << "\n";
	}
	ss << "   ]\n  }";
	outputS = ss.str();
	return outputS;
}
