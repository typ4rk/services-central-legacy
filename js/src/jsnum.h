/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef jsnum_h___
#define jsnum_h___

#include <math.h>

#include "jsobj.h"

/*
 * JS number (IEEE double) interface.
 *
 * JS numbers are optimistically stored in the top 31 bits of 32-bit integers,
 * but floating point literals, results that overflow 31 bits, and division and
 * modulus operands and results require a 64-bit IEEE double.  These are GC'ed
 * and pointed to by 32-bit jsvals on the stack and in object properties.
 */

/*
 * The ARM architecture supports two floating point models: VFP and FPA. When
 * targetting FPA, doubles are mixed-endian on little endian ARMs (meaning that
 * the high and low words are in big endian order).
 */
#if defined(__arm) || defined(__arm32__) || defined(__arm26__) || defined(__arm__)
#if !defined(__VFP_FP__)
#define FPU_IS_ARM_FPA
#endif
#endif

/* Low-level floating-point predicates. See bug 640494. */
#define JSDOUBLE_HI32_SIGNBIT   0x80000000
#define JSDOUBLE_HI32_EXPMASK   0x7ff00000
#define JSDOUBLE_HI32_MANTMASK  0x000fffff
#define JSDOUBLE_HI32_NAN       0x7ff80000
#define JSDOUBLE_LO32_NAN       0x00000000

#define JSDOUBLE_HI32_EXPSHIFT  20
#define JSDOUBLE_EXPBIAS        1023

typedef union jsdpun {
    struct {
#if defined(IS_LITTLE_ENDIAN) && !defined(FPU_IS_ARM_FPA)
        uint32_t lo, hi;
#else
        uint32_t hi, lo;
#endif
    } s;
    uint64_t u64;
    double d;
} jsdpun;

static inline int
JSDOUBLE_IS_NaN(double d)
{
    jsdpun u;
    u.d = d;
    return (u.u64 & JSDOUBLE_EXPMASK) == JSDOUBLE_EXPMASK &&
           (u.u64 & JSDOUBLE_MANTMASK) != 0;
}

static inline int
JSDOUBLE_IS_FINITE(double d)
{
    /* -0 is finite. NaNs are not. */
    jsdpun u;
    u.d = d;
    return (u.u64 & JSDOUBLE_EXPMASK) != JSDOUBLE_EXPMASK;
}

static inline int
JSDOUBLE_IS_INFINITE(double d)
{
    jsdpun u;
    u.d = d;
    return (u.u64 & ~JSDOUBLE_SIGNBIT) == JSDOUBLE_EXPMASK;
}

static inline bool
JSDOUBLE_IS_NEG(double d)
{
    jsdpun u;
    u.d = d;
    return (u.s.hi & JSDOUBLE_HI32_SIGNBIT) != 0;
}

static inline uint32_t
JS_HASH_DOUBLE(double d)
{
    jsdpun u;
    u.d = d;
    return u.s.lo ^ u.s.hi;
}

extern double js_NaN;
extern double js_PositiveInfinity;
extern double js_NegativeInfinity;

namespace js {

extern bool
InitRuntimeNumberState(JSRuntime *rt);

extern void
FinishRuntimeNumberState(JSRuntime *rt);

} /* namespace js */

/* Initialize the Number class, returning its prototype object. */
extern JSObject *
js_InitNumberClass(JSContext *cx, JSObject *obj);

/*
 * String constants for global function names, used in jsapi.c and jsnum.c.
 */
extern const char js_Infinity_str[];
extern const char js_NaN_str[];
extern const char js_isNaN_str[];
extern const char js_isFinite_str[];
extern const char js_parseFloat_str[];
extern const char js_parseInt_str[];

class JSString;
class JSFixedString;

extern JSString * JS_FASTCALL
js_IntToString(JSContext *cx, jsint i);

/*
 * When base == 10, this function implements ToString() as specified by
 * ECMA-262-5 section 9.8.1; but note that it handles integers specially for
 * performance.  See also js::NumberToCString().
 */
extern JSString * JS_FASTCALL
js_NumberToString(JSContext *cx, double d);

