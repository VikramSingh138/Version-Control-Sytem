#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <ctime>
#include <map>

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
        logFile << "[" << ctime(&t) << "] " << action << "\n";
        logFile.close();
    }

    static void logVersion(const string& filename, int version, const string& user) {
        ofstream versionLog("version_history.txt", ios::app);
        auto t = time(nullptr);
        versionLog << "File: " << filename << ", Version: " << version << ", User: " << user << ", Time: " << ctime(&t);
        versionLog.close();
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

    void deleteFile(const string& filename) {
        if (remove(filename.c_str()) == 0) {
            cout << "File deleted successfully.\n";
        } else {
            cout << "Error: Unable to delete the file.\n";
        }
    }

    bool hasPermission(const string& filename) {
        auto fileOwners = loadFileOwners();
        if (fileOwners.find(filename) != fileOwners.end() && fileOwners[filename] == currentUserID) {
            return true; // Current user is the owner
        }
        return false;
    }

    void requestPermission(const string& filename, const string& owner) {
        ofstream requestFile("permission_requests.txt", ios::app);
        requestFile << currentUserID << " requests access to " << filename << " from " << owner << "\n";
        requestFile.close();
        cout << "Permission request sent to " << owner << ".\n";
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

    void saveVersion(const string& filename, int version) {
        string versionFile = filename + "_v" + to_string(version) + ".txt";
        ifstream srcFile(filename, ios::binary);
        ofstream destFile(versionFile, ios::binary);

        if (!srcFile.is_open() || !destFile.is_open()) {
            cout << "Error: Unable to create version file.\n";
            return;
        }

        destFile << srcFile.rdbuf(); // Copy file content
        srcFile.close();
        destFile.close();

        ofstream versionTracker(filename + "_versions.txt");
        versionTracker << version;
        versionTracker.close();
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

    void deleteFileAction(const string& filename) {
        auto fileOwners = loadFileOwners();
        if (fileOwners.find(filename) != fileOwners.end() && fileOwners[filename] == currentUserID) {
            deleteFile(filename);
            Logger::logAction(currentUserID + " deleted file: " + filename);
            cout << "File " << filename << " deleted successfully.\n";
        } else {
            cout << "You do not have permission to delete this file.\n";
            if (fileOwners.find(filename) != fileOwners.end()) {
                requestPermission(filename, fileOwners[filename]);
            }
        }
    }

    void modifyFile(const string& filename) {
        auto fileOwners = loadFileOwners();
        if (fileOwners.find(filename) != fileOwners.end()) {
            if (fileOwners[filename] == currentUserID) {
                int version = getNextVersion(filename);
                saveVersion(filename, version);
                Logger::logVersion(filename, version, currentUserID);

                string line;
                ofstream file(filename, ios::app);
                cout << "Enter content to append (type 'END' to stop): \n";
                cin.ignore();
                while (getline(cin, line) && line != "END") {
                    file << line << "\n";
                }
                file.close();
                Logger::logAction(currentUserID + " modified file: " + filename);
                cout << "File " << filename << " modified successfully.\n";
            } else {
                cout << "You do not have permission to modify this file.\n";
                requestPermission(filename, fileOwners[filename]);
            }
        } else {
            cout << "File does not exist.\n";
        }
    }

    void rollbackFile(const string& filename, int version) {
        string versionFile = filename + "_v" + to_string(version) + ".txt";
        auto fileOwners = loadFileOwners();

        if (fileOwners.find(filename) != fileOwners.end() && fileOwners[filename] == currentUserID) {
            if (fileExists(versionFile)) {
                ifstream srcFile(versionFile, ios::binary);
                ofstream destFile(filename, ios::binary);

                if (!srcFile.is_open() || !destFile.is_open()) {
                    cout << "Error: Unable to rollback file.\n";
                    return;
                }

                destFile << srcFile.rdbuf(); // Copy file content
                srcFile.close();
                destFile.close();

                Logger::logAction(currentUserID + " rolled back file: " + filename + " to version: " + to_string(version));
                cout << "File " << filename << " rolled back to version " << version << " successfully.\n";
            } else {
                cout << "Specified version does not exist.\n";
            }
        } else {
            cout << "You do not have permission to rollback this file.\n";
        }
    }
};

void menu(const string& userID) {
    FileSystem fs(userID);
    int choice;

    do {
        cout << "\n1. Create File\n2. Delete File\n3. Modify File\n4. Rollback File\n5. Logout\nEnter choice: ";
        cin >> choice;

        if (choice == 1) {
            string filename;
            cout << "Enter filename: ";
            cin >> filename;
            fs.createFile(filename);
        } else if (choice == 2) {
            string filename;
            cout << "Enter filename to delete: ";
            cin >> filename;
            fs.deleteFileAction(filename);
        } else if (choice == 3) {
            string filename;
            cout << "Enter filename to modify: ";
            cin >> filename;
            fs.modifyFile(filename);
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
