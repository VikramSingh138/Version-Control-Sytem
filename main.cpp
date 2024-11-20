#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <map>
#include <ctime>
#include <cstdlib>

using namespace std;

// Base User Class
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

    static bool authenticate(const string& id, const string& pass, string& role) {
        ifstream file("users.txt");
        string storedID, storedHash, storedRole;
        string hashedPass = hashPassword(pass);

        while (file >> storedID >> storedHash >> storedRole) {
            if (storedID == id && storedHash == hashedPass) {
                role = storedRole;
                return true;
            }
        }
        return false;
    }

    static void registerUser(const string& id, const string& pass, const string& role) {
        ofstream file("users.txt", ios::app);
        file << id << " " << hashPassword(pass) << " " << role << "\n";
        file.close();
    }

    virtual bool hasElevatedPrivileges() const { return false; }
};

// Derived class for basic users
class BasicUser : public User {
public:
    BasicUser(string id, string pass) : User(id, pass) {}

    void displayRole() const override {
        cout << "Role: Basic User\n";
    }
};

// Derived class for admin users
class AdminUser : public User {
public:
    AdminUser(string id, string pass) : User(id, pass) {}

    void displayRole() const override {
        cout << "Role: Admin User\n";
    }

    bool hasElevatedPrivileges() const override { return true; }

    friend class FileSystem;
};

// Logger Class
class Logger {
public:
    static void logAction(const string& action) {
        ofstream logFile("logs.txt", ios::app);
        auto t = time(nullptr);
        logFile << "- " << ctime(&t) << "  " << action << "\n";
        logFile.close();
    }
};

// FileSystem Class
class FileSystem {
private:
    string currentUserID;
    bool isAdmin;

    bool fileExists(const string& filename) {
        ifstream file(filename);
        return file.good();
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

        destFile << srcFile.rdbuf(); // Copy content from source file to destination file
        srcFile.close();
        destFile.close();

        ofstream versionTracker(filename + "_versions.txt");
        versionTracker << version;
        versionTracker.close();

        Logger::logAction("Saved version " + to_string(version) + " for file: " + filename);
        cout << "Changes saved as version " << version << ".\n";
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

        destFile << srcFile.rdbuf(); // Replace content with the specified version
        srcFile.close();
        destFile.close();

        Logger::logAction("Rolled back " + filename + " to version " + to_string(version));
        cout << "File " << filename << " rolled back to version " << version << " successfully.\n";
    }

    void detectMergeConflicts(const string& baseFile, const string& modifiedFile) {
        ifstream base(baseFile);
        ifstream modified(modifiedFile);

        if (!base.is_open() || !modified.is_open()) {
            cout << "Error: Unable to open files for conflict detection.\n";
            return;
        }

        string baseLine, modifiedLine;
        bool conflict = false;

        cout << "\n-- Conflict Detection --\n";
        while (getline(base, baseLine) && getline(modified, modifiedLine)) {
            if (baseLine != modifiedLine) {
                conflict = true;
                cout << "Conflict detected:\n";
                cout << "Base: " << baseLine << "\nModified: " << modifiedLine << "\n";
            }
        }

        if (conflict) {
            cout << "Merge conflicts found. Review the lines above before merging.\n";
        } else {
            cout << "No merge conflicts detected.\n";
        }

        base.close();
        modified.close();
    }

public:
    FileSystem(const string& user, bool admin = false) : currentUserID(user), isAdmin(admin) {}

    void createFile(const string& filename) {
        if (fileExists(filename)) {
            cout << "File already exists.\n";
            return;
        }
        ofstream file(filename);
        file.close();
        Logger::logAction(currentUserID + " created file: " + filename);
        cout << "File " << filename << " created successfully.\n";
    }