namespace js {

/*
 * Convert an integer or double (contained in the given value) to a string and
 * append to the given buffer.
 */
extern bool JS_FASTCALL
NumberValueToStringBuffer(JSContext *cx, const Value &v, StringBuffer &sb);

/* Same as js_NumberToString, different signature. */
extern JSFixedString *
NumberToString(JSContext *cx, double d);

extern JSFixedString *
IndexToString(JSContext *cx, uint32_t index);

/*
 * Usually a small amount of static storage is enough, but sometimes we need
 * to dynamically allocate much more.  This struct encapsulates that.
 * Dynamically allocated memory will be freed when the object is destroyed.
 */
struct ToCStringBuf
{
    /*
     * The longest possible result that would need to fit in sbuf is
     * (-0x80000000).toString(2), which has length 33.  Longer cases are
     * possible, but they'll go in dbuf.
     */
    static const size_t sbufSize = 34;
    char sbuf[sbufSize];
    char *dbuf;

    ToCStringBuf();
    ~ToCStringBuf();
};

/*
 * Convert a number to a C string.  When base==10, this function implements
 * ToString() as specified by ECMA-262-5 section 9.8.1.  It handles integral
 * values cheaply.  Return NULL if we ran out of memory.  See also
 * js_NumberToCString().
 */
extern char *
NumberToCString(JSContext *cx, ToCStringBuf *cbuf, double d, jsint base = 10);

/*
 * The largest positive integer such that all positive integers less than it
 * may be precisely represented using the IEEE-754 double-precision format.
 */
const double DOUBLE_INTEGRAL_PRECISION_LIMIT = uint64_t(1) << 53;

/*
 * Compute the positive integer of the given base described immediately at the
 * start of the range [start, end) -- no whitespace-skipping, no magical
 * leading-"0" octal or leading-"0x" hex behavior, no "+"/"-" parsing, just
 * reading the digits of the integer.  Return the index one past the end of the
 * digits of the integer in *endp, and return the integer itself in *dp.  If
 * base is 10 or a power of two the returned integer is the closest possible
 * double; otherwise extremely large integers may be slightly inaccurate.
 *
 * If [start, end) does not begin with a number with the specified base,
 * *dp == 0 and *endp == start upon return.
 */
extern bool
GetPrefixInteger(JSContext *cx, const jschar *start, const jschar *end, int base,
                 const jschar **endp, double *dp);

/* ES5 9.3 ToNumber. */
JS_ALWAYS_INLINE bool
ToNumber(JSContext *cx, const Value &v, double *out)
{
    if (v.isNumber()) {
        *out = v.toNumber();
        return true;
    }
    extern bool ToNumberSlow(JSContext *cx, js::Value v, double *dp);
    return ToNumberSlow(cx, v, out);
}

/* ES5 9.3 ToNumber, overwriting *vp with the appropriate number value. */
JS_ALWAYS_INLINE bool
ToNumber(JSContext *cx, Value *vp)
{
    if (vp->isNumber())
        return true;
    double d;
    extern bool ToNumberSlow(JSContext *cx, js::Value v, double *dp);
    if (!ToNumberSlow(cx, *vp, &d))
        return false;
    vp->setNumber(d);
    return true;
}

/*
 * Convert a value to an int32_t or uint32_t, according to the ECMA rules for
 * ToInt32 and ToUint32. Return converted value in *out on success, !ok on
 * failure.
 */
JS_ALWAYS_INLINE bool
ToInt32(JSContext *cx, const js::Value &v, int32_t *out)
{
    if (v.isInt32()) {
        *out = v.toInt32();
        return true;
    }
    extern bool ToInt32Slow(JSContext *cx, const js::Value &v, int32_t *ip);
    return ToInt32Slow(cx, v, out);
}

JS_ALWAYS_INLINE bool
ToUint32(JSContext *cx, const js::Value &v, uint32_t *out)
{
    if (v.isInt32()) {
        *out = (uint32_t)v.toInt32();
        return true;
    }
    extern bool ToUint32Slow(JSContext *cx, const js::Value &v, uint32_t *ip);
    return ToUint32Slow(cx, v, out);
}

/*
 * Convert a value to a number, then to an int32_t if it fits by rounding to
 * nearest. Return converted value in *out on success, !ok on failure. As a
 * side effect, *vp will be mutated to match *out.
 */
JS_ALWAYS_INLINE bool
NonstandardToInt32(JSContext *cx, const js::Value &v, int32_t *out)
{
    if (v.isInt32()) {
        *out = v.toInt32();
        return true;
    }
    extern bool NonstandardToInt32Slow(JSContext *cx, const js::Value &v, int32_t *ip);
    return NonstandardToInt32Slow(cx, v, out);
}

/*
 * Convert a value to a number, then to a uint16_t according to the ECMA rules
 * for ToUint16. Return converted value on success, !ok on failure. v must be a
 * copy of a rooted value.
 */
JS_ALWAYS_INLINE bool
ValueToUint16(JSContext *cx, const js::Value &v, uint16_t *out)
{
    if (v.isInt32()) {
        *out = uint16_t(v.toInt32());
        return true;
    }
    extern bool ValueToUint16Slow(JSContext *cx, const js::Value &v, uint16_t *out);
    return ValueToUint16Slow(cx, v, out);
}

JSBool
num_parseInt(JSContext *cx, unsigned argc, Value *vp);

}  /* namespace js */

