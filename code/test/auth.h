// Small representation of your Python User/Session logic
struct User {
    char username[20];
    char password_hash[40];
    char role[10];      // admin or user
    char dept[10];      // CS, etc.
};

struct Session {
    char username[20];
    char role[10];
    int startTime;
};