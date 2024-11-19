#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <map>
#include <ctime>
#include <cstdlib>

using namespace std;

class User {
protected:
    string userID;
    string password;

    static string hashPassword(const string& pass) {
        int hash = 0;
        for (char c : pass)
            hash += c;
        return to_string(hash);
    }

public:
    User(string id, string pass) : userID(id), password(hashPassword(pass)) {}

    string getUserID() { return userID; }

    virtual void displayRole() const = 0;

    static bool authenticate(const string& id, const string& pass) {
        ifstream file("users.txt");
        string storedID, storedHash;
        string hashedPass = hashPassword(pass);

        while (file >> storedID >> storedHash) {
            if (storedID == id && storedHash == hashedPass)
                return true;
        }
        return false;
    }

    static void registerUser(const string& id, const string& pass) {
        ofstream file("users.txt", ios::app);
        file << id << " " << hashPassword(pass) << "\n";
        file.close();
    }
};

class Logger {
public:
    static void logAction(const string& action) {
        ofstream logFile("logs.txt", ios::app);
        auto t = time(nullptr);
        logFile << "- " << ctime(&t) << "  " << action << "\n";
        logFile.close();
    }
};

class FileSystem {
private:
    string currentUserID;

    map<string, string> loadFileOwners() {
        map<string, string> fileOwners;
        ifstream ownerFile("file_owners.txt");
        string filename, owner;

        while (ownerFile >> filename >> owner) {
            fileOwners[filename] = owner;
        }
        ownerFile.close();
        return fileOwners;
    }

    void saveFileOwner(const string& filename, const string& owner) {
        ofstream ownerFile("file_owners.txt", ios::app);
        ownerFile << filename << " " << owner << "\n";
        ownerFile.close();
    }

    bool fileExists(const string& filename) {
        ifstream file(filename);
        return file.good();
    }

    void openFileForEditing(const string& filename) {
        string command;
#ifdef _WIN32
        command = "notepad " + filename; // Windows
#else
        command = "nano " + filename; // Linux/Unix
#endif
        system(command.c_str());
    }

    int getNextVersion(const string& filename) {
        string versionFile = filename + "_versions.txt";
        ifstream file(versionFile);
        int version = 0;

        if (file.is_open()) {
            file >> version;
            file.close();
        }

        return version + 1;
    }

    void saveVersion(const string& filename, const string& copyFilename) {
        int version = getNextVersion(filename);
        string versionFile = filename + "_v" + to_string(version) + ".txt";

        ifstream srcFile(copyFilename, ios::binary);
        ofstream destFile(versionFile, ios::binary);

        if (!srcFile.is_open() || !destFile.is_open()) {
            cout << "Error: Unable to save version.\n";
            return;
        }

        destFile << srcFile.rdbuf(); // Copy content
        srcFile.close();
        destFile.close();

        ofstream versionTracker(filename + "_versions.txt");
        versionTracker << version;
        versionTracker.close();

        Logger::logAction("Saved version " + to_string(version) + " for file: " + filename);
        cout << "Changes saved as version " << version << ".\n";
    }

    void saveMain(const string& filename, const string& copyFilename){
        ifstream srcFile(copyFilename, ios::binary);
        ofstream destFile(filename, ios::binary);

        if (!srcFile.is_open() || !destFile.is_open()) {
            cout << "Error: Unable to save version.\n";
            return;
        }

        destFile << srcFile.rdbuf(); // Copy content
        srcFile.close();
        destFile.close();

        Logger::logAction("Merged changes for file: " + filename);
        cout << "Changes saved" << ".\n";
    }

    void proposeMerge(const string& filename, const string& rootUser) {
        ofstream mergeRequests("merge_requests.txt", ios::app);
        mergeRequests << currentUserID << " proposes merge for " << filename << " to " << rootUser << "\n";
        mergeRequests.close();
        cout << "Merge proposal sent to " << rootUser << ".\n";
    }

    void rollbackToVersion(const string& filename, int version) {
        string versionFile = filename + "_v" + to_string(version) + ".txt";

        if (!fileExists(versionFile)) {
            cout << "Specified version does not exist.\n";
            return;
        }

        ifstream srcFile(versionFile, ios::binary);
        ofstream destFile(filename, ios::binary);

        if (!srcFile.is_open() || !destFile.is_open()) {
            cout << "Error: Unable to rollback to version.\n";
            return;
        }

        destFile << srcFile.rdbuf(); // Copy content
        srcFile.close();
        destFile.close();

        Logger::logAction("Rolled back " + filename + " to version " + to_string(version));
        cout << "File " << filename << " rolled back to version " << version << " successfully.\n";
    }

public:
    FileSystem(const string& user) : currentUserID(user) {}