/*
 * Specialized ToInt32 and ToUint32 converters for doubles.
 */
/*
 * From the ES3 spec, 9.5
 *  2.  If Result(1) is NaN, +0, -0, +Inf, or -Inf, return +0.
 *  3.  Compute sign(Result(1)) * floor(abs(Result(1))).
 *  4.  Compute Result(3) modulo 2^32; that is, a finite integer value k of Number
 *      type with positive sign and less than 2^32 in magnitude such the mathematical
 *      difference of Result(3) and k is mathematically an integer multiple of 2^32.
 *  5.  If Result(4) is greater than or equal to 2^31, return Result(4)- 2^32,
 *  otherwise return Result(4).
 */
static inline int32_t
js_DoubleToECMAInt32(double d)
{
#if defined(__i386__) || defined(__i386) || defined(__x86_64__) || \
    defined(_M_IX86) || defined(_M_X64)
    jsdpun du, duh, two32;
    uint32_t di_h, u_tmp, expon, shift_amount;
    int32_t mask32;

    /*
     * Algorithm Outline
     *  Step 1. If d is NaN, +/-Inf or |d|>=2^84 or |d|<1, then return 0
     *          All of this is implemented based on an exponent comparison.
     *  Step 2. If |d|<2^31, then return (int)d
     *          The cast to integer (conversion in RZ mode) returns the correct result.
     *  Step 3. If |d|>=2^32, d:=fmod(d, 2^32) is taken  -- but without a call
     *  Step 4. If |d|>=2^31, then the fractional bits are cleared before
     *          applying the correction by 2^32:  d - sign(d)*2^32
     *  Step 5. Return (int)d
     */

    du.d = d;
    di_h = du.s.hi;

    u_tmp = (di_h & 0x7ff00000) - 0x3ff00000;
    if (u_tmp >= (0x45300000-0x3ff00000)) {
        // d is Nan, +/-Inf or +/-0, or |d|>=2^(32+52) or |d|<1, in which case result=0
        return 0;
    }

    if (u_tmp < 0x01f00000) {
        // |d|<2^31
        return int32_t(d);
    }

    if (u_tmp > 0x01f00000) {
        // |d|>=2^32
        expon = u_tmp >> 20;
        shift_amount = expon - 21;
        duh.u64 = du.u64;
        mask32 = 0x80000000;
        if (shift_amount < 32) {
            mask32 >>= shift_amount;
            duh.s.hi = du.s.hi & mask32;
            duh.s.lo = 0;
        } else {
            mask32 >>= (shift_amount-32);
            duh.s.hi = du.s.hi;
            duh.s.lo = du.s.lo & mask32;
        }
        du.d -= duh.d;
    }

    di_h = du.s.hi;

    // eliminate fractional bits
    u_tmp = (di_h & 0x7ff00000);
    if (u_tmp >= 0x41e00000) {
        // |d|>=2^31
        expon = u_tmp >> 20;
        shift_amount = expon - (0x3ff - 11);
        mask32 = 0x80000000;
        if (shift_amount < 32) {
            mask32 >>= shift_amount;
            du.s.hi &= mask32;
            du.s.lo = 0;
        } else {
            mask32 >>= (shift_amount-32);
            du.s.lo &= mask32;
        }
        two32.s.hi = 0x41f00000 ^ (du.s.hi & 0x80000000);
        two32.s.lo = 0;
        du.d -= two32.d;
    }

    return int32_t(du.d);
#elif defined (__arm__) && defined (__GNUC__)
    int32_t i;
    uint32_t    tmp0;
    uint32_t    tmp1;
    uint32_t    tmp2;
    asm (
    // We use a pure integer solution here. In the 'softfp' ABI, the argument
    // will start in r0 and r1, and VFP can't do all of the necessary ECMA
    // conversions by itself so some integer code will be required anyway. A
    // hybrid solution is faster on A9, but this pure integer solution is
    // notably faster for A8.

    // %0 is the result register, and may alias either of the %[QR]1 registers.
    // %Q4 holds the lower part of the mantissa.
    // %R4 holds the sign, exponent, and the upper part of the mantissa.
    // %1, %2 and %3 are used as temporary values.

    // Extract the exponent.
"   mov     %1, %R4, LSR #20\n"
"   bic     %1, %1, #(1 << 11)\n"  // Clear the sign.

    // Set the implicit top bit of the mantissa. This clobbers a bit of the
    // exponent, but we have already extracted that.
"   orr     %R4, %R4, #(1 << 20)\n"

    // Special Cases
    //   We should return zero in the following special cases:
    //    - Exponent is 0x000 - 1023: +/-0 or subnormal.
    //    - Exponent is 0x7ff - 1023: +/-INFINITY or NaN
    //      - This case is implicitly handled by the standard code path anyway,
    //        as shifting the mantissa up by the exponent will result in '0'.
    //
    // The result is composed of the mantissa, prepended with '1' and
    // bit-shifted left by the (decoded) exponent. Note that because the r1[20]
    // is the bit with value '1', r1 is effectively already shifted (left) by
    // 20 bits, and r0 is already shifted by 52 bits.

    // Adjust the exponent to remove the encoding offset. If the decoded
    // exponent is negative, quickly bail out with '0' as such values round to
    // zero anyway. This also catches +/-0 and subnormals.
"   sub     %1, %1, #0xff\n"
"   subs    %1, %1, #0x300\n"
"   bmi     8f\n"

    //  %1 = (decoded) exponent >= 0
    //  %R4 = upper mantissa and sign

    // ---- Lower Mantissa ----
"   subs    %3, %1, #52\n"         // Calculate exp-52
"   bmi     1f\n"

    // Shift r0 left by exp-52.
    // Ensure that we don't overflow ARM's 8-bit shift operand range.
    // We need to handle anything up to an 11-bit value here as we know that
    // 52 <= exp <= 1024 (0x400). Any shift beyond 31 bits results in zero
    // anyway, so as long as we don't touch the bottom 5 bits, we can use
    // a logical OR to push long shifts into the 32 <= (exp&0xff) <= 255 range.
"   bic     %2, %3, #0xff\n"
"   orr     %3, %3, %2, LSR #3\n"
    // We can now perform a straight shift, avoiding the need for any
    // conditional instructions or extra branches.
"   mov     %Q4, %Q4, LSL %3\n"
"   b       2f\n"
"1:\n" // Shift r0 right by 52-exp.
    // We know that 0 <= exp < 52, and we can shift up to 255 bits so 52-exp
    // will always be a valid shift and we can sk%3 the range check for this case.
"   rsb     %3, %1, #52\n"
"   mov     %Q4, %Q4, LSR %3\n"

    //  %1 = (decoded) exponent
    //  %R4 = upper mantissa and sign
    //  %Q4 = partially-converted integer

"2:\n"
    // ---- Upper Mantissa ----
    // This is much the same as the lower mantissa, with a few different
    // boundary checks and some masking to hide the exponent & sign bit in the
    // upper word.
    // Note that the upper mantissa is pre-shifted by 20 in %R4, but we shift
    // it left more to remove the sign and exponent so it is effectively
    // pre-shifted by 31 bits.
"   subs    %3, %1, #31\n"          // Calculate exp-31
"   mov     %1, %R4, LSL #11\n"     // Re-use %1 as a temporary register.
"   bmi     3f\n"

    // Shift %R4 left by exp-31.
    // Avoid overflowing the 8-bit shift range, as before.
"   bic     %2, %3, #0xff\n"
"   orr     %3, %3, %2, LSR #3\n"
    // Perform the shift.
"   mov     %2, %1, LSL %3\n"
"   b       4f\n"
"3:\n" // Shift r1 right by 31-exp.
    // We know that 0 <= exp < 31, and we can shift up to 255 bits so 31-exp
    // will always be a valid shift and we can skip the range check for this case.
"   rsb     %3, %3, #0\n"          // Calculate 31-exp from -(exp-31)
"   mov     %2, %1, LSR %3\n"      // Thumb-2 can't do "LSR %3" in "orr".

    //  %Q4 = partially-converted integer (lower)
    //  %R4 = upper mantissa and sign
    //  %2 = partially-converted integer (upper)

"4:\n"
    // Combine the converted parts.
"   orr     %Q4, %Q4, %2\n"
    // Negate the result if we have to, and move it to %0 in the process. To
    // avoid conditionals, we can do this by inverting on %R4[31], then adding
    // %R4[31]>>31.
"   eor     %Q4, %Q4, %R4, ASR #31\n"
"   add     %0, %Q4, %R4, LSR #31\n"
"   b       9f\n"
"8:\n"
    // +/-INFINITY, +/-0, subnormals, NaNs, and anything else out-of-range that
    // will result in a conversion of '0'.
"   mov     %0, #0\n"
"9:\n"
    : "=r" (i), "=&r" (tmp0), "=&r" (tmp1), "=&r" (tmp2)
    : "r" (d)
    : "cc"
        );
    return i;
#else
    int32_t i;
    double two32, two31;

    if (!JSDOUBLE_IS_FINITE(d))
        return 0;

    i = (int32_t) d;
    if ((double) i == d)
        return i;

    two32 = 4294967296.0;
    two31 = 2147483648.0;
    d = fmod(d, two32);
    d = (d >= 0) ? floor(d) : ceil(d) + two32;
    return (int32_t) (d >= two31 ? d - two32 : d);
#endif
}

