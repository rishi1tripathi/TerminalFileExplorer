#include <iostream>
#include <iomanip>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <stack>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
using namespace std;
#define pos printf("%c[%d;%dH", 27, cx, cy)


// Function declaration
void listHelper(const char *path);
string getParent(string path);
void displayListHelper();
void display(const char *dirName);
void printLastLine(string text);
void setNCanonicalMode();
void downArrow();
void upArrow();
void rightArrow();
void leftArrow();
void backSpaceKey();
void homeKey();
void pageDown();
void pageDown();
void pageUp();
void enterKey();
int isFileExist(string path);
void winsz_handler(int sig);
void createDirectory();
void createFile();
void deleteFile();
void deleteDirectory();
void deleteDirectoryHelper(string path);
int gotoDirectory();
void search();
bool searchHelper(string path);
void copyDirectoryHelper(string fromPath, string toPath);
void copyPermission(string from, string to);
void copy();
void copySingleFile();
void rename();
void move();
int callFunction();
void clearStack(stack<string> &s);
bool isDirectory(string);
int commandMode();
void resetCursor();
void clearLastLine();

// Varibale declaration global
vector<string> inputWords;
char root[4096];
char curPath[4096];
int searchflag;
int maxRow = 0;
vector<string> itemList;
stack<string> backStack;
stack<string> forwardStack;
int allowableRows, colsize;
int totalFiles;
int cx, cy, start = 1;

void clearStack(stack<string> &st)
{
    while (!st.empty())
        st.pop();
}
string createAbsolutePath(string relativePath)
{
    string absolutePath = "";
    if (relativePath[0] == '~' || relativePath[0] == '/')
    {
        if (relativePath[0] == '~')
            relativePath = relativePath.substr(2, relativePath.length());
        else
            relativePath = relativePath.substr(1, relativePath.length());
        absolutePath = string(root) + "/" + relativePath;
    }
    else if (relativePath[0] == '.' && relativePath[1] == '/')
    {
        absolutePath = string(curPath) + "/" + relativePath.substr(2, relativePath.length());
    }
    else
    {
        absolutePath = string(curPath) + "/" + relativePath;
    }
    return absolutePath;
}
string getParent(string path)
{
    return path.substr(0, path.find_last_of("/\\"));
}

bool isDirectory(string path)
{
    struct stat sb;
    if (stat(path.c_str(), &sb) < 0)
    {
        perror(path.c_str());
        return false;
    }
    if (S_ISDIR(sb.st_mode))
        return true;
    else
        return false;
}

int isFileExist(string path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

void resetCursor()
{
    cy = 1;
    cx = 1;
    start = 1;
    pos;
}

void setNCanonicalMode()
{
    struct termios raw, newraw;
    tcgetattr(STDIN_FILENO, &raw);
    newraw = raw;
    //changing ICANON for entering Non Canonical mode.
    newraw.c_lflag &= ~(ICANON | ECHO | IEXTEN | ISIG);
    newraw.c_iflag &= ~(BRKINT);
    //set new terminal settings.
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &newraw) != 0)
        fprintf(stderr, "Could not set attributes\n");
    else
    {
        char ch;
        while (1)
        {
            signal(SIGWINCH, winsz_handler);
            printf("%c[%d;%dH", 27, maxRow, 1);
            printf("--Normal Mode----");
            pos;
            fflush(0);
            ch = cin.get();
            if (ch == 27)
            {
                ch = cin.get();
                ch = cin.get();
                if (ch == 'A')
                    upArrow();
                else if (ch == 'B')
                    downArrow();
                else if (ch == 'D')
                    leftArrow();
                else if (ch == 'C')
                    rightArrow();
            }
            else if (ch == 'k')
                pageUp();
            else if (ch == 'l')
                pageDown();
            else if (ch == 10)
                enterKey();
            else if (ch == 'H' || ch == 'h')
                homeKey();
            else if (ch == 127)
                backSpaceKey();
            else if (ch == ':')
            {
                int ret = commandMode();
                cx = 1;
                cy = 1;
                pos;
                listHelper(curPath);
            }
            else if (ch == 'q')
            {
                write(STDOUT_FILENO, "\x1b[2J", 4);
                tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
                cx = 1;
                cy = 1;
                pos;
                exit(1);
            }
            fflush(0);
        }
    }
}