    void createFile(const string& filename) {
        if (fileExists(filename)) {
            cout << "File already exists.\n";
            return;
        }
        ofstream file(filename);
        file.close();
        saveFileOwner(filename, currentUserID);
        Logger::logAction(currentUserID + " created file: " + filename);
        cout << "File " << filename << " created successfully and is owned by " << currentUserID << ".\n";
    }

    void modifyFile(const string& filename) {
        string copyFilename = filename + "_copy.txt";

        if (!fileExists(filename)) {
            cout << "File does not exist.\n";
            return;
        }

        ifstream srcFile(filename);
        ofstream copyFile(copyFilename);

        if (!srcFile.is_open() || !copyFile.is_open()) {
            cout << "Error: Unable to create a copy of the file.\n";
            return;
        }

        copyFile << srcFile.rdbuf(); // Copy content
        srcFile.close();
        copyFile.close();

        Logger::logAction(currentUserID + " created a copy for modification: " + copyFilename);

        cout << "Opened a copy of the file for modification: " << copyFilename << "\n";
        openFileForEditing(copyFilename);

        cout << "Do you want to propose merging changes to the main file? (yes/no): ";
        string response;
        cin >> response;

        if (response == "yes") {
            auto fileOwners = loadFileOwners();
            if (fileOwners.find(filename) != fileOwners.end()) {
                proposeMerge(filename, fileOwners[filename]);
            } else {
                cout << "Owner not found for this file.\n";
            }
        } else {
            saveVersion(filename, copyFilename);
        }
    }

    void mergeFile(const string& filename) {
        auto fileOwners = loadFileOwners();
        const string& copyFilename = filename + "_copy.txt";

        if (fileOwners.find(filename) != fileOwners.end() && fileOwners[filename] == currentUserID) {
            saveVersion(filename, copyFilename);
            saveMain(filename, copyFilename);
            cout << "Merged changes from " << copyFilename << " into " << filename << ".\n";
        } else {
            cout << "You do not have permission to merge changes into this file.\n";
        }
    }

    void rollbackFile(const string& filename, int version) {
        auto fileOwners = loadFileOwners();

        if (fileOwners.find(filename) != fileOwners.end() && fileOwners[filename] == currentUserID) {
            rollbackToVersion(filename, version);
        } else {
            cout << "You do not have permission to rollback this file.\n";
        }
    }
};

void menu(const string& userID) {
    FileSystem fs(userID);
    int choice;

    do {
        cout << "\n1. Create File\n2. Modify File\n3. Merge File\n4. Rollback File\n5. Logout\nEnter choice: ";
        cin >> choice;

        if (choice == 1) {
            string filename;
            cout << "Enter filename: ";
            cin >> filename;
            fs.createFile(filename);
        } else if (choice == 2) {
            string filename;
            cout << "Enter filename to modify: ";
            cin >> filename;
            fs.modifyFile(filename);
        } else if (choice == 3) {
            string filename, copyFilename;
            cout << "Enter main filename: ";
            cin >> filename;
            fs.mergeFile(filename);
        } else if (choice == 4) {
            string filename;
            int version;
            cout << "Enter filename: ";
            cin >> filename;
            cout << "Enter version to rollback to: ";
            cin >> version;
            fs.rollbackFile(filename, version);
        } else if (choice == 5) {
            Logger::logAction(userID + " logged out");
            cout << "Logged out successfully.\n";
        } else {
            cout << "Invalid choice. Try again.\n";
        }
    } while (choice != 5);
}

int main() {
    string id, pass;
    int option;

    cout << "1. Register\n2. Login\n3. Exit\nEnter choice: ";
    cin >> option;

    if (option == 1) {
        cout << "Enter userID: ";
        cin >> id;
        cout << "Enter password: ";
        cin >> pass;
        User::registerUser(id, pass);
        cout << "Registration successful.\n";
    } else if (option == 2) {
        cout << "Enter userID: ";
        cin >> id;
        cout << "Enter password: ";
        cin >> pass;
        if (User::authenticate(id, pass)) {
            Logger::logAction(id + " logged in");
            cout << "Login successful.\n";
            menu(id);
        } else {
            cout << "Invalid credentials. Please register.\n";
        }
    } else if (option == 3) {
        cout << "Exiting...\n";
    } else {
        cout << "Invalid choice. Exiting...\n";
    }

    return 0;
}
