#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <security/pam_modules.h>
#include <stdio.h>

const struct pam_conv conv = {
	misc_conv,
	NULL
};

void end_and_exit(pam_handle_t* pamh, int retval){
	if (pam_end(pamh, retval) != PAM_SUCCESS) {
		pamh = NULL;
		printf("check_user: failed to release authenticator\n");
		exit(1);
	}

	exit(0);
}

int main(int argc, char *argv[]) {
	if(argc != 2) {
		printf("Usage: app [username]\n");
		return 1;
	}

	const auto user = argv[1];

	int retval;
	pam_handle_t* pamh = NULL;
	if ((retval = pam_start("check_user", user, &conv, &pamh)) == PAM_SUCCESS) {
		printf("Began check_user.\n");
	} else {
		end_and_exit(pamh, retval);
	}

	if ((retval = pam_authenticate(pamh, 0)) == PAM_SUCCESS) {
		printf("Authenticated.\n");
	} else {
		end_and_exit(pamh, retval);
	}

	if ((retval = pam_acct_mgmt(pamh, 0)) == PAM_SUCCESS) {
		printf("Account is valid.\n");
	} else {
		end_and_exit(pamh, retval);
	}

	return retval == PAM_SUCCESS ? 0 : 1;
}
