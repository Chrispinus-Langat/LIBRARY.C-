// Library Management System
// Compile: g++ -std=c++11 -Wall -Wextra -o library_management library_management.cpp
// Run:     ./library_management          (interactive menu, data saved to library_data.txt)
//          ./library_management --demo   (runs the sample scenario, nothing is saved)

#include <algorithm>
#include <cctype>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// ---------- helpers ----------
static string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(),
              [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return s;
}

static string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static vector<string> split(const string& s, char sep) {
    vector<string> parts;
    string item;
    stringstream ss(s);
    while (getline(ss, item, sep)) parts.push_back(item);
    if (!s.empty() && s.back() == sep) parts.push_back("");
    return parts;
}

static string formatDate(time_t t) {
    char buf[32];
    strftime(buf, sizeof buf, "%d %b %Y", localtime(&t));
    return buf;
}

static string readLine(const string& prompt) {
    cout << prompt;
    string line;
    getline(cin, line);
    return trim(line);
}

// ---------- Book ----------
class Book {
public:
    Book(const string& title, const string& author,
         const string& borrower = "", time_t due = 0)
        : title_(title), author_(author), borrower_(borrower), due_(due) {}

    const string& title() const { return title_; }
    const string& author() const { return author_; }
    const string& borrower() const { return borrower_; }
    time_t dueDate() const { return due_; }
    bool isAvailable() const { return borrower_.empty(); }
    bool isOverdue(time_t now) const { return !isAvailable() && due_ < now; }

    void lend(const string& who, time_t due) { borrower_ = who; due_ = due; }
    void giveBack() { borrower_.clear(); due_ = 0; }

    void print() const {
        cout << "  - \"" << title_ << "\" by " << author_ << " [";
        if (isAvailable()) cout << "Available";
        else cout << "Borrowed by " << borrower_ << ", due " << formatDate(due_);
        cout << "]\n";
    }

private:
    string title_, author_, borrower_;
    time_t due_;
};

// ---------- User ----------
class User {
public:
    explicit User(const string& name) : name_(name) {}
    const string& name() const { return name_; }

private:
    string name_;
};

// ---------- Library ----------
class Library {
public:
    static const int MAX_LOANS = 3;
    static const int LOAN_DAYS = 14;

    // Pass a file name to enable saving/loading; leave empty for in-memory only.
    explicit Library(const string& file = "") : file_(file) {
        if (!file_.empty()) load();
    }

    bool addBook(const string& title, const string& author) {
        if (!validText(title) || !validText(author)) return false;
        if (findBook(title)) {
            cout << "\"" << title << "\" is already in the library.\n";
            return false;
        }
        books_.push_back(Book(title, author));
        cout << "Added \"" << title << "\" by " << author << ".\n";
        return true;
    }

    bool removeBook(const string& title) {
        for (size_t i = 0; i < books_.size(); ++i) {
            if (toLower(books_[i].title()) != toLower(title)) continue;
            if (!books_[i].isAvailable()) {
                cout << "Cannot remove \"" << books_[i].title()
                     << "\": it is currently borrowed by " << books_[i].borrower() << ".\n";
                return false;
            }
            cout << "Removed \"" << books_[i].title() << "\".\n";
            books_.erase(books_.begin() + i);
            return true;
        }
        cout << "No book titled \"" << title << "\" found.\n";
        return false;
    }

    // The term is matched against both title and author (case-insensitive).
    void searchBooks(const string& term) const {
        string t = toLower(term);
        bool found = false;
        cout << "Search results for \"" << term << "\":\n";
        for (const Book& b : books_) {
            if (toLower(b.title()).find(t) != string::npos ||
                toLower(b.author()).find(t) != string::npos) {
                b.print();
                found = true;
            }
        }
        if (!found) cout << "  No matching books.\n";
    }