/*============================================================
Move cursot one step upward.Also can scroll the list
=============================================================*/
void upArrow()
{
    if (start + cx - 1 > 1)
    {
        if (cx > 1)
        {
            cx--;
            pos;
        }
        else
        {
            start--;
            displayListHelper();
            pos;
        }
    }
}
/*============================================================
Move cursot one step downward.Also can scroll the list
=============================================================*/
void downArrow()
{
    if (start + cx - 1 < itemList.size() - 1)
    {
        if (cx < allowableRows)
        {
            cx++;
            pos;
        }
        else
        {
            start++;
            displayListHelper();
            pos;
        }
    }
}
/*============================================================
Move cursot to one page above if it exist. Used in scrolling
=============================================================*/
void pageUp()
{
    if (start > 1)
    {
        if (start - allowableRows >= 1)
            start = start - allowableRows;
        else
            start = 1;
        cx = 1;
        displayListHelper();
        pos;
    }
}
/*============================================================
Move cursot to one page down if it exist. Used in scrolling
=============================================================*/
void pageDown()
{
    if (start + allowableRows - 1 < itemList.size() - 1)
    {
        start = start + allowableRows;
        cx = 1;
        displayListHelper();
        pos;
    }
}

/*============================================================
On directory it will go inside directory. else open in vi
=============================================================*/
void enterKey()
{
    if (itemList[start + cx - 1] == ".")
    {
        return;
    }
    else if (itemList[start + cx - 1] == "..")
    {
        // if selected directory is '..' .
        string temp = getParent(string(curPath));
        backStack.push(curPath);
        strcpy(curPath, temp.c_str());

        clearStack(forwardStack);
        resetCursor();
        listHelper(curPath);
    }

    else
    {
        string nextPath = string(curPath) + "/" + itemList[start + cx - 1];
        clearStack(forwardStack);
        if (!isDirectory(nextPath))
        {
            pid_t pid = fork();
            if (pid == 0)
            {
                close(2);
                execlp("xdg-open", "xdg-open", nextPath.c_str(), NULL);
                exit(0);
            }
        }
        else
        {
            backStack.push(string(curPath));
            strcpy(curPath, nextPath.c_str());
            resetCursor();
            listHelper(curPath);
        }
    }
}
/*============================================================
Move to folder where application is started
=============================================================*/
void homeKey()
{
    if (string(curPath) != string(root))
    {
        clearStack(forwardStack);
        strcpy(curPath, root);
        resetCursor();
        listHelper(curPath);
    }
}
/*============================================================
Move to parent folder if exist
=============================================================*/
void backSpaceKey()
{
    if (string(curPath) != string(root))
    {
        backStack.pop();
        string temp = getParent(string(curPath));
        strcpy(curPath, temp.c_str());
        clearStack(forwardStack);
        resetCursor();
        listHelper(curPath);
    }
}
/*============================================================
Move to folder pointed by top item of forw_stack
=============================================================*/

void rightArrow()
{
    if (!forwardStack.empty())
    {
        backStack.push(curPath);
        string nextPath = forwardStack.top();
        forwardStack.pop();
        strcpy(curPath, nextPath.c_str());
        resetCursor();
        listHelper(curPath);
    }
}
/*============================================================
Move to folder pointed by top item of back_stack
=============================================================*/
void leftArrow()
{
    if (!backStack.empty())
    {
        forwardStack.push(string(curPath));
        string nextPath = backStack.top();
        backStack.pop();
        strcpy(curPath, nextPath.c_str());
        resetCursor();
        listHelper(curPath);
    }
}