inline uint32_t
js_DoubleToECMAUint32(double d)
{
    return uint32_t(js_DoubleToECMAInt32(d));
}

/*
 * Convert a double to an integral number, stored in a double.
 * If d is NaN, return 0.  If d is an infinity, return it without conversion.
 */
static inline double
js_DoubleToInteger(double d)
{
    if (d == 0)
        return d;

    if (!JSDOUBLE_IS_FINITE(d)) {
        if (JSDOUBLE_IS_NaN(d))
            return 0;
        return d;
    }

    JSBool neg = (d < 0);
    d = floor(neg ? -d : d);

    return neg ? -d : d;
}

/*
 * Similar to strtod except that it replaces overflows with infinities of the
 * correct sign, and underflows with zeros of the correct sign.  Guaranteed to
 * return the closest double number to the given input in dp.
 *
 * Also allows inputs of the form [+|-]Infinity, which produce an infinity of
 * the appropriate sign.  The case of the "Infinity" string must match exactly.
 * If the string does not contain a number, set *ep to s and return 0.0 in dp.
 * Return false if out of memory.
 */
extern JSBool
js_strtod(JSContext *cx, const jschar *s, const jschar *send,
          const jschar **ep, double *dp);

extern JSBool
js_num_valueOf(JSContext *cx, unsigned argc, js::Value *vp);