    bool registerUser(const string& name) {
        if (!validText(name)) return false;
        if (findUser(name)) {
            cout << name << " is already registered.\n";
            return false;
        }
        users_.push_back(User(name));
        cout << "Registered " << name << ".\n";
        return true;
    }

    bool borrowBook(const string& title, const string& userName) {
        const User* u = findUser(userName);
        if (!u) {
            cout << "User \"" << userName << "\" is not registered. Register first.\n";
            return false;
        }
        Book* b = findBook(title);
        if (!b) {
            cout << "No book titled \"" << title << "\" found.\n";
            return false;
        }
        if (!b->isAvailable()) {
            cout << "Sorry, \"" << b->title() << "\" is already borrowed (due back "
                 << formatDate(b->dueDate()) << ").\n";
            return false;
        }
        if (loanCount(u->name()) >= MAX_LOANS) {
            cout << u->name() << " has reached the limit of " << MAX_LOANS
                 << " borrowed books. Return one first.\n";
            return false;
        }
        time_t due = time(nullptr) + static_cast<time_t>(LOAN_DAYS) * 24 * 60 * 60;
        b->lend(u->name(), due);
        cout << u->name() << " borrowed \"" << b->title() << "\". Due back "
             << formatDate(due) << ".\n";
        return true;
    }

    bool returnBook(const string& title, const string& userName) {
        Book* b = findBook(title);
        if (!b) {
            cout << "No book titled \"" << title << "\" found.\n";
            return false;
        }
        if (b->isAvailable()) {
            cout << "\"" << b->title() << "\" was not borrowed.\n";
            return false;
        }
        if (toLower(b->borrower()) != toLower(userName)) {
            cout << "\"" << b->title() << "\" was borrowed by " << b->borrower()
                 << ", not " << userName << ".\n";
            return false;
        }
        time_t now = time(nullptr);
        if (b->isOverdue(now)) {
            long days = static_cast<long>((now - b->dueDate()) / (24 * 60 * 60));
            if (days > 0) cout << "Note: this book was " << days << " day(s) overdue.\n";
        }
        cout << b->borrower() << " returned \"" << b->title() << "\".\n";
        b->giveBack();
        return true;
    }

    void listBooks() const {
        cout << "All books (" << books_.size() << "):\n";
        if (books_.empty()) cout << "  (library is empty)\n";
        for (const Book& b : books_) b.print();
    }

    void listLoans(const string& userName) const {
        const User* u = findUser(userName);
        if (!u) {
            cout << "User \"" << userName << "\" is not registered.\n";
            return;
        }
        cout << "Books borrowed by " << u->name() << ":\n";
        bool any = false;
        for (const Book& b : books_) {
            if (toLower(b.borrower()) == toLower(u->name())) { b.print(); any = true; }
        }
        if (!any) cout << "  (none)\n";
    }

    void listOverdue() const {
        time_t now = time(nullptr);
        bool any = false;
        cout << "Overdue books:\n";
        for (const Book& b : books_) {
            if (b.isOverdue(now)) { b.print(); any = true; }
        }
        if (!any) cout << "  (none)\n";
    }

    bool save() const {
        if (file_.empty()) return true;
        ofstream out(file_.c_str());
        if (!out) {
            cout << "Warning: could not save to " << file_ << ".\n";
            return false;
        }
        for (const User& u : users_) out << "U|" << u.name() << "\n";
        for (const Book& b : books_)
            out << "B|" << b.title() << "|" << b.author() << "|" << b.borrower()
                << "|" << static_cast<long long>(b.dueDate()) << "\n";
        return true;
    }

private:
    vector<Book> books_;
    vector<User> users_;
    string file_;

    // '|' is used as the field separator in the save file.
    static bool validText(const string& s) {
        if (s.empty()) {
            cout << "Input cannot be empty.\n";
            return false;
        }
        if (s.find('|') != string::npos) {
            cout << "The '|' character is not allowed.\n";
            return false;
        }
        return true;
    }