    void modifyFile(const string& filename) {
        if (!fileExists(filename)) {
            cout << "File does not exist.\n";
            return;
        }

        string copyFilename = filename + "_copy.txt";

        ifstream srcFile(filename);
        ofstream copyFile(copyFilename);

        if (!srcFile.is_open() || !copyFile.is_open()) {
            cout << "Error: Unable to create a copy of the file.\n";
            return;
        }

        copyFile << srcFile.rdbuf(); // Create a copy
        srcFile.close();
        copyFile.close();

        Logger::logAction(currentUserID + " created a copy for modification: " + copyFilename);

        cout << "Opened a copy of the file for modification: " << copyFilename << "\n";
        system(("notepad " + copyFilename).c_str()); // Edit the file using notepad or another text editor like 'nano' for Linux

        saveVersion(filename, copyFilename); // Save changes as a new version
    }

    void rollbackFile(const string& filename, int version) {
        if (!isAdmin) {
            cout << "Only admins can perform rollbacks.\n";
            return;
        }

        rollbackToVersion(filename, version);
    }

    void mergeFile(const string& filename, const string& copyFilename) {
        if (!isAdmin) {
            cout << "Only admins can merge changes into the main file.\n";
            return;
        }

        if (!fileExists(copyFilename)) {
            cout << "Error: Copy file does not exist.\n";
            return;
        }

        detectMergeConflicts(filename, copyFilename);

        cout << "Proceed with merge? (yes/no): ";
        string response;
        cin >> response;

        if (response == "yes") {
            ifstream srcFile(copyFilename, ios::binary);
            ofstream destFile(filename, ios::binary);

            if (!srcFile.is_open() || !destFile.is_open()) {
                cout << "Error: Unable to merge file.\n";
                return;
            }

            destFile << srcFile.rdbuf(); // Replace content of main file with copy
            srcFile.close();
            destFile.close();

            Logger::logAction(currentUserID + " merged changes from " + copyFilename + " into " + filename);
            cout << "Changes from " << copyFilename << " merged into " << filename << " successfully.\n";
        } else {
            cout << "Merge operation canceled.\n";
        }
    }
};

void menu(User* user) {
    FileSystem fs(user->getUserID(), user->hasElevatedPrivileges());
    int choice;

    do {
        cout << "\n1. Create File\n2. Modify File\n3. Rollback File (Admin Only)\n4. Merge File (Admin Only)\n5. Logout\nEnter choice: ";
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
        } else if (choice == 3 && user->hasElevatedPrivileges()) {
            string filename;
            int version;
            cout << "Enter filename: ";
            cin >> filename;
            cout << "Enter version to rollback to: ";
            cin >> version;
            fs.rollbackFile(filename, version);
        } else if (choice == 4 && user->hasElevatedPrivileges()) {
            string filename, copyFilename;
            cout << "Enter main filename: ";
            cin >> filename;
            cout << "Enter copy filename to merge: ";
            cin >> copyFilename;
            fs.mergeFile(filename, copyFilename);
        } else if (choice == 5) {
            Logger::logAction(user->getUserID() + " logged out");
            cout << "Logged out successfully.\n";
        } else {
            cout << "Invalid choice or insufficient privileges. Try again.\n";
        }
    } while (choice != 5);
}

int main() {
    string id, pass;
    int option;

    cout << "1. Register Basic User\n2. Register Admin User\n3. Login\n4. Exit\nEnter option: ";
    cin >> option;

    if (option == 1) {
        cout << "Enter UserID: ";
        cin >> id;
        cout << "Enter Password: ";
        cin >> pass;
        User::registerUser(id, pass, "basic");
        cout << "Basic User registered successfully.\n";
    } else if (option == 2) {
        cout << "Enter UserID: ";
        cin >> id;
        cout << "Enter Password: ";
        cin >> pass;
        User::registerUser(id, pass, "admin");
        cout << "Admin User registered successfully.\n";
    } else if (option == 3) {
        cout << "Enter UserID: ";
        cin >> id;
        cout << "Enter Password: ";
        cin >> pass;
        string role;

        if (User::authenticate(id, pass, role)) {
            cout << "Login successful.\n";

            User* user = nullptr;
            if (role == "admin") {
                user = new AdminUser(id, pass);
            } else {
                user = new BasicUser(id, pass);
            }

            user->displayRole();
            menu(user);
            delete user; // Clean up memory
        } else {
            cout << "Authentication failed. Try again.\n";
        }
    }

    return 0;
}
