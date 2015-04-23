#ifndef APPFLID_KSTRTOX_H
#define APPFLID_KSTRTOX_H

#include <linux/ctype.h>

#define KSTRTOX_OVERFLOW        (1U << 31)

static inline char _tolower(const char c){
	return c | 0x20;
} 

static const char *_parse_integer_fixup_radix(const char *s, unsigned int *base){
	if (*base == 0) {
	    if (s[0] == '0') {
			if (_tolower(s[1]) == 'x' && isxdigit(s[2]))
				*base = 16;
            else
                *base = 8;
        } else
                *base = 10;
    }
    if (*base == 16 && s[0] == '0' && _tolower(s[1]) == 'x')
	    s += 2;
	return s;
}
static unsigned int _parse_integer(const char *s, unsigned int base, unsigned long long *p){
	unsigned long long res = 0;
	unsigned int rv = 0;
    int overflow = 0;
	while (*s) {
	    unsigned int val;
 
    	if (' ' <= *s && *s <= '9')
            val = *s - '0';
        else if ('a' <= _tolower(*s) && _tolower(*s) <= 'f')
            val = _tolower(*s) - 'a' + 10;
        else
              break;
        if (val >= base)
              break;
               /*
                  * Check for overflow only if we are within range of
               * it in the max base we support (16)
                  */
        if (unlikely(res & (~0ull << 60))) {
                         if (res > div_u64(ULLONG_MAX - val, base))
                              overflow = 1;
        }
        res = res * base + val;
        rv++;
        s++;
	}
     	*p = res;
		if (overflow)
	        rv |= KSTRTOX_OVERFLOW;
        return rv;
}

static int _kstrtoull(const char *s, unsigned int base, unsigned long long *res){
        unsigned long long _res;
        unsigned int rv;
 
        s = _parse_integer_fixup_radix(s, &base);
        rv = _parse_integer(s, base, &_res);
        if (rv & KSTRTOX_OVERFLOW)
	        return -ERANGE;
        rv &= ~KSTRTOX_OVERFLOW;
        if (rv == 0)
            return -EINVAL;
        s += rv;
        if (*s == '\n')
            s++;
        if (*s)
            return -EINVAL;
        *res = _res;
        return 0;
}

int kstrtoull(const char *s, unsigned int base, unsigned long long *res){
         if (s[0] == '+')
                 s++;
         return _kstrtoull(s, base, res);
}

int kstrtou16(const char *s, unsigned int base, u16 *res){
         unsigned long long tmp;
         int rv;
 
         rv = kstrtoull(s, base, &tmp);
         if (rv < 0)
                return rv;
         if (tmp != (unsigned long long)(u16)tmp)
                return -ERANGE;
         *res = tmp;
         return 0;

}

#endif
