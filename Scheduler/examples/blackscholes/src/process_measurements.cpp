#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>

using namespace std;

int testsPerAverage = 5;

vector<string> intputMeasu;
vector<vector<double>> testTimes;
vector<double> averageTestTimes;

void read_file(string filename){

    string line;

    ifstream myfile(filename);

    if(myfile.is_open()){

        while(getline(myfile, line)){

            intputMeasu.push_back(line);
        }

        myfile.close();
    }
    else{
        std::cerr << filename + " not found" <<"\n";
        exit(1);
    }
}




void proccess_measurements(){
    int testNum = -1;

    for(int i=0; i < intputMeasu.size();i++){

        string line = intputMeasu.at(i);

        if(line.find("sched iteration 0") != std::string::npos || (i  == intputMeasu.size() - 1 )){
            
            for (int j = i; j > i-100; j--){

                if (testNum == -1) break;

                line = intputMeasu.at(j);

                if(line.find("Status of thread") != std::string::npos){
                    
                    vector<double> temp;

                    while (true){
                        line = intputMeasu.at(j);
                        
                        if(line.find("Status of thread") != std::string::npos){

                            int pos = line.find("=");
                            double time = stod(line.substr(pos+2,10));
                            temp.push_back(time);
                        }
                        else{
                            testTimes.push_back(temp);
                            break;
                        }

                        j--;

                     };

                    break;
                }
            }

            testNum++;
        }
    }
    cout << testTimes.size() << " tests found" << endl;
}

void get_average_time(){

    int counter = 0;
    int temp = 0;
    double averageTime = 0;

    for(vector<vector<double>>::iterator it = begin(testTimes); it != end(testTimes); ++it){

        double max = it->at(0);
        for(int i = 1; i < it->size(); i++){
            if(it->at(i) > max){max = it->at(i);}
        }
        //cout << max << endl;
        averageTime+=max;
        counter++;

        if(counter == testsPerAverage){
            averageTestTimes.push_back(averageTime/=testsPerAverage);
            counter = 0;
            averageTime = 0;
        }
    }
    cout << "tests per average: " << testsPerAverage << endl;
    cout << "calculated: " << averageTestTimes.size() << " averages"  << endl;
}

void save_times(string prefix){

    ofstream myfile;
    myfile.open(prefix);
    
    if(myfile.is_open()){
        myfile << "6 Threads" << endl;
        for(int i = 0; i< averageTestTimes.size(); i+=3){
            myfile << averageTestTimes.at(i) << endl;
        }

        myfile << "\n12 Threads" << endl;
        for(int i = 1; i< averageTestTimes.size(); i+=3){
            myfile << averageTestTimes.at(i) << endl;
        }

        myfile << "\n24 Threads" << endl;
        for(int i = 2; i< averageTestTimes.size(); i+=3){
            myfile << averageTestTimes.at(i) << endl;
        }
    }
    myfile.close();
}


int main(int argc, char * argv[]){

    if (argc<3) {
        std::cerr << "Usage: letters_count <input file> <output file>" <<"\n";
        exit(1);
    }

    read_file(argv[1]);
    proccess_measurements();
    get_average_time();
    save_times(argv[2]);

}

// void process_measurements(){

//     int currentSchedIt = -1;
//     double results[4];
//     string temp;
//     string temp2;
//     int pos;
//     int pos2;

//     string iterationHeader = "new scheduler update";
//     string iterationAvgPerfHeader = "Average Performances";
//     string threadFinHeader = "FINISHED!";

//     for(int i=0; i < intputMesu.size();i++){
        
//         string line = intputMesu.at(i);

//         if(line.find(iterationHeader) != std::string::npos)
//         {
//             currentSchedIt++;          
//         }
 
//         if(line.find(iterationAvgPerfHeader) != std::string::npos){
    
//             string delimiter = ",";

//             for(int j=0; j < 2; j++){
//                 temp = intputMesu.at(i+j+1);
//                 pos = temp.find(delimiter);

//                 temp2 = temp.substr(0,pos);
//                 pos2 = temp2.find(":");
//                 results[2*j] = stod(temp2.substr(pos2+1,10));

//                 temp2 = temp.substr(pos+1,temp.size());

//                 pos2 = temp2.find("f.");
//                 results[2*j+1] = stod(temp2.substr(pos2+2,10));
//             }

//             currentAvgPerf.push_back(results[0]);
//             runAvgPerf.push_back(results[1]);
//             currentBalPerf.push_back(results[2]); 
//             runAvBalPerf.push_back(results[2]);
//             i+=2;
//         }
//     }

//     // thread finish times
//     bool threadFinTime = true;
//     int i = intputMesu.size()-2;
//     while (threadFinTime){

//         string line  = intputMesu.at(i);

//         if(line.find(threadFinHeader) != std::string::npos){
//             int pos = line.find("=");
//             double time = stod(line.substr(pos+2,10));
//             threadTime.insert(threadTime.begin(),time);
//         }
//         else{

//             threadFinTime = false;
//         }

//         i--;

//     };
// }
