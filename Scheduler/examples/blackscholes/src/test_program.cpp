#include <iostream>
#include <string>
#include <fstream>

using namespace std;

string headerFilePath;
string programName;
string outputFile;
int timesRepeat = 5;

void runCommand(int cores, string outputfile);

void runStepSizeTests(){

    for(double i = 0.005; i<=0.1;i+=0.01){

        ofstream myfile;
        myfile.open(headerFilePath);
        
        if(myfile.is_open()){

            myfile << "#define STEP_SIZE\t\t\t" << i << endl;
            myfile << "#define LAMBDA\t\t\t\t" << 0.02 << endl;
            myfile << "#define SCHED_PERIOD	\t" << 0.20 << endl;
            myfile << "#define GAMMA\t\t\t\t" << 0.02 << endl;
            myfile << "#define OS_MAPPING\t\t\t" << 0 << endl;
            myfile << "#define RL_MAPPING\t\t\t" << 1 << endl;
            
        }
        myfile.close();

        system("make");

        for(int t = 6; t<=24; t*=2){
            for(int j = 0; j<timesRepeat; j++){
                runCommand(t, outputFile + "step_size.txt");
           }
        }
    }
}

void runLambdaTests(){

    for(double i = 0.04; i<=2;i+=0.2){

        ofstream myfile;
        myfile.open(headerFilePath);
        
        if(myfile.is_open()){

            myfile << "#define STEP_SIZE\t\t\t" << 0.005 << endl;
            myfile << "#define LAMBDA\t\t\t\t" << i << endl;
            myfile << "#define SCHED_PERIOD	\t" << 0.20 << endl;
            myfile << "#define GAMMA\t\t\t\t" << 0.02 << endl;
            myfile << "#define OS_MAPPING\t\t\t" << 0 << endl;
            myfile << "#define RL_MAPPING\t\t\t" << 1 << endl;
            
        }
        myfile.close();

        system("make");

        for(int t = 6; t<=24; t*=2){
            for(int j = 0; j<timesRepeat; j++){
                runCommand(t, outputFile + "_lambda.txt");
           }
        }
    }
}

void runSchedPeriodTests(){

    for(double i = 0.05; i<=1;i+=0.1){

        ofstream myfile;
        myfile.open(headerFilePath);
        
        if(myfile.is_open()){

            myfile << "#define STEP_SIZE\t\t\t" << 0.005 << endl;
            myfile << "#define LAMBDA\t\t\t\t" << 0.02 << endl;
            myfile << "#define SCHED_PERIOD	\t" << i << endl;
            myfile << "#define GAMMA\t\t\t\t" << 0.02 << endl;
            myfile << "#define OS_MAPPING\t\t\t" << 0 << endl;
            myfile << "#define RL_MAPPING\t\t\t" << 1 << endl;
            
        }
        myfile.close();

        system("make");

        for(int t = 6; t<=24; t*=2){
            for(int j = 0; j<timesRepeat; j++){

                runCommand(t, outputFile + "_sched_period.txt");

           }
        }
    }
}

void runGammaTests(){

    for(double i = 0.01; i<=1; i+=0.1){

        ofstream myfile;
        myfile.open(headerFilePath);
        
        if(myfile.is_open()){

            myfile << "#define STEP_SIZE\t\t\t" << 0.005 << endl;
            myfile << "#define LAMBDA\t\t\t\t" << 0.02 << endl;
            myfile << "#define SCHED_PERIOD	\t" << 0.20 << endl;
            myfile << "#define GAMMA\t\t\t\t" << i << endl;
            myfile << "#define OS_MAPPING\t\t\t" << 0 << endl;
            myfile << "#define RL_MAPPING\t\t\t" << 1 << endl;
            
        }
        myfile.close();

        system("make");

        for(int t = 6; t<=24; t*=2){
            for(int j = 0; j<timesRepeat; j++){

                runCommand(t, outputFile + "_gamma.txt");
           }
        }
    }
}

void runOSTests(){
        ofstream myfile;
        myfile.open("headerFilePath");
        
        if(myfile.is_open()){

            myfile << "#define STEP_SIZE\t\t\t" << 0.005 << endl;
            myfile << "#define LAMBDA\t\t\t\t" << 0.02 << endl;
            myfile << "#define SCHED_PERIOD	\t" << 0.20 << endl;
            myfile << "#define GAMMA\t\t\t\t" << 0.02 << endl;
            myfile << "#define OS_MAPPING\t\t\t" << 1 << endl;
            myfile << "#define RL_MAPPING\t\t\t" << 0 << endl;
            
        }
        myfile.close();

        system("make");

    for(int t = 6; t<=24; t*=2){
        for(int j = 0; j<3; j++){

            runCommand(t, outputFile + "_os.txt");
        }
    }
}

void runCommand(int cores, string outputfile) {

    string command = programName + " " + to_string(cores) + " input.txt output.txt >>" + outputfile;
    cout << command << endl;
    system(&command[0]);

}


int main(int argc, char * argv[]){

    if (argc<3) {
        std::cerr << "Usage: ./test_program <program> <header file> <output>" <<"\n";
        exit(1);
    }

    programName = argv[1];
    headerFilePath = argv[2];
    outputFile = argv[3];

    ifstream myfile(headerFilePath);

    if(!myfile.is_open()){
        std::cerr << headerFilePath + " not found" <<"\n";
        exit(1);
    }

    runStepSizeTests();
    runLambdaTests();
    runSchedPeriodTests();
    runGammaTests();
    //runOSTests();
}
