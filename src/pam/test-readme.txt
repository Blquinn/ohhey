In order to have this actually run, you need to:

1. Compile the shared lib.
2. Copy/Link the shared lib to /lib/security/pam_ohhey.so
3. Add the following to /etc/pam.d/check_user
    auth sufficient pam_ohhey.so
    account sufficient pam_ohhey.so
4. Run the test with ./pam_ohhey_test <user>
