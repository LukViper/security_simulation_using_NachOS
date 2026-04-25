#include "syscall.h"

/* Simple helper to compare strings in C (since we don't have string.h) */
int str_equal(char *s1, char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return (*s1 == *s2);
}

int main() {
    char user[20];
    char pass[20];
    char *admin_user = "admin";
    char *admin_pass = "1234";

    Write("\n--- Nachos Secure Login System ---\n");
    
    Write("Username: ");
    ReadString(user, 20);
    
    Write("Password: ");
    ReadString(pass, 20);

    if (str_equal(user, admin_user) && str_equal(pass, admin_pass)) {
        Write("\nLogin Successful! Role: ADMIN\n");
        Write("Access Granted: [read, write, delete, profile]\n");
        
        /* Simulate an RBAC action */
        Write("\nExecuting: delete_resource(ID: 101)...\n");
        Write("Status: Success.\n");
    } else {
        Write("\nLogin Failed: Invalid credentials.\n");
    }

    Halt();
    return 0;
}