int main(int argc, char *argv[])
{
    if (argc == 1)
        getcwd(root, sizeof(root)); //set root as current working dir.
    else
    {
        cout << "Wrong arguments"<< endl;
        exit(0);
    }
    strcpy(curPath, root);
    listHelper(root); 
    setNCanonicalMode();
    return 0;
}
void winsz_handler(int sig)
{
    listHelper(curPath);
    fflush(0);
    cx = 1;
}
void listHelper(const char *path)
{

    struct winsize win;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);
    maxRow = win.ws_row;
    allowableRows = win.ws_row - 1;
    colsize = win.ws_col;
    DIR *d;
    d = opendir(path);
    if (d == NULL)
    {
        perror("opendir");
        return;
    }
    itemList.clear();
    struct dirent *dirIndex;
    itemList.push_back("dsadas");
    while ((dirIndex = readdir(d)))
    {
        if ((strcmp(path, root) != 0) || (string(dirIndex->d_name) != ".."))
            itemList.push_back(string(dirIndex->d_name));
    }
    sort(itemList.begin() + 1, itemList.end());
    cx = 1;
    displayListHelper();
    cx = 1;
    pos;
    closedir(d);
    return;
}

void displayListHelper()
{
    printf("\033[H\033[J");
    cy = 1;
    pos;
    int to;
    if ((unsigned)itemList.size() - start > allowableRows)
        to = start + allowableRows - 1;
    else
        to = itemList.size() - 1;
    for (int i = start; i <= to; i++)
    {
        string t = itemList[i];
        display(t.c_str());
    };
}

void display(const char *dirName)
{
    cy = 0;
    struct stat sb;
    string absolutePath = createAbsolutePath(dirName);
    stat(absolutePath.c_str(), &sb);
    cy+=printf((S_ISDIR(sb.st_mode))  ? "d" : "-");
    cy+=printf((sb.st_mode & S_IRUSR) ? "r" : "-");
    cy+=printf((sb.st_mode & S_IWUSR) ? "w" : "-");
    cy+=printf((sb.st_mode & S_IXUSR) ? "x" : "-");
    cy+=printf((sb.st_mode & S_IRGRP) ? "r" : "-");
    cy+=printf((sb.st_mode & S_IWGRP) ? "w" : "-");
    cy+=printf((sb.st_mode & S_IXGRP) ? "x" : "-");
    cy+=printf((sb.st_mode & S_IROTH) ? "r" : "-");
    cy+=printf((sb.st_mode & S_IWOTH) ? "w" : "-");
    cy+=printf((sb.st_mode & S_IXOTH) ? "x" : "-");
    cy += 1;

    struct passwd *userName;
    userName = getpwuid(sb.st_uid);
    string uname = userName->pw_name;
    cy += printf(" %10s ", uname.c_str());

    struct group *getGroupName;
    getGroupName = getgrgid(sb.st_gid);
    string gname = getGroupName->gr_name;
    cy += printf(" %10s ", gname.c_str());

    long long fileSize = sb.st_size;
    if (fileSize >= (1 << 30))
        cy += printf("%4lldG ", fileSize / (1 << 30));
    else if (fileSize >= (1 << 20))
        cy += printf("%4lldM ", fileSize / (1 << 20));
    else if (fileSize >= (1 << 10))
        cy += printf("%4lldK ", fileSize / (1 << 10));
    else
        cy += printf("%4lldB ", fileSize);

    string modifiedTime = string(ctime(&sb.st_mtime));
    modifiedTime = modifiedTime.substr(4, 12);
    cy += printf(" %-12s ", modifiedTime.c_str());
    // Color Greeen if Directory
    if (isDirectory(absolutePath))
        printf("%c[32m", 27);
    printf(" %-20s\n", dirName);
    // Revert color To white
    printf("%c[0m", 27);
    cy++;
}

