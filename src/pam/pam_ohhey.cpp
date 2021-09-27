#include <ostream>
#include <security/pam_appl.h>
#include <security/pam_modules.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <iostream>

#include "src/lib/compare.h"

extern "C" {
/* expected hook */
PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char *argv[]) {
    std::clog << "Setting credentials" << std::endl;
    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char *argv[]) {
    std::clog << "Account mgmt" << std::endl;
    return PAM_SUCCESS;
}

/* expected hook, this is where custom stuff happens */
PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char *argv[]) {
    int retval;
    const char *pUsername;
    retval = pam_get_user(pamh, &pUsername, "Username: ");

    // printf("Authenticating %s\n", pUsername);
    std::clog << "Authenticating " << pUsername << std::endl;

    auto image = "/home/ben/Pictures/my-face.jpg";

    auto result = Compare::run(std::vector<std::string>{"", image});
    switch (result) {
    case Compare::Result::AUTH_SUCCESS:    
        return PAM_SUCCESS;
    case Compare::Result::AUTH_FAIULRE:    
    case Compare::Result::ERROR:    
        return PAM_AUTH_ERR;
    }
}

// PAM_EXTERN int pam_sm_authenticate(pam_handle_t *handle, int flags, int argc, const char **argv){
//     /* In this function we will ask the username and the password with pam_get_user()
//      * and pam_get_authtok(). We will then decide if the user is authenticated */
// }

// PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
//     /* In this function we check that the user is allowed in the system. We already know
//      * that he's authenticated, but we could apply restrictions based on time of the day,
//      * resources in the system etc. */
// }

// PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
//     /* We could have many more information of the user other then password and username.
//      * These are the credentials. For example, a kerberos ticket. Here we establish those
//      * and make them visible to the application */
// }

PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    /* When the application wants to open a session, this function is called. Here we should
     * build the user environment (setting environment variables, mounting directories etc) */
    std::clog << "Open session" << std::endl;

    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_close_session(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    /* Here we destroy the environment we have created above */
    std::clog << "Close session" << std::endl;

    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_chauthtok(pam_handle_t *pamh, int flags, int argc, const char **argv){
    /* This function is called to change the authentication token. Here we should,
     * for example, change the user password with the new password */
    std::clog << "Change auth hook" << std::endl;

    return PAM_SUCCESS;
}

}