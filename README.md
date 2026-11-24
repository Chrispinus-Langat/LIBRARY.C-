# Library Management System (C++)

A console program that manages a library's books, users, loans and due dates. Data is saved to a file, so it survives restarts.

## Classes

- **Book**: title, author, availability status, who borrowed it and the due date.
- **User**: a library user with a name.
- **Library**: stores books and users, and provides adding, removing, searching, borrowing and returning, plus loan reports and saving/loading.

## Operations

| Operation | How it works |
|---|---|
| Add a book | Provide a title and author. Empty input, duplicate titles and the `\|` character are rejected. |
| Remove a book | Specify the title (case-insensitive). A borrowed book cannot be removed. |
| Search | The term is matched against both title and author (case-insensitive). |
| Borrow | The user must be registered. The book must be available. Limit of 3 books per user. Loan period is 14 days. |
| Return | Only the user who borrowed the book can return it. Overdue days are reported. |
| My books | Lists the books a user currently has. |
| Overdue | Lists every book past its due date. |
| Register user | Adds a new library user. |

## Improvements over the basic spec

- Each book remembers who borrowed it, so only that person can return it.
- Due dates and overdue tracking.
- A borrow limit per user.
- Data is saved to `library_data.txt` after every change and loaded at startup.
- Input validation and clear error messages.

## Compilation

```bash
g++ -std=c++11 -Wall -Wextra -o library_management library_management.cpp
```

## Execution

```bash
./library_management          # interactive menu (data saved to library_data.txt)
./library_management --demo   # runs the sample scenario, nothing is saved
```

On Windows, run `library_management.exe` instead.

## Usage

Follow the menu prompts: type the number of an option, then enter the details when asked. Register yourself first (option 9) before borrowing books. Choose `0` to save and exit.

The `--demo` option runs this sample scenario:

1. Adds sample books and registers "John Doe".
2. Searches for "Scott".
3. John Doe borrows "The Great Gatsby" and "To Kill a Mockingbird".
4. John Doe returns "To Kill a Mockingbird".
5. Searches for "Scott" again to show the updated availability.

## Customization

- Change `MAX_LOANS` and `LOAN_DAYS` at the top of the `Library` class to adjust the borrow limit and loan period.
- Edit `runSampleUsage()` to try different scenarios.
- Delete `library_data.txt` to reset the library to empty.
