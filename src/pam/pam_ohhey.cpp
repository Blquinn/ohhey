#include <iostream>
#include <ostream>
#include <security/pam_appl.h>
#include <security/pam_modules.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "src/lib/compare.h"

// Note: We can store global state here that should persist through the user's session.

int auth(std::string username) {
    // std::clog << "Authenticating " << username << std::endl;

    auto result = Compare::run(std::vector<std::string>{"", username});
    switch (result) {
    case Compare::Result::AUTH_SUCCESS:
        return PAM_SUCCESS;
    case Compare::Result::AUTH_FAIULRE:
    case Compare::Result::ERROR:
    default:
        return PAM_AUTH_ERR;
    }
}

int getUserAndAuth(pam_handle_t *pamh, int flags, int argc, const char *argv[]) {
    int retval;
    const char *pUsername;
    retval = pam_get_user(pamh, &pUsername, "Username: ");
    if (retval != PAM_SUCCESS)
        return retval;

    return auth(pUsername);
}

/* Begin hooks */

extern "C" {
/* expected hook, this is where custom stuff happens */
PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char *argv[]) {
    // std::clog << "Running ohhey auth." << std::endl;

    return getUserAndAuth(pamh, flags, argc, argv);
}

PAM_EXTERN int pam_sm_open_session(pam_handle_t *pamh, int flags, int argc, const char *argv[]) {
    /* When the application wants to open a session, this function is called. Here we should
     * build the user environment (setting environment variables, mounting directories etc) */
    // std::clog << "Open session" << std::endl;

    return getUserAndAuth(pamh, flags, argc, argv);
}

/* expected hook */
PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char *argv[]) {
    // std::clog << "Setting credentials" << std::endl;
    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char *argv[]) {
    // std::clog << "Account mgmt" << std::endl;
    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_close_session(pam_handle_t *pamh, int flags, int argc, const char *argv[]) {
    /* Here we destroy the environment we have created above */
    // std::clog << "Close session" << std::endl;

    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_chauthtok(pam_handle_t *pamh, int flags, int argc, const char *argv[]) {
    /* This function is called to change the authentication token. Here we should,
     * for example, change the user password with the new password */
    // std::clog << "Change auth hook" << std::endl;

    return PAM_SUCCESS;
}
}