int commandMode()

{
    int storeCy = cy;
    int storeCX = cx;
    cx = maxRow;

    cy = 1;
    pos;
    printf("\x1b[0K");
    printf(":");
    fflush(0);
    char ch;
    string input;
    while (true)
    {
        ch = cin.get();
        if (ch == 27)
        {
            printf("%c[%d;%dH", 27, maxRow, 1);
            printf("%c[2K", 27);
            cx = storeCX;
            cy = storeCy;
            return 0;
        }
        else if (ch != 10)
        {
            if (ch != 127)
            {
                input = input + ch;
                cout << ch;
            }
            else
            {
                printf("%c[%d;%dH", 27, maxRow, 1);
                printf("%c[2K", 27);
                cy += printf(":");
                if (input.length() <= 1)
                    input = "";
                else
                    input = input.substr(0, input.length() - 1);
                cout << input;
            }
        }
        else
        {
            inputWords.clear();
            string temp = "";
            for (int i = 0; i < input.length(); i++)
            {
                if (input[i] == ' ')
                {
                    if (temp != "")
                        inputWords.push_back(temp);
                    temp = "";
                    continue;
                }
                else
                {
                    temp += input[i];
                }
            }
            inputWords.push_back(temp);
            int retValue = callFunction();
            if (retValue == 0)
            {
                printf("%c[%d;%dH", 27, maxRow, 1);
                printf("%c[2K", 27);
                cx = 1;
                cy = 1;
                return 0;
            }
            // printf("%c[%d;%dH", 27, maxRow, 1);
            // printf("%c[2K", 27);
            // cy += printf(":");
            input = "";
        }
    }
    return -1;
}
/*============================================================
Call various modes based on input
=============================================================*/
int callFunction()
{
    string firstWord = inputWords[0];
    if (firstWord == "create_dir")
        createDirectory();
    else if (firstWord == "create_file")
        createFile();
    else if (firstWord == "delete_file")
        deleteFile();
    else if (firstWord == "delete_dir")
        deleteDirectory();
    else if (firstWord == "copy")
        copy();
    else if (firstWord == "goto")
    {
        return gotoDirectory();
    }
    else if (firstWord == "search")
        search();
    else if (firstWord == "move")
        move();
    else if (firstWord == "rename")
        rename();

    else
        printLastLine("Command not found.");
    return -1;
}
/*============================================================
Clear last line in command mode
=============================================================*/
void clearLastLine()
{
    cx = maxRow;
    cy = 1;
    pos;
    printf("\x1b[0K");
}
void printLastLine(string text)
{
    clearLastLine();
    cout << ":" << text << endl;
    clearLastLine();
    cout << ":";
}
/*============================================================
create file at <Destination path>
eg : create_file <f1> <Destination Path>
=============================================================*/
void createFile()
{
    if (inputWords.size() != 3)
    {
        printLastLine("Bad arguments.");
        return;
    }
    string destinationDirectory = createAbsolutePath(inputWords[2]);
    if (!isDirectory(destinationDirectory))
    {
        printLastLine("Destination folder is not a directory.");
        return;
    }
    string newFIlePath = destinationDirectory + "/" + inputWords[1];
    FILE *fileCreate = fopen(newFIlePath.c_str(), "w+");
    if (fileCreate == NULL)
        perror("");
    else
        printLastLine("File created successfuly.");
    fclose(fileCreate);
}

/*============================================================
create directory at Destination path
eg : create_dir <d1>  <Destination path>
=============================================================*/

