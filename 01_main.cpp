/*
VERSION CONTROL SYSTEM

USER:- 
1) NEW USER
2) REMOVE USER
3) CHANGE PASSWORD
4) LOGOUT
5) SAVE IN SOME FILE ( LOGIN DETAILS )

FILE / VERSION CONTROL :-
1) CREATE ( INIT  REPO )
2) CREATE NEW FILE
3) TRACK CHANGES ( GIT STATUS ) 
4) ADD TO EXISTING ( COMMIT TO FILE )
5) DELETE FILE 
6) MERGE FILE

*/

#include<iostream>
#include<fstream>

using namespace std;

class File{
    private :

    public:
};

int main(){
    fstream file("01_example.txt",ios::in |ios::out |ios::app);
    if (!file) { 
            std::cerr << "Error opening file." << endl;
            return 1; 
        }

        file << "Appending new line to file." << endl;

        file.seekg(0);

        string line;
        while (getline(file, line)) {
            cout << line << endl;
        }

        file.close();
        return 0;
}