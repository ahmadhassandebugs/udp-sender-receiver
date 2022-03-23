#ifndef UDP_UTILS_H
#define UDP_UTILS_H

#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <memory>
#include <arpa/inet.h>
#include <system_error>

/* tagged_error: system_error + name of what was being attempted */
class tagged_error : public std::system_error {
private:
    std::string attempt_and_error_;

public:
    tagged_error(const std::error_category &category,
                 const std::string &s_attempt,
                 const int error_code)
                 : std::system_error(error_code, category),
                 attempt_and_error_( s_attempt + ": " + std::system_error::what() ) {};

    const char *what() const noexcept override {
            return attempt_and_error_.c_str();
    }
};

/* unix_error: a tagged_error for system calls */
class unix_error : public tagged_error {
public:
    explicit unix_error(const std::string &s_attempt,
               const int s_errno=errno)
               : tagged_error(std::system_category(), s_attempt, s_errno) {};
};

/* print the error to the terminal */
inline void print_exception(const std::exception &e) {
    std::cerr << e.what() << std::endl;
}

/* error-checking wrapper for most system calls */
inline int SystemCall(const char *s_attempt, const int return_value) {
    if (return_value >= 0) {
        return return_value;
    }
    throw unix_error(s_attempt);
}

/* version of SystemCall that takes a C++ std::string */
inline int SystemCall(const std::string &s_attempt, const int return_value) {
    return SystemCall(s_attempt.c_str(), return_value);
}

/* zero out an arbitrary structure */
template <typename T> void zero(T &x) {
    memset(&x, 0, sizeof(x));
}

/* Macros - not type-safe, has side-effects. Use inline functions instead */
inline std::string BoolToString(bool b)
{
  return b ? "true" : "false";
}

inline void Error(const char * format, ...) {
  char msg[4096];
  va_list argptr;
  va_start(argptr, format);
  vsprintf(msg, format, argptr);
  va_end(argptr);
  fprintf(stderr, "Error: %s\n", msg);
  exit(-1);
}

inline void Log(const char * format, ...) {
  char msg[2048];
  va_list argptr;
  va_start(argptr, format);
  vsprintf(msg, format, argptr);
  va_end(argptr);
  fprintf(stderr, "%s\n", msg);
}

// Convert a struct sockaddr address to a string, IPv4 and IPv6:
inline char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen)
{
    switch(sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
                    s, maxlen);
            break;

        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
                    s, maxlen);
            break;

        default:
            strncpy(s, "Unknown AF", maxlen);
            return NULL;
    }
    return s;
}

/* set socket option */
template <typename option_type>
inline void setsocketopt(const int fd, const int level, const int option, const option_type &option_Value)
{
    SystemCall("setsockopt", setsockopt(fd, level, option, &option_Value, sizeof(option_Value)));
}

/* allow local address to be reused sooner, at the cost of some robustness */
inline void set_reuseaddr(const int fd)
{
    setsocketopt(fd, SOL_SOCKET, SO_REUSEADDR, int(true));
}

/* turn on timestamps on receipt */
inline void set_timestamps(const int fd)
{
    setsocketopt(fd, SOL_SOCKET, SO_TIMESTAMPNS, int (true));
}

/* connect socket to a specified peer address */
inline void connect_socket_to_address(const int fd, const struct sockaddr *sa, socklen_t len)
{
    SystemCall("connect", connect(fd, sa, len));
}



inline std::string string_format(const std::string fmt_str, ...) {
    int final_n, n = ((int)fmt_str.size()) * 2; /* Reserve two times as much as the length of the fmt_str */
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while(1) {
        formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
        strcpy(&formatted[0], fmt_str.c_str());
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return std::string(formatted.get());
}

#endif //UDP_UTILS_H