void createDirectory()
{
    if (inputWords.size() != 3)
    {
        printLastLine("Bad arguments.");
        return;
    }
    string destinationDirectory = createAbsolutePath(inputWords[2]);
    if (!isDirectory(destinationDirectory))
    {
        printLastLine("Destination folder is not a directory.");
        return;
    }
    string newFolderPath = destinationDirectory + "/" + inputWords[1];
    if (mkdir(newFolderPath.c_str(), 0755) != 0)
        perror("Cannot create new directory.");
    else
        printLastLine("Directory created successfully.");
}
/*============================================================
Delete file
eg : delete_file <f1> 
=============================================================*/
void deleteFile()
{
    if (inputWords.size() != 2)
    {
        printLastLine("Bad arguments.");
        return;
    }
    string filePath = createAbsolutePath(inputWords[1]);
    if (!isFileExist(filePath))
    {
        printLastLine("File does not exist.");
        return;
    }
    if (remove(filePath.c_str()) != 0)
        perror("");
    else
        printLastLine(" Successfully Deleted");
}
/*============================================================
Delete directory
eg : delete_dir <f1> 
=============================================================*/
void deleteDirectory()
{
    if (inputWords.size() != 2)
    {
        printLastLine("Bad arguments.");
        return;
    }
    string directoryPath = createAbsolutePath(inputWords[1]);
    if (!isDirectory(directoryPath))
    {
        printLastLine("Directory does not exist.");
        return;
    }
    deleteDirectoryHelper(directoryPath);
    printLastLine("Directory deleted successfully.");
}
/*============================================================
Recursive function to delete directory
=============================================================*/
void deleteDirectoryHelper(string path)
{
    DIR *dir;
    dir = opendir(path.c_str());
    struct dirent *d;
    if (dir == NULL)
    {
        perror("error in opening");
        return;
    }
    while ((d = readdir(dir)))
    {
        if (strcmp(d->d_name, "..") != 0 && strcmp(d->d_name, ".") != 0)
        {
            string newPath = path + "/" + string(d->d_name);
            if (isDirectory(newPath))
            {
                deleteDirectoryHelper(newPath);
            }
            else
            {
                remove(newPath.c_str());
            }
        }
    }
    rmdir(path.c_str());
    closedir(dir);
    return;
}
/*============================================================
Copy Single File Helper
=============================================================*/
void copySingleFile(string from, string to)
{
    char block[1024];
    int in = open(from.c_str(), O_RDONLY);
    int out = open(to.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    int nread;
    while ((nread = read(in, block, sizeof(block))) > 0)
        write(out, block, nread);
    copyPermission(from, to);
    close(in);
    close(out);
    return;
}

void copyPermission(string from, string to)
{
    struct stat from_stat;
    stat(from.c_str(), &from_stat);
    chown(to.c_str(), from_stat.st_uid, from_stat.st_gid);
    chmod(to.c_str(), from_stat.st_mode);
}

void copyDirectoryHelper(string fromPath, string toPath)
{
    DIR *dir = opendir(fromPath.c_str());
    if (dir == NULL)
    {
        perror("Error in opening file");
        return;
    }
    struct dirent *d;
    while ((d = readdir(dir)))
    {
        if ((strcmp(d->d_name, ".") != 0) && (strcmp(d->d_name, "..") != 0))
        {
            string newFromPath = fromPath + "/" + string(d->d_name);
            string newToPath = toPath + "/" + string(d->d_name);
            if (!isDirectory(newFromPath))
            {
                copySingleFile(newFromPath, newToPath);
            }
            else
            {
                if (mkdir(newToPath.c_str(), 0755) != 0)
                {
                    perror("");
                    return;
                }
                else
                {
                    copyPermission(newFromPath, newToPath);
                    copyDirectoryHelper(newFromPath, newToPath);
                }
            }
        }
    }
    closedir(dir);
}
/*============================================================
Copy files or directory 
eg : copy <f1> <<f2> <d1> <Destination path> 
=============================================================*/
void copy()
{
    if (inputWords.size() < 3)
    {
        printLastLine("Arguments size less.");
        return;
    }

    string destinationFolder = createAbsolutePath(inputWords[inputWords.size() - 1]);
    if (!isDirectory(destinationFolder))
    {
        printLastLine("Destination path is not a folder.");
        return;
    }
    int len = inputWords.size() - 2;
    for (int i = 1; i <= len; i++)
    {
        string fromPath = createAbsolutePath(inputWords[i]);
        size_t found = fromPath.find_last_of("/\\");
        string toPath = destinationFolder + fromPath.substr(found, fromPath.length());
        if (!isDirectory(fromPath))
        {
            copySingleFile(fromPath, toPath);
            printLastLine("Copied Successful");
        }
        else
        {

            if (mkdir(toPath.c_str(), 0755) != 0)
            {
                perror("");
            }
            copyPermission(fromPath, toPath);
            copyDirectoryHelper(fromPath, toPath);
            printLastLine("Copied Successful");
        }
    }
}
/*============================================================
Goto directory specified
eg : goto <dir>
=============================================================*/
int gotoDirectory()
{
    if (inputWords.size() != 2)
    {
        printLastLine("Bad arguments.");
        return -1;
    }
    string nextPath = createAbsolutePath(inputWords[1]);
    if (!isDirectory(nextPath))
    {
        printLastLine("Invalid path");
        return -1;
    }
    backStack.push(curPath);
    strcpy(curPath, nextPath.c_str());
    return 0;
} /*============================================================
Search if File or Folder is Present
eg : goto <dir>
=============================================================*/
void search()
{
    if (inputWords.size() != 2)
    {
        printLastLine("Bad arguments.");
        return;
    }
    bool found = searchHelper((string)curPath);
    if (found)
        printLastLine("True");
    else
        printLastLine("False");
}

bool searchHelper(string path)
{
    DIR *dir;
    dir = opendir(path.c_str());
    if (dir == NULL)
    {
        perror("opendir");
        return false;
    }
    struct dirent *d;
    while ((d = readdir(dir)))
    {
        if (strcmp(d->d_name, ".") != 0 && strcmp(d->d_name, "..") != 0)
        {
            if (strcmp(d->d_name, inputWords[1].c_str()) == 0)
                return true;
            string nextPath = path + "/" + string(d->d_name);
            if (isDirectory(nextPath))
            {
                if (searchHelper(nextPath))
                    return true;
            }
        }
    }
    closedir(dir);
    return false;
}
/*============================================================
Move files or directory to destination path
eg : move <f1> <f2> <Destination path> 
=============================================================*/
void move()
{

    if (inputWords.size() < 3)
    {
        printLastLine("Arguments size less.");
        return;
    }

    string destinationFolder = createAbsolutePath(inputWords[inputWords.size() - 1]);
    if (!isDirectory(destinationFolder))
    {
        printLastLine("Destination path is not a folder.");
        return;
    }
    int len = inputWords.size() - 2;
    for (int i = 1; i <= len; i++)
    {
        string fromPath = createAbsolutePath(inputWords[i]);
        size_t found = fromPath.find_last_of("/\\");
        string toPath = destinationFolder + fromPath.substr(found, fromPath.length());
        if (!isDirectory(fromPath))
        {
            copySingleFile(fromPath, toPath);
            if (remove(fromPath.c_str()) == 0)
                ;
            printLastLine("Move Successful");
        }
        else
        {
            if (mkdir(toPath.c_str(), 0755) != 0)
            {
                perror("");
            }
            copyDirectoryHelper(fromPath, toPath);
            deleteDirectoryHelper(fromPath);
            printLastLine("Move Successful");
        }
    }
}
/*============================================================
Rename files
eg : move <source-file or directory> <Changed name> 
=============================================================*/

void rename()
{
    if (inputWords.size() != 3)
    {
        printLastLine("Bad arguments.");
        return;
    }
    string oldName = createAbsolutePath(inputWords[1]);
    string newName_ = createAbsolutePath(inputWords[2]);
    if (rename(oldName.c_str(), newName_.c_str()) == -1)
        perror("");
    else
        printLastLine("Rename successful");
}

