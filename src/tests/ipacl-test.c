#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

#include <pulsecore/log.h>
#include <pulsecore/macro.h>
#include <pulsecore/socket.h>
#include <pulsecore/ipacl.h>
#include <pulsecore/arpa-inet.h>

static void do_ip_acl_check(const char *s, int fd, int expected) {
    pa_ip_acl *acl;
    int result;

    pa_assert_se(acl = pa_ip_acl_new(s));
    result = pa_ip_acl_check(acl, fd);
    pa_ip_acl_free(acl);

    pa_log_info("%-20s result=%u (should be %u)", s, result, expected);
    pa_assert(result == expected);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in sa;
#ifdef HAVE_IPV6
    struct sockaddr_in6 sa6;
#endif
    int fd;
    int r;

    if (!getenv("MAKE_CHECK"))
        pa_log_set_level(PA_LOG_DEBUG);

    fd = socket(PF_INET, SOCK_STREAM, 0);
    pa_assert(fd >= 0);

    sa.sin_family = AF_INET;
    sa.sin_port = htons(22);
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");

    r = connect(fd, (struct sockaddr*) &sa, sizeof(sa));
    pa_assert(r >= 0);

    do_ip_acl_check("127.0.0.1", fd, 1);
    do_ip_acl_check("127.0.0.2/0", fd, 1);
    do_ip_acl_check("127.0.0.1/32", fd, 1);
    do_ip_acl_check("127.0.0.1/7", fd, 1);
    do_ip_acl_check("127.0.0.2", fd, 0);
    do_ip_acl_check("127.0.0.0/8;0.0.0.0/32", fd, 1);
    do_ip_acl_check("128.0.0.2/9", fd, 0);
    do_ip_acl_check("::1/9", fd, 0);

    close(fd);

#ifdef HAVE_IPV6
    if ( (fd = socket(PF_INET6, SOCK_STREAM, 0)) < 0 ) {
      pa_log_error("Unable to open IPv6 socket, IPv6 tests ignored");
      return 0;
    }

    memset(&sa6, 0, sizeof(sa6));
    sa6.sin6_family = AF_INET6;
    sa6.sin6_port = htons(22);
    pa_assert_se(inet_pton(AF_INET6, "::1", &sa6.sin6_addr) == 1);

    r = connect(fd, (struct sockaddr*) &sa6, sizeof(sa6));
    pa_assert(r >= 0);

    do_ip_acl_check("::1", fd, 1);
    do_ip_acl_check("::1/9", fd, 1);
    do_ip_acl_check("::/0", fd, 1);
    do_ip_acl_check("::2/128", fd, 0);
    do_ip_acl_check("::2/127", fd, 0);
    do_ip_acl_check("::2/126", fd, 1);

    close(fd);
#endif

    return 0;
}