namespace js {

static JS_ALWAYS_INLINE bool
ValueFitsInInt32(const Value &v, int32_t *pi)
{
    if (v.isInt32()) {
        *pi = v.toInt32();
        return true;
    }
    return v.isDouble() && JSDOUBLE_IS_INT32(v.toDouble(), pi);
}

/*
 * Returns true if the given value is definitely an index: that is, the value
 * is a number that's an unsigned 32-bit integer.
 *
 * This method prioritizes common-case speed over accuracy in every case.  It
 * can produce false negatives (but not false positives): some values which are
 * indexes will be reported not to be indexes by this method.  Users must
 * consider this possibility when using this method.
 */
static JS_ALWAYS_INLINE bool
IsDefinitelyIndex(const Value &v, uint32_t *indexp)
{
    if (v.isInt32() && v.toInt32() >= 0) {
        *indexp = v.toInt32();
        return true;
    }

    int32_t i;
    if (v.isDouble() && JSDOUBLE_IS_INT32(v.toDouble(), &i) && i >= 0) {
        *indexp = uint32_t(i);
        return true;
    }

    return false;
}

/* ES5 9.4 ToInteger. */
static inline bool
ToInteger(JSContext *cx, const js::Value &v, double *dp)
{
    if (v.isInt32()) {
        *dp = v.toInt32();
        return true;
    }
    if (v.isDouble()) {
        *dp = v.toDouble();
    } else {
        extern bool ToNumberSlow(JSContext *cx, Value v, double *dp);
        if (!ToNumberSlow(cx, v, dp))
            return false;
    }
    *dp = js_DoubleToInteger(*dp);
    return true;
}

} /* namespace js */

#endif /* jsnum_h___ */