    Book* findBook(const string& title) {
        for (Book& b : books_)
            if (toLower(b.title()) == toLower(title)) return &b;
        return nullptr;
    }

    const User* findUser(const string& name) const {
        for (const User& u : users_)
            if (toLower(u.name()) == toLower(name)) return &u;
        return nullptr;
    }

    int loanCount(const string& name) const {
        int n = 0;
        for (const Book& b : books_)
            if (toLower(b.borrower()) == toLower(name)) ++n;
        return n;
    }

    void load() {
        ifstream in(file_.c_str());
        if (!in) return;  // first run: no file yet
        string line;
        while (getline(in, line)) {
            vector<string> p = split(line, '|');
            if (p.size() == 2 && p[0] == "U") {
                users_.push_back(User(p[1]));
            } else if (p.size() == 5 && p[0] == "B") {
                time_t due = static_cast<time_t>(atoll(p[4].c_str()));
                books_.push_back(Book(p[1], p[2], p[3], due));
            }
        }
    }
};

// ---------- demo + menu ----------
static void runSampleUsage() {
    Library lib;  // in-memory only
    cout << "===== Sample usage =====\n";
    lib.addBook("The Great Gatsby", "F. Scott Fitzgerald");
    lib.addBook("To Kill a Mockingbird", "Harper Lee");
    lib.addBook("1984", "George Orwell");
    lib.addBook("Tender Is the Night", "F. Scott Fitzgerald");
    lib.registerUser("John Doe");

    cout << "\n";
    lib.searchBooks("Scott");
    cout << "\n";
    lib.borrowBook("The Great Gatsby", "John Doe");
    lib.borrowBook("To Kill a Mockingbird", "John Doe");
    cout << "\n";
    lib.returnBook("To Kill a Mockingbird", "John Doe");
    cout << "\n";
    lib.searchBooks("Scott");
    cout << "========================\n";
}

static void printMenu() {
    cout << "\n--- Library Menu ---\n"
         << "1. Add a book\n"
         << "2. Remove a book\n"
         << "3. Search books\n"
         << "4. Borrow a book\n"
         << "5. Return a book\n"
         << "6. List all books\n"
         << "7. Show a user's borrowed books\n"
         << "8. Show overdue books\n"
         << "9. Register a user\n"
         << "0. Save and exit\n";
}

int main(int argc, char* argv[]) {
    if (argc > 1 && string(argv[1]) == "--demo") {
        runSampleUsage();
        return 0;
    }

    Library lib("library_data.txt");
    cout << "Welcome to the Library Management System.\n";

    while (true) {
        printMenu();
        string choice = readLine("Choose an option: ");
        if (!cin) { lib.save(); break; }  // end of input

        bool changed = false;
        if (choice == "0") {
            lib.save();
            cout << "Data saved. Goodbye!\n";
            break;
        } else if (choice == "1") {
            string t = readLine("Title: ");
            string a = readLine("Author: ");
            changed = lib.addBook(t, a);
        } else if (choice == "2") {
            changed = lib.removeBook(readLine("Title to remove: "));
        } else if (choice == "3") {
            lib.searchBooks(readLine("Search term: "));
        } else if (choice == "4") {
            string u = readLine("Your name: ");
            string t = readLine("Title to borrow: ");
            changed = lib.borrowBook(t, u);
        } else if (choice == "5") {
            string u = readLine("Your name: ");
            string t = readLine("Title to return: ");
            changed = lib.returnBook(t, u);
        } else if (choice == "6") {
            lib.listBooks();
        } else if (choice == "7") {
            lib.listLoans(readLine("User name: "));
        } else if (choice == "8") {
            lib.listOverdue();
        } else if (choice == "9") {
            changed = lib.registerUser(readLine("New user name: "));
        } else {
            cout << "Invalid option, please enter a number from the menu.\n";
        }
        if (changed) lib.save();
    }
    return 0;
}